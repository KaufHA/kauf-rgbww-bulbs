import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import light, output
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
)

kauf_rgbww_ns = cg.esphome_ns.namespace('kauf_rgbww')
KaufRGBWWLight = kauf_rgbww_ns.class_('KaufRGBWWLight', light.LightOutput)

def validate_kauf_light(value):
    if (value["aux"]):
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
        if ( "warm_rgb" not in value ):
            raise cv.Invalid("Main KAUF Light requires a warm_rgb light.")
        if ( "cold_rgb" not in value ):
            raise cv.Invalid("Main KAUF Light requires a cold_rgb light.")

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
            cv.Optional("aux", default=False): cv.boolean,
        }
    ),
    cv.has_none_or_all_keys(
        [CONF_COLD_WHITE_COLOR_TEMPERATURE, CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ),
    light.validate_color_temperature_channels,
    validate_kauf_light
)


def final_validate(config):
    if ("esp8266" in fv.full_config.get()):
        esp8266_config = fv.full_config.get()["esp8266"]
        if ( ("start_free" in esp8266_config) and ("forced_addr" in config)):
            if ( (esp8266_config["start_free"] <= config["forced_addr"] + 11) ):
                start_free_num = esp8266_config["start_free"]
                forced_addr_num = config["forced_addr"]
                raise cv.Invalid(
                    f"Forced address ({forced_addr_num}) conflicts with esp8266: start_free ({start_free_num})"
                )
    else:
        if ("forced_addr" in config):
            raise cv.Invalid(
                "Forced_addr is only compatible with esp8266 platform"
            )

    if "forced_addr" in config and "global_addr" not in config:
        raise cv.Invalid(
            "Forced_addr requires global_addr"
        )

FINAL_VALIDATE_SCHEMA = cv.All(
    final_validate,
)


async def to_code(config):

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])

    # for main light
    if not config["aux"] :

        # set cold and warm rgb lights first
        crgb = await cg.get_variable(config["cold_rgb"])
        cg.add(var.set_cold_rgb(crgb))
        wrgb = await cg.get_variable(config["warm_rgb"])
        cg.add(var.set_warm_rgb(wrgb))

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

    # register light
    await light.register_light(var, config)
