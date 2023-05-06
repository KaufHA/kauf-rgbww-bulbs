#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace kauf_rgbww {

class KaufRGBWWLight : public light::LightOutput, public Component {
 public:
  void setup() override;
  light::LightTraits get_traits() override;

  void set_red(output::FloatOutput *red) { red_ = red; }
  void set_green(output::FloatOutput *green) { green_ = green; }
  void set_blue(output::FloatOutput *blue) { blue_ = blue; }
  void set_cold_white(output::FloatOutput *cold_white) { cold_white_ = cold_white; }
  void set_warm_white(output::FloatOutput *warm_white) { warm_white_ = warm_white; }
  void set_cold_white_temperature(float cold_white_temperature) { this->min_mireds = cold_white_temperature; }
  void set_warm_white_temperature(float warm_white_temperature) { this->max_mireds = warm_white_temperature; }
  void set_constant_brightness(bool constant_brightness) { constant_brightness_ = constant_brightness; }
  void set_color_interlock(bool color_interlock) { color_interlock_ = color_interlock; }

  void write_state(light::LightState *state) override;
  void dump_config() override;

  void set_outputs(float red, float green, float blue, float white_brightness = 0.0f);

  void set_cold_rgb(light::LightState *cold_rgb_in) { cold_rgb = cold_rgb_in; }
  void set_warm_rgb(light::LightState *warm_rgb_in) { warm_rgb = warm_rgb_in; }


 protected:
  output::FloatOutput *red_;
  output::FloatOutput *green_;
  output::FloatOutput *blue_;
  output::FloatOutput *cold_white_;
  output::FloatOutput *warm_white_;
  bool constant_brightness_;
  bool color_interlock_{false};

  float min_mireds = 150.0f;
  float max_mireds = 350.0f;

  float max_white = .75; // applies only to rgb blending into white.  Color temp mode will still go to 1.0 in combination
  float max_blue  = .6;  // blue really overpowers red and green.  .6 scaling factor seems about right.

  float ct = .5;         // CT variable declared up here so that it gets saved across calls to write_state.

};

} //namespace kauf_rgbww
} //namespace esphome