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
    // main light sets itself on aux lights so they can wake it up when they change
    if (!this->is_aux()) {
        this->warm_rgb->get_output()->set_main_light(state);
        this->cold_rgb->get_output()->set_main_light(state);
    }
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

    // light bulb is off, all levels 0.  Don't need ct.
    else if ( !state->current_values.is_on() ) {

        white_brightness = 0.0f;
        red = 0.0f;
        green = 0.0f;
        blue = 0.0f;

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


    // get minimum of input rgb values for blending into white
    float min_val;
    if ( (red <= green) && (red <= blue) ) { min_val = red;   } else
    if ( green <= blue )                   { min_val = green; } else
                                           { min_val = blue;  }


    float scaled_red, scaled_green, scaled_blue, scaled_warm, scaled_cold;

    // see whether aux light are both off so we can do a simplified path
    if (!warm_rgb->current_values.is_on() && !cold_rgb->current_values.is_on()) {

        scaled_red   =  red   - min_val;
        scaled_green =  green - min_val;
        scaled_blue  = (blue  - min_val) * max_blue;

        float white_blend = (min_val * max_white) + white_brightness;
        scaled_warm = white_blend * ct;
        scaled_cold = white_blend * (1.0f - ct);
       
    }

    else {

        // grab values from aux lights.  defaults are rgb all 0 and white maxed out at 1.0.
        float warm_red=0.0f, warm_green=0.0f, warm_blue=0.0f, warm_white=1.0f;
        float cold_red=0.0f, cold_green=0.0f, cold_blue=0.0f, cold_white=1.0f;

        // if aux light is on, get its values
        if ( warm_rgb->current_values.is_on() ) { warm_rgb->current_values_as_rgbw(&warm_red, &warm_green, &warm_blue, &warm_white); }
        if ( cold_rgb->current_values.is_on() ) { cold_rgb->current_values_as_rgbw(&cold_red, &cold_green, &cold_blue, &cold_white); }


        // ESP_LOGV("Kauf Light", "Input RGBW: - R:%f G:%f B:%f W:%f CT:%f)", red, green, blue, white_brightness, ct);
        // ESP_LOGV("Kauf Light", " Warm RGBW: - R:%f G:%f B:%f W:%f)", warm_red, warm_green, warm_blue, warm_white);
        // ESP_LOGV("Kauf Light", " Cold RGBW: - R:%f G:%f B:%f W:%f)", cold_red, cold_green, cold_blue, cold_white);

        // calculate output values:
        //                    color in
        //                    |       reduced by amount going to white blend
        //                    |       |          add cold_rgb scaled to white brightness and color temp
        //                    |       |          |                                             add warm_rgb scaled to white brightness and color temp
        //                    |       |          |                                             |                                       reduce blue to make RGB more accurate
        scaled_red   = clamp( red   - min_val + (cold_red   * white_brightness * (1.0f-ct)) + (warm_red   * white_brightness * ct) /*  |   */ , 0.0f, 1.0f);
        scaled_green = clamp( green - min_val + (cold_green * white_brightness * (1.0f-ct)) + (warm_green * white_brightness * ct) /*  |   */ , 0.0f, 1.0f);
        scaled_blue  = clamp((blue  - min_val + (cold_blue  * white_brightness * (1.0f-ct)) + (warm_blue  * white_brightness * ct)) * max_blue, 0.0f, 1.0f);

        //                      white blend amount, scale with max white since 100% white is too powerful for RGB colors
        //                      |                     white brightness, scale with aux white in case aux light indicates to turn down white channel
        //                      |                     |                                     scale both previous values per color temp
        scaled_warm  = clamp( ((min_val*max_white) + (white_brightness*warm_white)) *       ct , 0.0f, 1.0f);
        scaled_cold  = clamp( ((min_val*max_white) + (white_brightness*cold_white)) * (1.0f-ct), 0.0f, 1.0f);

    }

    // round up to the nearest PWM step to prevent near-zero values from rounding to zero
    // this might get moved into the warm_rgb/cold_rgb path because the simplified path just uses 
    // values directly from the esphome light component more-or-less.   If someone sets a very low
    // brightness on the simplified path, it's probably intentional and close to a "real" value,
    // not a floating point artifact.  Keepin it here for now because I think the ct multiplication
    // does end up giving some pretty low white values.

    // I could also see removing these, but there were some complaints that long fades were turning off
    // way too soon because the values rounded down to zero.
    scaled_red   = ceil(scaled_red   * KAUF_PWM_STEPS_RED  ) / KAUF_PWM_STEPS_RED;
    scaled_green = ceil(scaled_green * KAUF_PWM_STEPS_GREEN) / KAUF_PWM_STEPS_GREEN;
    scaled_blue  = ceil(scaled_blue  * KAUF_PWM_STEPS_BLUE ) / KAUF_PWM_STEPS_BLUE;
    scaled_cold  = ceil(scaled_cold  * KAUF_PWM_STEPS_COLD ) / KAUF_PWM_STEPS_COLD;
    scaled_warm  = ceil(scaled_warm  * KAUF_PWM_STEPS_WARM ) / KAUF_PWM_STEPS_WARM;


    ESP_LOGV("Kauf Light", "Setting Levels - R:%f G:%f B:%f CW:%f WW:%f)", scaled_red, scaled_green, scaled_blue, scaled_cold, scaled_warm);

    // set outputs
    this->red_->set_level(scaled_red);
    this->green_->set_level(scaled_green);
    this->blue_->set_level(scaled_blue);
    this->cold_white_->set_level(scaled_cold);
    this->warm_white_->set_level(scaled_warm);

}

} //namespace esphome::kauf_rgbww
