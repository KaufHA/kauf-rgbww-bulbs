#include "esphome.h"

class KaufLightRGBAux : public Component, public LightOutput {
 public:

  void setup() override {
  }

  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::RGB_WHITE});
    return traits;
  }

  void write_state(LightState *state) override {

    // tells main light that the aux light has changed so refresh.
    state->has_changed = true;

    // Ignore straight brightness (always reset to max).
    // We just rely on separate color and white brightness sliders.
    state->current_values.set_brightness(1.0f);

  }

};