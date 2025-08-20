#include <cinttypes>
#include "light_call.h"
#include "light_state.h"
#include "esphome/core/log.h"
#include "esphome/core/optional.h"

namespace esphome {
namespace light {

static const char *const TAG = "light";

// Helper functions to reduce code size for logging
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_WARN
static void log_validation_warning(const char *name, const char *param_name, float val, float min, float max) {
  ESP_LOGW(TAG, "'%s': %s value %.2f is out of range [%.1f - %.1f]", name, param_name, val, min, max);
}

static void log_feature_not_supported(const char *name, const char *feature) {
  ESP_LOGW(TAG, "'%s': %s not supported", name, feature);
}

static void log_color_mode_not_supported(const char *name, const char *feature) {
  ESP_LOGW(TAG, "'%s': color mode does not support setting %s", name, feature);
}

static void log_invalid_parameter(const char *name, const char *message) { ESP_LOGW(TAG, "'%s': %s", name, message); }
#else
#define log_validation_warning(name, param_name, val, min, max)
#define log_feature_not_supported(name, feature)
#define log_color_mode_not_supported(name, feature)
#define log_invalid_parameter(name, message)
#endif

// Macro to reduce repetitive setter code
#define IMPLEMENT_LIGHT_CALL_SETTER(name, type, flag) \
  LightCall &LightCall::set_##name(optional<type>(name)) { \
    if ((name).has_value()) { \
      this->name##_ = (name).value(); \
    } \
    this->set_flag_(flag, (name).has_value()); \
    return *this; \
  } \
  LightCall &LightCall::set_##name(type name) { \
    this->name##_ = name; \
    this->set_flag_(flag, true); \
    return *this; \
  }

static const LogString *color_mode_to_human(ColorMode color_mode) {
  if (color_mode == ColorMode::UNKNOWN)
    return LOG_STR("Unknown");
  if (color_mode == ColorMode::WHITE)
    return LOG_STR("White");
  if (color_mode == ColorMode::COLOR_TEMPERATURE)
    return LOG_STR("Color temperature");
  if (color_mode == ColorMode::COLD_WARM_WHITE)
    return LOG_STR("Cold/warm white");
  if (color_mode == ColorMode::RGB)
    return LOG_STR("RGB");
  if (color_mode == ColorMode::RGB_WHITE)
    return LOG_STR("RGBW");
  if (color_mode == ColorMode::RGB_COLD_WARM_WHITE)
    return LOG_STR("RGB + cold/warm white");
  if (color_mode == ColorMode::RGB_COLOR_TEMPERATURE)
    return LOG_STR("RGB + color temperature");
  return LOG_STR("");
}

// Helper to log percentage values
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
static void log_percent(const char *name, const char *param, float value) {
  ESP_LOGD(TAG, "  %s: %.0f%%", param, value * 100.0f);
}
#else
#define log_percent(name, param, value)
#endif

void LightCall::perform() {
  const char *name = this->parent_->get_name().c_str();
  LightColorValues v = this->validate_();
  const bool publish = this->get_publish_();

  // determine if call is superfluous.  i.e., a second call when the light is still in a transition
  // for another call with the same target values.  Basically looking for if new values are same as
  // remote values already being reported to Home Assistant and if transition is still ongoing.
  // Can't figure out why Home Assistant is sending two calls in a row sometimes.  Screwing with fade.

  // don't do any checks if not in transition
  if ( this->parent_->is_transformer_active() ) {

    bool super = true;

    // is the effect being set/cleared?
    if (this->has_effect_())
    {
      super = false;
      ESP_LOGV("KAUF Transition Filter", "Want to set an effect @ index %ud", this->effect_);
    }

    // is color mode the same?
    else if ( v.get_color_mode() != this->parent_->remote_values.get_color_mode()) {
      super = false;
      ESP_LOGV("KAUF Transition Filter","Color Mode different");
    }

    // if color mode is RGB, are r, g, and b values the same?
    else if ( v.get_color_mode() == ColorMode::RGB) {

      // get old and new rgb values.  as_rgb function applies brightness for us.
      float new_r, new_g, new_b, old_r, old_g, old_b;
      v.as_rgb(&new_r,&new_g,&new_b);
      this->parent_->remote_values.as_rgb(&old_r,&old_g,&old_b);

      if      ( new_r != old_r ) {
        super = false;
        ESP_LOGV("KAUF Transition Filter","Red channel different");
      }
      else if ( new_g != old_g ) {
        super = false;
        ESP_LOGV("KAUF Transition Filter","Green channel different");
      }
      else if ( new_b != old_b ) {
        super = false;
        ESP_LOGV("KAUF Transition Filter","Blue channel different");
      }
      else {
        ESP_LOGV("KAUF Transition Filter","RGB values all the same");
      }
    }

    // if in CT mode, compare color temp and white brightness
    else if ( v.get_color_mode() == ColorMode::COLOR_TEMPERATURE) {

      // get old and new color temp and white brightness values.
      float new_ct, new_wb, old_ct, old_wb;
      v.as_ct(150,350, &new_ct, &new_wb);
      this->parent_->remote_values.as_ct(150,350, &old_ct, &old_wb);

      if      ( new_wb != old_wb ) {
        super = false;
        ESP_LOGV("KAUF Transition Filter","White Brightness different");
      }
      else if ( new_ct != old_ct ) {
        super = false;
        ESP_LOGV("KAUF Transition Filter","Color Temp different");
      }
      else {
        ESP_LOGV("KAUF Transition Filter","Color Temp values all the same");
      }
    }

    if ( super == true ) {
      ESP_LOGD("KAUF Transition Filter","Double light call detected, skipping second call while first is still ongoing");
      return; }

  }

  ESP_LOGV("KAUF Transition Filter","--------------------------done, going ahead with call");

  if (publish) {
    ESP_LOGD(TAG, "'%s' Setting:", name);

    // Only print color mode when it's being changed
    ColorMode current_color_mode = this->parent_->remote_values.get_color_mode();
    ColorMode target_color_mode = this->has_color_mode() ? this->color_mode_ : current_color_mode;
    if (target_color_mode != current_color_mode) {
      ESP_LOGD(TAG, "  Color mode: %s", LOG_STR_ARG(color_mode_to_human(v.get_color_mode())));
    }

    // Only print state when it's being changed
    bool current_state = this->parent_->remote_values.is_on();
    bool target_state = this->has_state() ? this->state_ : current_state;
    if (target_state != current_state) {
      ESP_LOGD(TAG, "  State: %s", ONOFF(v.is_on()));
    }

    if (this->has_brightness()) {
      log_percent(name, "Brightness", v.get_brightness());
    }

    if (this->has_color_brightness()) {
      log_percent(name, "Color brightness", v.get_color_brightness());
    }
    if (this->has_red() || this->has_green() || this->has_blue()) {
      ESP_LOGD(TAG, "  Red: %.0f%%, Green: %.0f%%, Blue: %.0f%%", v.get_red() * 100.0f, v.get_green() * 100.0f,
               v.get_blue() * 100.0f);
    }

    if (this->has_white()) {
      log_percent(name, "White", v.get_white());
    }
    if (this->has_color_temperature()) {
      ESP_LOGD(TAG, "  Color temperature: %.1f mireds", v.get_color_temperature());
    }

    if (this->has_cold_white() || this->has_warm_white()) {
      ESP_LOGD(TAG, "  Cold white: %.0f%%, warm white: %.0f%%", v.get_cold_white() * 100.0f,
               v.get_warm_white() * 100.0f);
    }
  }

  if (this->has_flash_()) {
    // FLASH
    if (publish) {
      ESP_LOGD(TAG, "  Flash length: %.1fs", this->flash_length_ / 1e3f);
    }

    this->parent_->start_flash_(v, this->flash_length_, publish);
  } else if (this->has_transition_()) {
    // TRANSITION
    if (publish) {
      ESP_LOGD(TAG, "  Transition length: %.1fs", this->transition_length_ / 1e3f);
    }

    // Special case: Transition and effect can be set when turning off
    if (this->has_effect_()) {
      if (publish) {
        ESP_LOGD(TAG, "  Effect: 'None'");
      }
      this->parent_->stop_effect_();
    }

    this->parent_->start_transition_(v, this->transition_length_, publish);

  } else if (this->has_effect_()) {
    // EFFECT
    const char *effect_s;
    if (this->effect_ == 0u) {
      effect_s = "None";
    } else {
      effect_s = this->parent_->effects_[this->effect_ - 1]->get_name().c_str();
    }

    if (publish) {
      ESP_LOGD(TAG, "  Effect: '%s'", effect_s);
    }

    this->parent_->start_effect_(this->effect_);

    // Also set light color values when starting an effect
    // For example to turn off the light
    this->parent_->set_immediately_(v, true);
  } else {
    // INSTANT CHANGE
    this->parent_->set_immediately_(v, publish);
  }

  if (!this->has_transition_()) {
    this->parent_->target_state_reached_callback_.call();
  }
  if (publish) {
    this->parent_->publish_state();
  }
  if (this->get_save_()) {
    this->parent_->save_remote_values_();
  }
}

LightColorValues LightCall::validate_() {
  auto *name = this->parent_->get_name().c_str();
  auto traits = this->parent_->get_traits();

  // Color mode check
  if (this->has_color_mode() && !traits.supports_color_mode(this->color_mode_)) {
    ESP_LOGW(TAG, "'%s' does not support color mode %s", name, LOG_STR_ARG(color_mode_to_human(this->color_mode_)));
    this->set_flag_(FLAG_HAS_COLOR_MODE, false);
  }

  // Ensure there is always a color mode set
  if (!this->has_color_mode()) {
    this->color_mode_ = this->compute_color_mode_();
    this->set_flag_(FLAG_HAS_COLOR_MODE, true);
  }
  auto color_mode = this->color_mode_;

  // Transform calls that use non-native parameters for the current mode.
  this->transform_parameters_();

  // Brightness exists check
  if (this->has_brightness() && this->brightness_ > 0.0f && !(color_mode & ColorCapability::BRIGHTNESS)) {
    log_feature_not_supported(name, "brightness");
    this->set_flag_(FLAG_HAS_BRIGHTNESS, false);
  }

  // Transition length possible check
  if (this->has_transition_() && this->transition_length_ != 0 && !(color_mode & ColorCapability::BRIGHTNESS)) {
    log_feature_not_supported(name, "transitions");
    this->set_flag_(FLAG_HAS_TRANSITION, false);
  }

  // Color brightness exists check
  if (this->has_color_brightness() && this->color_brightness_ > 0.0f && !(color_mode & ColorCapability::RGB)) {
    log_color_mode_not_supported(name, "RGB brightness");
    this->set_flag_(FLAG_HAS_COLOR_BRIGHTNESS, false);
  }

  // RGB exists check
  if ((this->has_red() && this->red_ > 0.0f) || (this->has_green() && this->green_ > 0.0f) ||
      (this->has_blue() && this->blue_ > 0.0f)) {
    if (!(color_mode & ColorCapability::RGB)) {
      log_color_mode_not_supported(name, "RGB color");
      this->set_flag_(FLAG_HAS_RED, false);
      this->set_flag_(FLAG_HAS_GREEN, false);
      this->set_flag_(FLAG_HAS_BLUE, false);
    }
  }

  // White value exists check
  if (this->has_white() && this->white_ > 0.0f &&
      !(color_mode & ColorCapability::WHITE || color_mode & ColorCapability::COLD_WARM_WHITE)) {
    log_color_mode_not_supported(name, "white value");
    this->set_flag_(FLAG_HAS_WHITE, false);
  }

  // Color temperature exists check
  if (this->has_color_temperature() &&
      !(color_mode & ColorCapability::COLOR_TEMPERATURE || color_mode & ColorCapability::COLD_WARM_WHITE)) {
    log_color_mode_not_supported(name, "color temperature");
    this->set_flag_(FLAG_HAS_COLOR_TEMPERATURE, false);
  }

  // Cold/warm white value exists check
  if ((this->has_cold_white() && this->cold_white_ > 0.0f) || (this->has_warm_white() && this->warm_white_ > 0.0f)) {
    if (!(color_mode & ColorCapability::COLD_WARM_WHITE)) {
      log_color_mode_not_supported(name, "cold/warm white value");
      this->set_flag_(FLAG_HAS_COLD_WHITE, false);
      this->set_flag_(FLAG_HAS_WARM_WHITE, false);
    }
  }

#define VALIDATE_RANGE_(name_, upper_name, min, max) \
  if (this->has_##name_()) { \
    auto val = this->name_##_; \
    if (val < (min) || val > (max)) { \
      log_validation_warning(name, LOG_STR_LITERAL(upper_name), val, (min), (max)); \
      this->name_##_ = clamp(val, (min), (max)); \
    } \
  }
#define VALIDATE_RANGE(name, upper_name) VALIDATE_RANGE_(name, upper_name, 0.0f, 1.0f)

  // Range checks
  VALIDATE_RANGE(brightness, "Brightness")
  VALIDATE_RANGE(color_brightness, "Color brightness")
  VALIDATE_RANGE(red, "Red")
  VALIDATE_RANGE(green, "Green")
  VALIDATE_RANGE(blue, "Blue")
  VALIDATE_RANGE(white, "White")
  VALIDATE_RANGE(cold_white, "Cold white")
  VALIDATE_RANGE(warm_white, "Warm white")
  VALIDATE_RANGE_(color_temperature, "Color temperature", traits.get_min_mireds(), traits.get_max_mireds())

  // Flag whether an explicit turn off was requested, in which case we'll also stop the effect.
  bool explicit_turn_off_request = this->has_state() && !this->state_;

  // Turn off when brightness is set to zero, and reset brightness (so that it has nonzero brightness when turned on).
  if (this->has_brightness() && this->brightness_ == 0.0f) {
    this->state_ = false;
    this->set_flag_(FLAG_HAS_STATE, true);
    this->brightness_ = 1.0f;
  }

  // Set color brightness to 100% if currently zero and a color is set.
  if (this->has_red() || this->has_green() || this->has_blue()) {
    if (!this->has_color_brightness() && this->parent_->remote_values.get_color_brightness() == 0.0f) {
      this->color_brightness_ = 1.0f;
      this->set_flag_(FLAG_HAS_COLOR_BRIGHTNESS, true);
    }
  }

  // Create color values for the light with this call applied.
  auto v = this->parent_->remote_values;
  if (this->has_color_mode())
    v.set_color_mode(this->color_mode_);
  if (this->has_state())
    v.set_state(this->state_);
  if (this->has_brightness())
    v.set_brightness(this->brightness_);
  if (this->has_color_brightness())
    v.set_color_brightness(this->color_brightness_);
  if (this->has_red())
    v.set_red(this->red_);
  if (this->has_green())
    v.set_green(this->green_);
  if (this->has_blue())
    v.set_blue(this->blue_);
  if (this->has_white())
    v.set_white(this->white_);
  if (this->has_color_temperature())
    v.set_color_temperature(this->color_temperature_);
  if (this->has_cold_white())
    v.set_cold_white(this->cold_white_);
  if (this->has_warm_white())
    v.set_warm_white(this->warm_white_);

  v.normalize_color();

  // Flash length check
  if (this->has_flash_() && this->flash_length_ == 0) {
    log_invalid_parameter(name, "flash length must be greater than zero");
    this->set_flag_(FLAG_HAS_FLASH, false);
  }

  // validate transition length/flash length/effect not used at the same time
  bool supports_transition = color_mode & ColorCapability::BRIGHTNESS;

  // If effect is already active, remove effect start
  if (this->has_effect_() && this->effect_ == this->parent_->active_effect_index_) {
    this->set_flag_(FLAG_HAS_EFFECT, false);
  }

  // validate effect index
  if (this->has_effect_() && this->effect_ > this->parent_->effects_.size()) {
    ESP_LOGW(TAG, "'%s': invalid effect index %" PRIu32, name, this->effect_);
    this->set_flag_(FLAG_HAS_EFFECT, false);
  }

  if (this->has_effect_() && (this->has_transition_() || this->has_flash_())) {
    log_invalid_parameter(name, "effect cannot be used with transition/flash");
    this->set_flag_(FLAG_HAS_TRANSITION, false);
    this->set_flag_(FLAG_HAS_FLASH, false);
  }

  if (this->has_flash_() && this->has_transition_()) {
    log_invalid_parameter(name, "flash cannot be used with transition");
    this->set_flag_(FLAG_HAS_TRANSITION, false);
  }

  if (!this->has_transition_() && !this->has_flash_() && (!this->has_effect_() || this->effect_ == 0) &&
      supports_transition) {
    // nothing specified and light supports transitions, set default transition length
    this->transition_length_ = this->parent_->default_transition_length_;
    this->set_flag_(FLAG_HAS_TRANSITION, true);
  }

  if (this->has_transition_() && this->transition_length_ == 0) {
    // 0 transition is interpreted as no transition (instant change)
    this->set_flag_(FLAG_HAS_TRANSITION, false);
  }

  if (this->has_transition_() && !supports_transition) {
    log_feature_not_supported(name, "transitions");
    this->set_flag_(FLAG_HAS_TRANSITION, false);
  }

  // If not a flash and turning the light off, then disable the light
  // Do not use light color values directly, so that effects can set 0% brightness
  // Reason: When user turns off the light in frontend, the effect should also stop
  bool target_state = this->has_state() ? this->state_ : v.is_on();
  if (!this->has_flash_() && !target_state) {
    if (this->has_effect_()) {
      log_invalid_parameter(name, "cannot start effect when turning off");
      this->set_flag_(FLAG_HAS_EFFECT, false);
    } else if (this->parent_->active_effect_index_ != 0 && explicit_turn_off_request) {
      // Auto turn off effect
      this->effect_ = 0;
      this->set_flag_(FLAG_HAS_EFFECT, true);
    }
  }

  // Disable saving for flashes
  if (this->has_flash_())
    this->set_flag_(FLAG_SAVE, false);

  return v;
}
void LightCall::transform_parameters_() {
  auto traits = this->parent_->get_traits();

  // Allow CWWW modes to be set with a white value and/or color temperature.
  // This is used in three cases in HA:
  // - CW/WW lights, which set the "brightness" and "color_temperature"
  // - RGBWW lights with color_interlock=true, which also sets "brightness" and
  //   "color_temperature" (without color_interlock, CW/WW are set directly)
  // - Legacy Home Assistant (pre-colormode), which sets "white" and "color_temperature"

  // Cache min/max mireds to avoid repeated calls
  const float min_mireds = traits.get_min_mireds();
  const float max_mireds = traits.get_max_mireds();

  if (((this->has_white() && this->white_ > 0.0f) || this->has_color_temperature()) &&  //
      (this->color_mode_ & ColorCapability::COLD_WARM_WHITE) &&                         //
      !(this->color_mode_ & ColorCapability::WHITE) &&                                  //
      !(this->color_mode_ & ColorCapability::COLOR_TEMPERATURE) &&                      //
      min_mireds > 0.0f && max_mireds > 0.0f) {
    ESP_LOGD(TAG, "'%s': setting cold/warm white channels using white/color temperature values",
             this->parent_->get_name().c_str());
    if (this->has_color_temperature()) {
      const float color_temp = clamp(this->color_temperature_, min_mireds, max_mireds);
      const float range = max_mireds - min_mireds;
      const float ww_fraction = (color_temp - min_mireds) / range;
      const float cw_fraction = 1.0f - ww_fraction;
      const float max_cw_ww = std::max(ww_fraction, cw_fraction);
      const float gamma = this->parent_->get_gamma_correct();
      this->cold_white_ = gamma_uncorrect(cw_fraction / max_cw_ww, gamma);
      this->warm_white_ = gamma_uncorrect(ww_fraction / max_cw_ww, gamma);
      this->set_flag_(FLAG_HAS_COLD_WHITE, true);
      this->set_flag_(FLAG_HAS_WARM_WHITE, true);
    }
    if (this->has_white()) {
      this->brightness_ = this->white_;
      this->set_flag_(FLAG_HAS_BRIGHTNESS, true);
    }
  }
}
ColorMode LightCall::compute_color_mode_() {
  auto supported_modes = this->parent_->get_traits().get_supported_color_modes();
  int supported_count = supported_modes.size();

  // Some lights don't support any color modes (e.g. monochromatic light), leave it at unknown.
  if (supported_count == 0)
    return ColorMode::UNKNOWN;

  // In the common case of lights supporting only a single mode, use that one.
  if (supported_count == 1)
    return *supported_modes.begin();

  // Don't change if the light is being turned off.
  ColorMode current_mode = this->parent_->remote_values.get_color_mode();
  if (this->has_state() && !this->state_)
    return current_mode;

  // If no color mode is specified, we try to guess the color mode. This is needed for backward compatibility to
  // pre-colormode clients and automations, but also for the MQTT API, where HA doesn't let us know which color mode
  // was used for some reason.
  std::set<ColorMode> suitable_modes = this->get_suitable_color_modes_();

  // Don't change if the current mode is suitable.
  if (suitable_modes.count(current_mode) > 0) {
    ESP_LOGI(TAG, "'%s': color mode not specified; retaining %s", this->parent_->get_name().c_str(),
             LOG_STR_ARG(color_mode_to_human(current_mode)));
    return current_mode;
  }

  // Use the preferred suitable mode.
  for (auto mode : suitable_modes) {
    if (supported_modes.count(mode) == 0)
      continue;

    ESP_LOGI(TAG, "'%s': color mode not specified; using %s", this->parent_->get_name().c_str(),
             LOG_STR_ARG(color_mode_to_human(mode)));
    return mode;
  }

  // There's no supported mode for this call, so warn, use the current more or a mode at random and let validation strip
  // out whatever we don't support.
  auto color_mode = current_mode != ColorMode::UNKNOWN ? current_mode : *supported_modes.begin();
  ESP_LOGW(TAG, "'%s': no suitable color mode supported; defaulting to %s", this->parent_->get_name().c_str(),
           LOG_STR_ARG(color_mode_to_human(color_mode)));
  return color_mode;
}
std::set<ColorMode> LightCall::get_suitable_color_modes_() {
  bool has_white = this->has_white() && this->white_ > 0.0f;
  bool has_ct = this->has_color_temperature();
  bool has_cwww =
      (this->has_cold_white() && this->cold_white_ > 0.0f) || (this->has_warm_white() && this->warm_white_ > 0.0f);
  bool has_rgb = (this->has_color_brightness() && this->color_brightness_ > 0.0f) ||
                 (this->has_red() || this->has_green() || this->has_blue());

// Build key from flags: [rgb][cwww][ct][white]
#define KEY(white, ct, cwww, rgb) ((white) << 0 | (ct) << 1 | (cwww) << 2 | (rgb) << 3)

  uint8_t key = KEY(has_white, has_ct, has_cwww, has_rgb);

  switch (key) {
    case KEY(true, false, false, false):  // white only
      return {ColorMode::WHITE, ColorMode::RGB_WHITE, ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::COLD_WARM_WHITE,
              ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(false, true, false, false):  // ct only
      return {ColorMode::COLOR_TEMPERATURE, ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::COLD_WARM_WHITE,
              ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(true, true, false, false):  // white + ct
      return {ColorMode::COLD_WARM_WHITE, ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(false, false, true, false):  // cwww only
      return {ColorMode::COLD_WARM_WHITE, ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(false, false, false, false):  // none
      return {ColorMode::RGB_WHITE, ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::RGB_COLD_WARM_WHITE, ColorMode::RGB,
              ColorMode::WHITE,     ColorMode::COLOR_TEMPERATURE,     ColorMode::COLD_WARM_WHITE};
    case KEY(true, false, false, true):  // rgb + white
      return {ColorMode::RGB_WHITE, ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(false, true, false, true):  // rgb + ct
    case KEY(true, true, false, true):   // rgb + white + ct
      return {ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(false, false, true, true):  // rgb + cwww
      return {ColorMode::RGB_COLD_WARM_WHITE};
    case KEY(false, false, false, true):  // rgb only
      return {ColorMode::RGB, ColorMode::RGB_WHITE, ColorMode::RGB_COLOR_TEMPERATURE, ColorMode::RGB_COLD_WARM_WHITE};
    default:
      return {};  // conflicting flags
  }

#undef KEY
}

LightCall &LightCall::set_effect(const std::string &effect) {
  if (strcasecmp(effect.c_str(), "none") == 0) {
    this->set_effect(0);
    return *this;
  }

  bool found = false;
  for (uint32_t i = 0; i < this->parent_->effects_.size(); i++) {
    LightEffect *e = this->parent_->effects_[i];

    if (strcasecmp(effect.c_str(), e->get_name().c_str()) == 0) {
      this->set_effect(i + 1);
      found = true;
      break;
    }
  }
  if (!found) {
    ESP_LOGW(TAG, "'%s': no such effect '%s'", this->parent_->get_name().c_str(), effect.c_str());
  }
  return *this;
}
LightCall &LightCall::from_light_color_values(const LightColorValues &values) {
  this->set_state(values.is_on());
  this->set_brightness_if_supported(values.get_brightness());
  this->set_color_brightness_if_supported(values.get_color_brightness());
  this->set_color_mode_if_supported(values.get_color_mode());
  this->set_red_if_supported(values.get_red());
  this->set_green_if_supported(values.get_green());
  this->set_blue_if_supported(values.get_blue());
  this->set_white_if_supported(values.get_white());
  this->set_color_temperature_if_supported(values.get_color_temperature());
  this->set_cold_white_if_supported(values.get_cold_white());
  this->set_warm_white_if_supported(values.get_warm_white());
  return *this;
}
ColorMode LightCall::get_active_color_mode_() {
  return this->has_color_mode() ? this->color_mode_ : this->parent_->remote_values.get_color_mode();
}
LightCall &LightCall::set_transition_length_if_supported(uint32_t transition_length) {
  if (this->get_active_color_mode_() & ColorCapability::BRIGHTNESS)
    this->set_transition_length(transition_length);
  return *this;
}
LightCall &LightCall::set_brightness_if_supported(float brightness) {
  if (this->get_active_color_mode_() & ColorCapability::BRIGHTNESS)
    this->set_brightness(brightness);
  return *this;
}
LightCall &LightCall::set_color_mode_if_supported(ColorMode color_mode) {
  if (this->parent_->get_traits().supports_color_mode(color_mode))
    this->set_color_mode(color_mode);
  return *this;
}
LightCall &LightCall::set_color_brightness_if_supported(float brightness) {
  if (this->get_active_color_mode_() & ColorCapability::RGB)
    this->set_color_brightness(brightness);
  return *this;
}
LightCall &LightCall::set_red_if_supported(float red) {
  if (this->get_active_color_mode_() & ColorCapability::RGB)
    this->set_red(red);
  return *this;
}
LightCall &LightCall::set_green_if_supported(float green) {
  if (this->get_active_color_mode_() & ColorCapability::RGB)
    this->set_green(green);
  return *this;
}
LightCall &LightCall::set_blue_if_supported(float blue) {
  if (this->get_active_color_mode_() & ColorCapability::RGB)
    this->set_blue(blue);
  return *this;
}
LightCall &LightCall::set_white_if_supported(float white) {
  if (this->get_active_color_mode_() & ColorCapability::WHITE)
    this->set_white(white);
  return *this;
}
LightCall &LightCall::set_color_temperature_if_supported(float color_temperature) {
  if (this->get_active_color_mode_() & ColorCapability::COLOR_TEMPERATURE ||
      this->get_active_color_mode_() & ColorCapability::COLD_WARM_WHITE)
    this->set_color_temperature(color_temperature);
  return *this;
}
LightCall &LightCall::set_cold_white_if_supported(float cold_white) {
  if (this->get_active_color_mode_() & ColorCapability::COLD_WARM_WHITE)
    this->set_cold_white(cold_white);
  return *this;
}
LightCall &LightCall::set_warm_white_if_supported(float warm_white) {
  if (this->get_active_color_mode_() & ColorCapability::COLD_WARM_WHITE)
    this->set_warm_white(warm_white);
  return *this;
}
IMPLEMENT_LIGHT_CALL_SETTER(state, bool, FLAG_HAS_STATE)
IMPLEMENT_LIGHT_CALL_SETTER(transition_length, uint32_t, FLAG_HAS_TRANSITION)
IMPLEMENT_LIGHT_CALL_SETTER(flash_length, uint32_t, FLAG_HAS_FLASH)
IMPLEMENT_LIGHT_CALL_SETTER(brightness, float, FLAG_HAS_BRIGHTNESS)
IMPLEMENT_LIGHT_CALL_SETTER(color_mode, ColorMode, FLAG_HAS_COLOR_MODE)
IMPLEMENT_LIGHT_CALL_SETTER(color_brightness, float, FLAG_HAS_COLOR_BRIGHTNESS)
IMPLEMENT_LIGHT_CALL_SETTER(red, float, FLAG_HAS_RED)
IMPLEMENT_LIGHT_CALL_SETTER(green, float, FLAG_HAS_GREEN)
IMPLEMENT_LIGHT_CALL_SETTER(blue, float, FLAG_HAS_BLUE)
IMPLEMENT_LIGHT_CALL_SETTER(white, float, FLAG_HAS_WHITE)
IMPLEMENT_LIGHT_CALL_SETTER(color_temperature, float, FLAG_HAS_COLOR_TEMPERATURE)
IMPLEMENT_LIGHT_CALL_SETTER(cold_white, float, FLAG_HAS_COLD_WHITE)
IMPLEMENT_LIGHT_CALL_SETTER(warm_white, float, FLAG_HAS_WARM_WHITE)
LightCall &LightCall::set_effect(optional<std::string> effect) {
  if (effect.has_value())
    this->set_effect(*effect);
  return *this;
}
LightCall &LightCall::set_effect(uint32_t effect_number) {
  this->effect_ = effect_number;
  this->set_flag_(FLAG_HAS_EFFECT, true);
  return *this;
}
LightCall &LightCall::set_effect(optional<uint32_t> effect_number) {
  if (effect_number.has_value()) {
    this->effect_ = effect_number.value();
  }
  this->set_flag_(FLAG_HAS_EFFECT, effect_number.has_value());
  return *this;
}
LightCall &LightCall::set_publish(bool publish) {
  this->set_flag_(FLAG_PUBLISH, publish);
  return *this;
}
LightCall &LightCall::set_save(bool save) {
  this->set_flag_(FLAG_SAVE, save);
  return *this;
}
LightCall &LightCall::set_rgb(float red, float green, float blue) {
  this->set_red(red);
  this->set_green(green);
  this->set_blue(blue);
  return *this;
}
LightCall &LightCall::set_rgbw(float red, float green, float blue, float white) {
  this->set_rgb(red, green, blue);
  this->set_white(white);
  return *this;
}

}  // namespace light
}  // namespace esphome
