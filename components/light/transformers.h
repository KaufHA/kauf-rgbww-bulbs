#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "light_color_values.h"
#include "light_state.h"
#include "light_transformer.h"

namespace esphome::light {

class LightTransitionTransformer : public LightTransformer {
 public:

  // KAUF: variables for start and end points
  float start_r, start_g, start_b, start_ct, start_wb;
  float end_r, end_g, end_b, end_ct, end_wb;

  // KAUF: precomputed reverse gamma values (computed once in start(), used every frame)
  float start_r_rev, start_g_rev, start_b_rev, start_wb_rev;
  float end_r_rev, end_g_rev, end_b_rev, end_wb_rev;

  void start() override {
    // When turning light on from off state, use target state and only increase brightness from zero.
    if (!this->start_values_.is_on() && this->target_values_.is_on()) {
      this->start_values_ = LightColorValues(this->target_values_);
      this->start_values_.set_brightness(0.0f);
    }

    // KAUF: if starting in RGB, clear white brightness and vice versa
    if ( this->start_values_.get_color_mode() & ColorCapability::RGB) {
      this->start_values_.set_white(0.0f);
      this->start_values_.set_color_temperature(this->target_values_.get_color_temperature());
    } else {
      this->start_values_.set_red(0.0f);
      this->start_values_.set_green(0.0f);
      this->start_values_.set_blue(0.0f);
    }


    // When turning light off from on state, use source state and only decrease brightness to zero. Use a second
    // variable for transition end state, as overwriting target_values breaks LightState logic.
    if (this->start_values_.is_on() && !this->target_values_.is_on()) {
      this->end_values_ = LightColorValues(this->start_values_);
      this->end_values_.set_brightness(0.0f);
    } else {
      this->end_values_ = LightColorValues(this->target_values_);
    }

    // KAUF: if ending in RGB, clear white brightness and vice versa
    if ( this->end_values_.get_color_mode() & ColorCapability::RGB) {
      this->end_values_.set_white(0.0f);
      this->end_values_.set_color_temperature(this->start_values_.get_color_temperature());
    } else {
      this->end_values_.set_red(0.0f);
      this->end_values_.set_green(0.0f);
      this->end_values_.set_blue(0.0f);
    }

    // get starting and ending actual RGBCW values to be output including gamma and brightness
    // If start_values_ is already raw (output-space), avoid applying gamma twice.
    if (this->start_values_.use_raw) {
      start_r = this->start_values_.get_red();
      start_g = this->start_values_.get_green();
      start_b = this->start_values_.get_blue();
      // Color temperature is stored in mireds; convert back to 0..1 for our CT interpolation.
      const float ct_mireds = this->start_values_.get_color_temperature();
      start_ct = (ct_mireds - 150.0f) / (350.0f - 150.0f);
      start_ct = clamp(start_ct, 0.0f, 1.0f);
      // In apply(), WB is written via set_brightness(), so use brightness here.
      start_wb = this->start_values_.get_brightness();
    } else {
      this->start_values_.as_rgbct(150, 350, &start_r, &start_g, &start_b, &start_ct, &start_wb, 2.8f);
    }
    this->end_values_.as_rgbct(150, 350, &end_r, &end_g, &end_b, &end_ct, &end_wb, 2.8f);

    // precompute reverse gamma values once, used every frame in apply()
    start_r_rev = kauf_gamma_rev(start_r);
    start_g_rev = kauf_gamma_rev(start_g);
    start_b_rev = kauf_gamma_rev(start_b);
    start_wb_rev = kauf_gamma_rev(start_wb);
    end_r_rev = kauf_gamma_rev(end_r);
    end_g_rev = kauf_gamma_rev(end_g);
    end_b_rev = kauf_gamma_rev(end_b);
    end_wb_rev = kauf_gamma_rev(end_wb);

    ESP_LOGV("KAUF Transformer","");
    ESP_LOGV("KAUF Transformer","/////////////////////////////////////////////////////////////////////////////");
    ESP_LOGV("KAUF Transformer","Start Values: R:%f  G:%f  B:%f  CT:%f  WB:%f", start_r, start_g, start_b, start_ct, start_wb);
    ESP_LOGV("KAUF Transformer","End Values:   R:%f  G:%f  B:%f  CT:%f  WB:%f", end_r, end_g, end_b, end_ct, end_wb);
    ESP_LOGV("KAUF Transformer","/////////////////////////////////////////////////////////////////////////////");
    ESP_LOGV("KAUF Transformer","");

  }

  optional<LightColorValues> apply() override {
    float p = this->get_progress_();

    // RGB variables.  CT F for float value, CT I for integer mired value
    float red, green, blue, ct_f, ct_i, wb;

    // ct is just straight linear interpolation.
    ct_f = ((end_ct-start_ct)*p) + start_ct;
    ct_i = 200*ct_f + 150; // need mireds for set_color_temperature function

    // apply Tasmota's fast gamma between start and end
    // uses precomputed reverse gamma values from start()
    red = kauf_gamma((end_r_rev - start_r_rev) * p + start_r_rev);
    green = kauf_gamma((end_g_rev - start_g_rev) * p + start_g_rev);
    blue = kauf_gamma((end_b_rev - start_b_rev) * p + start_b_rev);
    wb = kauf_gamma((end_wb_rev - start_wb_rev) * p + start_wb_rev);

//    ESP_LOGD("KAUF Transformer","Progress Values: P:%f R:%f  G:%f  B:%f  CT:%f:%f  WB:%f", p, red, green, blue, ct_f, ct_i, wb);

    LightColorValues kauf_display;
    kauf_display.set_color_mode(this->end_values_.get_color_mode());
    kauf_display.set_state(((this->end_values_.get_state() - this->start_values_.get_state()) * p) + this->start_values_.get_state());

    kauf_display.set_red(red);
    kauf_display.set_green(green);
    kauf_display.set_blue(blue);
    kauf_display.set_color_temperature(ct_i);
    kauf_display.set_brightness(wb);
    kauf_display.use_raw = true;

//    ESP_LOGD("KAUF Transformer","Return Values: P:%f R:%f  G:%f  B:%f  CT:%f  WB:%f", p, red, green, blue, ct_i, wb);

    return kauf_display;

  }

 protected:

  // Tasmota fast gamma table constants (precomputed at compile time)
  // grabbing fast gamma table from Tasmota xdrv_04_light_utils.ino
  // input < 0        :: output = 0
  // input   0 -  384 :: output   0 -  192        2x
  // input 384 -  768 :: output 192 -  576        1x (both are 384)
  // input 768 - 1023 :: output 576 - 1023         255:447 = .57...
  // input > 1023     :: output = 1023
  static constexpr float GAMMA_I1 = 384.0f / 1023.0f;
  static constexpr float GAMMA_I2 = 768.0f / 1023.0f;
  static constexpr float GAMMA_O1 = 192.0f / 1023.0f;
  static constexpr float GAMMA_O2 = 576.0f / 1023.0f;

  // precomputed slopes for each segment
  static constexpr float GAMMA_SLOPE1 = GAMMA_O1 / GAMMA_I1;                         // segment 1: 0.5
  static constexpr float GAMMA_SLOPE2 = (GAMMA_O2 - GAMMA_O1) / (GAMMA_I2 - GAMMA_I1); // segment 2: 1.0
  static constexpr float GAMMA_SLOPE3 = (1.0f - GAMMA_O2) / (1.0f - GAMMA_I2);         // segment 3: ~1.75

  static float kauf_gamma(float x) {
    if (x >= 0.0f && x <= GAMMA_I1) {
      return x * GAMMA_SLOPE1;
    } else if (x <= GAMMA_I2) {
      return (x - GAMMA_I1) * GAMMA_SLOPE2 + GAMMA_O1;
    } else if (x <= 1.0f) {
      return (x - GAMMA_I2) * GAMMA_SLOPE3 + GAMMA_O2;
    }
    return 0.0f;
  }

  // reverse gamma: same piecewise function with inputs/outputs swapped
  static constexpr float GAMMA_REV_SLOPE1 = GAMMA_I1 / GAMMA_O1;                         // 2.0
  static constexpr float GAMMA_REV_SLOPE2 = (GAMMA_I2 - GAMMA_I1) / (GAMMA_O2 - GAMMA_O1); // 1.0
  static constexpr float GAMMA_REV_SLOPE3 = (1.0f - GAMMA_I2) / (1.0f - GAMMA_O2);         // ~0.57

  static float kauf_gamma_rev(float x) {
    if (x >= 0.0f && x <= GAMMA_O1) {
      return x * GAMMA_REV_SLOPE1;
    } else if (x <= GAMMA_O2) {
      return (x - GAMMA_O1) * GAMMA_REV_SLOPE2 + GAMMA_I1;
    } else if (x <= 1.0f) {
      return (x - GAMMA_O2) * GAMMA_REV_SLOPE3 + GAMMA_I2;
    }
    return 0.0f;
  }


  LightColorValues end_values_{};
  LightColorValues intermediate_values_{};
  bool changing_color_mode_{false};
};

class LightFlashTransformer : public LightTransformer {
 public:
  LightFlashTransformer(LightState &state) : state_(state) {}

  void start() override {
    this->transition_length_ = this->state_.get_flash_transition_length();
    if (this->transition_length_ * 2 > this->length_)
      this->transition_length_ = this->length_ / 2;

    this->begun_lightstate_restore_ = false;

    // first transition to original target
    this->transformer_ = this->state_.get_output()->create_default_transition();
    this->transformer_->setup(this->state_.current_values, this->target_values_, this->transition_length_);
  }

  optional<LightColorValues> apply() override {
    optional<LightColorValues> result = {};

    if (this->transformer_ == nullptr && millis() > this->start_time_ + this->length_ - this->transition_length_) {
      // second transition back to start value
      this->transformer_ = this->state_.get_output()->create_default_transition();
      this->transformer_->setup(this->state_.current_values, this->get_start_values(), this->transition_length_);
      this->begun_lightstate_restore_ = true;
    }

    if (this->transformer_ != nullptr) {
      result = this->transformer_->apply();

      if (this->transformer_->is_finished()) {
        this->transformer_->stop();
        this->transformer_ = nullptr;
      }
    }

    return result;
  }

  // Restore the original values after the flash.
  void stop() override {
    if (this->transformer_ != nullptr) {
      this->transformer_->stop();
      this->transformer_ = nullptr;
    }
    this->state_.current_values = this->get_start_values();
    this->state_.remote_values = this->get_start_values();
    this->state_.publish_state();
  }

  bool is_finished() override { return this->begun_lightstate_restore_ && LightTransformer::is_finished(); }

 protected:
  LightState &state_;
  std::unique_ptr<LightTransformer> transformer_{nullptr};
  uint32_t transition_length_;
  bool begun_lightstate_restore_;
};

}  // namespace esphome::light
