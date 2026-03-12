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

  void start() override {
    // When turning light on from off state, use target state and only increase brightness from zero.
    if (!this->start_values_.is_on() && this->target_values_.is_on()) {
      this->start_values_ = LightColorValues(this->target_values_);
      this->start_values_.set_brightness(0.0f);
    }

    // KAUF: if starting in RGB, clear white brightness and vice versa
    if ( this->start_values_.color_mode_ & ColorCapability::RGB) {
      this->start_values_.white_ = 0.0f;
      this->start_values_.color_temperature_ = this->target_values_.color_temperature_;
    } else {
      this->start_values_.red_ = 0.0f;
      this->start_values_.green_ = 0.0f;
      this->start_values_.blue_ = 0.0f;
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
    if ( this->end_values_.color_mode_ & ColorCapability::RGB) {
      this->end_values_.white_ = 0.0f;
      this->end_values_.color_temperature_ = this->start_values_.color_temperature_;
    } else {
      this->end_values_.red_ = 0.0f;
      this->end_values_.green_ = 0.0f;
      this->end_values_.blue_ = 0.0f;
    }

    // Get starting and ending RGB/WB/CT endpoints.
    this->start_values_.as_rgb(&start_r, &start_g, &start_b);
    start_wb = this->start_values_.get_white_brightness();
    start_ct = this->start_values_.get_color_temperature();
    this->end_values_.as_rgb(&end_r, &end_g, &end_b);
    end_wb = this->end_values_.get_white_brightness();
    end_ct = this->end_values_.get_color_temperature();

    ESP_LOGV("KAUF Transformer","");
    ESP_LOGV("KAUF Transformer","/////////////////////////////////////////////////////////////////////////////");
    ESP_LOGV("KAUF Transformer","Start Values: R:%f  G:%f  B:%f  CT:%f  WB:%f", start_r, start_g, start_b, start_ct, start_wb);
    ESP_LOGV("KAUF Transformer","End Values:   R:%f  G:%f  B:%f  CT:%f  WB:%f", end_r, end_g, end_b, end_ct, end_wb);
    ESP_LOGV("KAUF Transformer","/////////////////////////////////////////////////////////////////////////////");
    ESP_LOGV("KAUF Transformer","");

  }

  optional<LightColorValues> apply() override {
    float p = this->get_progress_();

    // RGB variables and CT in mireds.
    float red, green, blue, ct_i, wb;

    // CT is linearly interpolated in mired space.
    ct_i = ((end_ct - start_ct) * p) + start_ct;

    // KAUF: interpolate linearly in output space.
    red = ((end_r - start_r) * p) + start_r;
    green = ((end_g - start_g) * p) + start_g;
    blue = ((end_b - start_b) * p) + start_b;
    wb = ((end_wb - start_wb) * p) + start_wb;

//    ESP_LOGD("KAUF Transformer","Progress Values: P:%f R:%f  G:%f  B:%f  CT:%f  WB:%f", p, red, green, blue, ct_i, wb);

    LightColorValues kauf_display;
    kauf_display.color_mode_ = this->end_values_.color_mode_;
    kauf_display.state_ = ((this->end_values_.state_ - this->start_values_.state_) * p) + this->start_values_.state_;
    kauf_display.red_ = red;
    kauf_display.green_ = green;
    kauf_display.blue_ = blue;
    kauf_display.color_temperature_ = ct_i;
    kauf_display.brightness_ = wb;

//    ESP_LOGD("KAUF Transformer","Return Values: P:%f R:%f  G:%f  B:%f  CT:%f  WB:%f", p, red, green, blue, ct_i, wb);

    return kauf_display;

  }

 protected:
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

    if (this->transformer_ == nullptr && millis() - this->start_time_ > this->length_ - this->transition_length_) {
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
