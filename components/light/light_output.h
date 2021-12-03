#pragma once

#include "esphome/core/component.h"
#include "light_traits.h"
#include "light_state.h"
#include "light_transformer.h"

namespace esphome {
namespace light {

/// Interface to write LightStates to hardware.
class LightOutput {
 public:
  /// Return the LightTraits of this LightOutput.
  virtual LightTraits get_traits() = 0;

  /// Return the default transformer used for transitions.
  virtual std::unique_ptr<LightTransformer> create_default_transition();

  virtual void setup_state(LightState *state) {}

  /// Called on every update of the current values of the associated LightState,
  /// can optionally be used to do processing of this change.
  virtual void update_state(LightState *state) {}

  /// Called from loop() every time the light state has changed, and should
  /// should write the new state to hardware. Every call to write_state() is
  /// preceded by (at least) one call to update_state().
  virtual void write_state(LightState *state) = 0;

  // pointers to aux lights
  light::LightState *warm_rgb;
  light::LightState *cold_rgb;

  // lets other logic know whether the aux pointers are actually valid.  easier than actually learning c++.
  // this way we never try to access aux lights that belong to aux lights and crash the firmware.
  bool has_cw_rgb = false;

  // toggle for whether bulb should monitor for UDP packets from WLED / DDP
  bool use_wled = false;

};

}  // namespace light
}  // namespace esphome
