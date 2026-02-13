import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output
from esphome.core import CORE
from esphome.const import (
    CONF_BLUE,
    CONF_COLOR_INTERLOCK,
    CONF_CONSTANT_BRIGHTNESS,
    CONF_GREEN,
    CONF_RED,
    CONF_OUTPUT_ID,
    CONF_COLD_WHITE,
    CONF_WARM_WHITE,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_FREQUENCY,
    CONF_ID,
)

kauf_rgbww_ns = cg.esphome_ns.namespace('kauf_rgbww')
KaufRGBWWLight = kauf_rgbww_ns.class_('KaufRGBWWLight', light.LightOutput)

def get_pwm_steps_for_output(output_id):
    """Look up output frequency and return PWM steps (1,000,000 / frequency)."""
    for out_conf in CORE.config.get("output", []):
        out_id = out_conf.get(CONF_ID)
        if out_id and out_id.id == output_id:
            freq = out_conf.get(CONF_FREQUENCY, 1000.0)
            return int(1000000 / freq)
    return 1000  # fallback default

def validate_kauf_light(value):
    is_main = value["aux"] in (False, "main")

    if not is_main:
        if ( "red" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a red PWM output.")
        if ( "green" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a green PWM output.")
        if ( "blue" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a blue PWM output.")
        if ( "warm_white" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a warm_white PWM output.")
        if ( "cold_white" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a cold_white PWM output.")
        if ( "warm_rgb" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a warm_rgb light.")
        if ( "cold_rgb" in value ):
            raise cv.Invalid("Aux KAUF Light should not have a cold_rgb light.")
        if value["aux"] in ("warm", "cold") and "main_light" not in value:
            raise cv.Invalid("Aux KAUF Light with aux: warm/cold requires a main_light.")
        if value["aux"] is True and "main_light" in value:
            raise cv.Invalid("Use aux: warm or aux: cold instead of aux: true when specifying main_light.")

    else:
        if ( "red" not in value ):
            raise cv.Invalid("Main KAUF Light requires a red PWM output.")
        if ( "green" not in value ):
            raise cv.Invalid("Main KAUF Light requires a green PWM output.")
        if ( "blue" not in value ):
            raise cv.Invalid("Main KAUF Light requires a blue PWM output.")
        if ( "warm_white" not in value ):
            raise cv.Invalid("Main KAUF Light requires a warm_white PWM output.")
        if ( "cold_white" not in value ):
            raise cv.Invalid("Main KAUF Light requires a cold_white PWM output.")

    return value


CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(KaufRGBWWLight),
            cv.Optional(CONF_RED): cv.use_id(output.FloatOutput),
            cv.Optional(CONF_GREEN): cv.use_id(output.FloatOutput),
            cv.Optional(CONF_BLUE): cv.use_id(output.FloatOutput),
            cv.Optional(CONF_COLD_WHITE): cv.use_id(output.FloatOutput),
            cv.Optional(CONF_WARM_WHITE): cv.use_id(output.FloatOutput),
            cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
            cv.Optional(CONF_COLOR_INTERLOCK, default=False): cv.boolean,
            cv.Optional("cold_rgb"): cv.use_id(light.LightState),
            cv.Optional("warm_rgb"): cv.use_id(light.LightState),
            cv.Optional("aux", default=False): cv.Any(cv.boolean, cv.one_of("main", "warm", "cold", lower=True)),
            cv.Optional("main_light"): cv.use_id(light.LightState),
        }
    ),
    cv.has_none_or_all_keys(
        [CONF_COLD_WHITE_COLOR_TEMPERATURE, CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ),
    light.validate_color_temperature_channels,
    validate_kauf_light
)


async def to_code(config):

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])

    # for main light
    if config["aux"] in (False, "main") :

        # set cold and warm rgb lights if configured
        if "cold_rgb" in config and "warm_rgb" in config:
            crgb = await cg.get_variable(config["cold_rgb"])
            cg.add(var.set_cold_rgb(crgb))
            wrgb = await cg.get_variable(config["warm_rgb"])
            cg.add(var.set_warm_rgb(wrgb))
            cg.add_define("KAUF_HAS_AUX")

        # then set aux false after rgb light pointers set
        cg.add(var.set_aux(False))

        # set min/max color temp
        if CONF_WARM_WHITE_COLOR_TEMPERATURE in config:
            cg.add(var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
        if CONF_COLD_WHITE_COLOR_TEMPERATURE in config:
            cg.add(var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))


        # add RGBWW PWM, but wait until they exist using await
        red = await cg.get_variable(config[CONF_RED])
        cg.add(var.set_red(red))
        green = await cg.get_variable(config[CONF_GREEN])
        cg.add(var.set_green(green))
        blue = await cg.get_variable(config[CONF_BLUE])
        cg.add(var.set_blue(blue))
        cwhite = await cg.get_variable(config[CONF_COLD_WHITE])
        cg.add(var.set_cold_white(cwhite))
        wwhite = await cg.get_variable(config[CONF_WARM_WHITE])
        cg.add(var.set_warm_white(wwhite))

        # Calculate PWM steps for each channel based on output frequency
        # steps = 1,000,000 / frequency (e.g., 125Hz -> 8000 steps, 1000Hz -> 1000 steps)
        cg.add_define("KAUF_PWM_STEPS_RED", get_pwm_steps_for_output(config[CONF_RED].id))
        cg.add_define("KAUF_PWM_STEPS_GREEN", get_pwm_steps_for_output(config[CONF_GREEN].id))
        cg.add_define("KAUF_PWM_STEPS_BLUE", get_pwm_steps_for_output(config[CONF_BLUE].id))
        cg.add_define("KAUF_PWM_STEPS_COLD", get_pwm_steps_for_output(config[CONF_COLD_WHITE].id))
        cg.add_define("KAUF_PWM_STEPS_WARM", get_pwm_steps_for_output(config[CONF_WARM_WHITE].id))

    # register light
    light_state = await light.register_light(var, config)

    # aux light registers itself with the main light
    if config["aux"] in ("warm", "cold") and "main_light" in config:
        ml = await cg.get_variable(config["main_light"])
        cg.add(var.set_main_light(ml))
        ls = await cg.get_variable(config[CONF_ID])
        if config["aux"] == "warm":
            cg.add(cg.RawExpression(f"{ml}->get_output()->set_warm_rgb({ls})"))
        else:
            cg.add(cg.RawExpression(f"{ml}->get_output()->set_cold_rgb({ls})"))
        cg.add_define("KAUF_HAS_AUX")
