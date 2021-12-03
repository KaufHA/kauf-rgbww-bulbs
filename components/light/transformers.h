#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "light_color_values.h"
#include "light_state.h"
#include "light_transformer.h"

namespace esphome {
namespace light {

class LightTransitionTransformer : public LightTransformer {
 public:

  float start_r, start_g, start_b, start_ct, start_wb;
  float end_r, end_g, end_b, end_ct, end_wb;

  void start() override {
    // When turning light on from off state, use target state and only increase brightness from zero.
    if (!this->start_values_.is_on() && this->target_values_.is_on()) {
      this->start_values_ = LightColorValues(this->target_values_);
      this->start_values_.set_brightness(0.0f);
    }

    // if starting in RGB, clear white brightness and vice versa
    if ( this->start_values_.get_color_mode() & ColorCapability::RGB) {
      this->start_values_.set_white(0.0f);
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

    // if ending in RGB, clear white brightness and vice versa
    if ( this->end_values_.get_color_mode() & ColorCapability::RGB) {
      this->end_values_.set_white(0.0f);
    } else {
      this->end_values_.set_red(0.0f);
      this->end_values_.set_green(0.0f);
      this->end_values_.set_blue(0.0f);
    }

    // get starting and ending actual RGBCW values to be output including gamma and brightness
    this->start_values_.as_rgbct( 150, 350, &start_r, &start_g, &start_b, &start_ct, &start_wb, 2.8f);
    this->end_values_.as_rgbct(150, 350,   &end_r,   &end_g,   &end_b,   &end_ct,   &end_wb, 2.8f);


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
    red = convert_to_kauf(start_r,end_r,p);
    green = convert_to_kauf(start_g,end_g,p);
    blue = convert_to_kauf(start_b,end_b,p);
    wb = convert_to_kauf(start_wb,end_wb,p);

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
  // This looks crazy, but it reduces to 6x^5 - 15x^4 + 10x^3 which is just a smooth sigmoid-like
  // transition from 0 to 1 on x = [0, 1]
  static float smoothed_progress(float x) { return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f); }

  static float convert_to_kauf(float start, float end, float progress) {

    // begin and end are actual output values for the start and end points with gamma and everything,
    // so they need reverse gamma to figure out what would be the equivalent in Tasmota's fast gamma table
    float start_rev = kauf_gamma_rev(start);
    float end_rev = kauf_gamma_rev(end);

    //  linear interpolation of the reversed gamma points, and then re-applying gamma.
    return kauf_gamma( ((end_rev-start_rev)*progress)+start_rev  );
  
  }


  static float kauf_gamma(float x) {

    // grabbing fast gamma table from Tasmota xdrv_04_light_utils.ino
    // input < 0        :: output = 0
    // input   0 -  384 :: output   0 -  192        2x
    // input 384 -  768 :: output 192 -  576        1x (both are 384)
    // input 768 - 1023 :: output 576 - 1023         255:447 = .57...
    // input > 1023     :: output = 1023

    // all the dividing by 1023 is to convert the above Tasmota numbers to floats between 0 and 1.
    float i1 = 384.0f/1023.0f;
    float i2 = 768.0f/1023.0f;
    float o1 = 192.0f/1023.0f;
    float o2 = 576.0f/1023.0f;    

    if ( (x >= 0.0f) && (x <= i1) ) {
      return x*(o1/i1);  
    }
    else if ( x <= i2 ) {
      // compared to first section, everything has to subtract i1 or o1 to shift baseline down to 0 before doing the math
      // and then o1 gets added back at the end to shift back up to the second range.
      return ( (x-i1)*(o2-o1)/(i2-i1) + o1 );
    }
    else if ( x <= 1.0f ) {
      // compared to second section, just use i2/o2 instead of i1/o1
      return ( (x-i2)*(1.0f-o2)/(1.0f-i2) + o2 );
    }
    else {return 0.0f;}
  }


  static float kauf_gamma_rev(float x) {

    // to reverse, same function but switch inputs and outputs.
    float o1 = 384.0f/1023.0f;
    float o2 = 768.0f/1023.0f;
    float i1 = 192.0f/1023.0f;
    float i2 = 576.0f/1023.0f;    

    //everything else is the same
    if ( (x >= 0.0f) && (x <= i1) ) { return    x    *(  o1    /   i1); }
    else if ( x <= i2 ) {             return ( (x-i1)*(  o2-o1)/(  i2-i1) + o1 ); }
    else if ( x <= 1.0f ) {           return ( (x-i2)*(1.0f-o2)/(1.0f-i2) + o2 );
    } else { return 0.0f; }

  }


  bool changing_color_mode_{false};
  LightColorValues end_values_{};
  LightColorValues intermediate_values_{};
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
  uint32_t transition_length_;
  std::unique_ptr<LightTransformer> transformer_{nullptr};
  bool begun_lightstate_restore_;
};

}  // namespace light
}  // namespace esphome
