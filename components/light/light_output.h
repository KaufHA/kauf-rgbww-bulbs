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

  bool is_aux( ) {return aux;}
  void set_aux(bool aux_in) { aux = aux_in; }

  // pointers to aux lights
  light::LightState *warm_rgb;
  light::LightState *cold_rgb;

  bool aux = true;

  bool has_forced_hash = false;
  uint32_t forced_hash = 0;
  void set_forced_hash(uint32_t hash_value) {
    forced_hash = hash_value;
    has_forced_hash = true;
  }





};

}  // namespace light
}  // namespace esphome
