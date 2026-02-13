#include "esphome/core/log.h"
#include "kauf_rgbww.h"

namespace esphome::kauf_rgbww {

static const char *TAG = "kauf_rgbww.light";

light::LightTraits KaufRGBWWLight::get_traits() {
    auto traits = light::LightTraits();

    if ( this->is_aux() ) {
        traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
    }
    else { // Main
        traits.set_min_mireds(min_mireds);
        traits.set_max_mireds(max_mireds);

        // RGB and Color Temperature are two separate color modes.  Not RGBCT as a single mode.
        traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
    }

    return traits;
}

void KaufRGBWWLight::setup_state(light::LightState *state) {
}

void KaufRGBWWLight::write_state(light::LightState *state) {

    if ( this->is_aux() ) {

        // tells main light that the aux light has changed so refresh.
        state->has_changed = true;

        // enable main light's loop so it can check the has_changed flag
        if (this->main_light != nullptr) {
            this->main_light->enable_loop();
        }

        ESP_LOGV("KAUF RGBWW","set has_changed to true");

        // Ignore straight brightness (always reset to max).
        // We just rely on separate color and white brightness sliders.
        state->current_values.set_brightness(1.0f);

        return;
    }

    float red, green, blue;
    float white_brightness;


    // get rgbww values.

    // use_raw means don't apply gamma. Due to being in a transition.
    // The KAUF custom transition applies a different gamma curve
    // within the transition so we don't want to apply more gamma.

    // use_raw is now also used with values received via WLED DDP.
    // Presumably WLED applies any desired gamma correction before sending out the values.
    if ( state->current_values.use_raw ) {

        // ESP_LOGD("Kauf Light", "Use Raw - Yes");
        // state->current_values.use_raw = false;

        state->current_values.as_ct(min_mireds, max_mireds, &ct, &white_brightness);
        red   = state->current_values.get_red();
        green = state->current_values.get_green();
        blue  = state->current_values.get_blue();

    }

    // light bulb is off, set all outputs to 0 and return early.
    else if ( !state->current_values.is_on() ) {

        this->red_->set_level(0.0f);
        this->green_->set_level(0.0f);
        this->blue_->set_level(0.0f);
        this->cold_white_->set_level(0.0f);
        this->warm_white_->set_level(0.0f);
        return;

    }

    // CT color mode.  all RGB zeros, get ct values.
    else if ( state->current_values.get_color_mode() & light::ColorCapability::COLOR_TEMPERATURE ) {

        state->current_values_as_ct(&ct, &white_brightness);
        red = 0.0f;
        green = 0.0f;
        blue = 0.0f;

    }

    // RGB color mode.  Get rgb values with default gamma.  No white channel in RGB mode.
    else {

        state->current_values_as_rgb(&red, &green, &blue);
        white_brightness = 0.0f;

    }

    float inv_ct = 1.0f - ct;

    // get minimum of input rgb values for blending into white
    float min_val;
    if ( (red <= green) && (red <= blue) ) { min_val = red;   } else
    if ( green <= blue )                   { min_val = green; } else
                                           { min_val = blue;  }


    float scaled_red, scaled_green, scaled_blue, scaled_warm, scaled_cold;
#ifdef KAUF_HAS_AUX
    float mw = min_val * max_white;
    float white_blend = mw + white_brightness;
#else
    float white_blend = (min_val * max_white) + white_brightness;
#endif

    // calculate output values:
    //   scaled RGB = color in, reduced by amount going to white blend
    scaled_red   = red   - min_val;
    scaled_green = green - min_val;
    scaled_blue  = blue  - min_val;

#ifdef KAUF_HAS_AUX
    bool warm_on = (warm_rgb != nullptr && warm_rgb->current_values.is_on());
    bool cold_on = (cold_rgb != nullptr && cold_rgb->current_values.is_on());

    //   if warm aux light is on, accumulate warm_rgb scaled to white brightness and color temp
    if ( warm_on ) {
        float warm_red, warm_green, warm_blue, warm_white;
        warm_rgb->current_values_as_rgbw(&warm_red, &warm_green, &warm_blue, &warm_white);
        float wb_warm = white_brightness * ct;
        scaled_red   += warm_red   * wb_warm;
        scaled_green += warm_green * wb_warm;
        scaled_blue  += warm_blue  * wb_warm;

        //   scaled_warm = white blend amount (scaled with max_white since 100% white is too powerful for RGB colors)
        //               + white brightness scaled by aux warm_white (in case aux light indicates to turn down white channel)
        //               * color temp to scale for warm channel
        scaled_warm = (mw + (white_brightness * warm_white)) * ct;
    } else
#endif
    //   warm aux off or absent: white blend (includes white brightness since warm_white defaults to 1.0) * color temp
    { scaled_warm = white_blend * ct; }

#ifdef KAUF_HAS_AUX
    //   if cold aux light is on, accumulate cold_rgb scaled to white brightness and color temp
    if ( cold_on ) {
        float cold_red, cold_green, cold_blue, cold_white;
        cold_rgb->current_values_as_rgbw(&cold_red, &cold_green, &cold_blue, &cold_white);
        float wb_cold = white_brightness * inv_ct;
        scaled_red   += cold_red   * wb_cold;
        scaled_green += cold_green * wb_cold;
        scaled_blue  += cold_blue  * wb_cold;

        //   scaled_cold = white blend amount (scaled with max_white since 100% white is too powerful for RGB colors)
        //               + white brightness scaled by aux cold_white (in case aux light indicates to turn down white channel)
        //               * inverse color temp to scale for cold channel
        scaled_cold = (mw + (white_brightness * cold_white)) * inv_ct;
    } else
#endif
    //   cold aux off or absent: white blend (includes white brightness since cold_white defaults to 1.0) * inverse color temp
    { scaled_cold = white_blend * inv_ct; }

    //   reduce blue to make RGB more accurate
    scaled_blue *= max_blue;

    // Round up to the nearest PWM step to prevent near-zero values from rounding to zero.
    // Without this, long fades turn off too soon because small values round down to zero.
    // Gated on > 0.0f to skip the ceil/multiply/divide for channels that are already zero
    // (e.g. all three RGB channels in CT mode, or both white channels in RGB mode with no
    // white blend).  The > 0.0f comparison is essentially free (integer bit check on the
    // float representation) while the ceil/multiply/divide are expensive on ESP8266 (no FPU).
    if (scaled_red   > 0.0f) scaled_red   = ceil(scaled_red   * KAUF_PWM_STEPS_RED  ) / KAUF_PWM_STEPS_RED;
    if (scaled_green > 0.0f) scaled_green = ceil(scaled_green * KAUF_PWM_STEPS_GREEN) / KAUF_PWM_STEPS_GREEN;
    if (scaled_blue  > 0.0f) scaled_blue  = ceil(scaled_blue  * KAUF_PWM_STEPS_BLUE ) / KAUF_PWM_STEPS_BLUE;
    if (scaled_cold  > 0.0f) scaled_cold  = ceil(scaled_cold  * KAUF_PWM_STEPS_COLD ) / KAUF_PWM_STEPS_COLD;
    if (scaled_warm  > 0.0f) scaled_warm  = ceil(scaled_warm  * KAUF_PWM_STEPS_WARM ) / KAUF_PWM_STEPS_WARM;


    ESP_LOGV("Kauf Light", "Setting Levels - R:%f G:%f B:%f CW:%f WW:%f)", scaled_red, scaled_green, scaled_blue, scaled_cold, scaled_warm);

    // set outputs
    this->red_->set_level(scaled_red);
    this->green_->set_level(scaled_green);
    this->blue_->set_level(scaled_blue);
    this->cold_white_->set_level(scaled_cold);
    this->warm_white_->set_level(scaled_warm);

}

} //namespace esphome::kauf_rgbww
