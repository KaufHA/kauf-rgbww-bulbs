import enum

import esphome.automation as auto
import esphome.codegen as cg
from esphome.components import mqtt, power_supply, web_server
import esphome.config_validation as cv
from esphome.const import (
    CONF_BLUE,
    CONF_BRIGHTNESS,
    CONF_COLD_WHITE,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_COLOR_BRIGHTNESS,
    CONF_COLOR_CORRECT,
    CONF_COLOR_MODE,
    CONF_COLOR_TEMPERATURE,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_EFFECTS,
    CONF_ENTITY_CATEGORY,
    CONF_FLASH_TRANSITION_LENGTH,
    CONF_GAMMA_CORRECT,
    CONF_GREEN,
    CONF_ICON,
    CONF_ID,
    CONF_INITIAL_STATE,
    CONF_MQTT_ID,
    CONF_ON_STATE,
    CONF_ON_TURN_OFF,
    CONF_ON_TURN_ON,
    CONF_OUTPUT_ID,
    CONF_POWER_SUPPLY,
    CONF_RED,
    CONF_RESTORE_MODE,
    CONF_STATE,
    CONF_TRIGGER_ID,
    CONF_WARM_WHITE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_WEB_SERVER,
    CONF_WHITE,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.core.entity_helpers import entity_duplicate_validator, setup_entity
from esphome.cpp_generator import MockObjClass

from .automation import LIGHT_STATE_SCHEMA
from .effects import (
    ADDRESSABLE_EFFECTS,
    BINARY_EFFECTS,
    EFFECTS_REGISTRY,
    MONOCHROMATIC_EFFECTS,
    RGB_EFFECTS,
    validate_effects,
)
from .types import (  # noqa
    AddressableLight,
    AddressableLightState,
    ColorMode,
    LightOutput,
    LightState,
    LightStateRTCState,
    LightStateTrigger,
    LightTurnOffTrigger,
    LightTurnOnTrigger,
    light_ns,
)

CODEOWNERS = ["@esphome/core"]
IS_PLATFORM_COMPONENT = True

LightRestoreMode = light_ns.enum("LightRestoreMode")
RESTORE_MODES = {
    "RESTORE_DEFAULT_OFF": LightRestoreMode.LIGHT_RESTORE_DEFAULT_OFF,
    "RESTORE_DEFAULT_ON": LightRestoreMode.LIGHT_RESTORE_DEFAULT_ON,
    "ALWAYS_OFF": LightRestoreMode.LIGHT_ALWAYS_OFF,
    "ALWAYS_ON": LightRestoreMode.LIGHT_ALWAYS_ON,
    "RESTORE_INVERTED_DEFAULT_OFF": LightRestoreMode.LIGHT_RESTORE_INVERTED_DEFAULT_OFF,
    "RESTORE_INVERTED_DEFAULT_ON": LightRestoreMode.LIGHT_RESTORE_INVERTED_DEFAULT_ON,
    "RESTORE_AND_OFF": LightRestoreMode.LIGHT_RESTORE_AND_OFF,
    "RESTORE_AND_ON": LightRestoreMode.LIGHT_RESTORE_AND_ON,
}

LIGHT_SCHEMA = (
    cv.ENTITY_BASE_SCHEMA.extend(web_server.WEBSERVER_SORTING_SCHEMA)
    .extend(cv.MQTT_COMMAND_COMPONENT_SCHEMA)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(LightState),
            cv.OnlyWith(CONF_MQTT_ID, "mqtt"): cv.declare_id(
                mqtt.MQTTJSONLightComponent
            ),
            cv.Optional(CONF_RESTORE_MODE, default="ALWAYS_OFF"): cv.enum(
                RESTORE_MODES, upper=True, space="_"
            ),
            cv.Optional(CONF_ON_TURN_ON): auto.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LightTurnOnTrigger),
                }
            ),
            cv.Optional(CONF_ON_TURN_OFF): auto.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LightTurnOffTrigger),
                }
            ),
            cv.Optional(CONF_ON_STATE): auto.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LightStateTrigger),
                }
            ),
            cv.Optional(CONF_INITIAL_STATE): LIGHT_STATE_SCHEMA,
            cv.Optional("forced_hash"): cv.int_,
            cv.Optional("forced_addr"): cv.int_,
            cv.Optional("global_addr"): cv.use_id(globals),
        }
    )
)

LIGHT_SCHEMA.add_extra(entity_duplicate_validator("light"))

BINARY_LIGHT_SCHEMA = LIGHT_SCHEMA.extend(
    {
        cv.Optional(CONF_EFFECTS): validate_effects(BINARY_EFFECTS),
    }
)

BRIGHTNESS_ONLY_LIGHT_SCHEMA = LIGHT_SCHEMA.extend(
    {
        cv.Optional(CONF_GAMMA_CORRECT, default=2.8): cv.positive_float,
        cv.Optional(
            CONF_DEFAULT_TRANSITION_LENGTH, default="1s"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(
            CONF_FLASH_TRANSITION_LENGTH, default="0s"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_EFFECTS): validate_effects(MONOCHROMATIC_EFFECTS),
    }
)

RGB_LIGHT_SCHEMA = BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.Optional(CONF_EFFECTS): validate_effects(RGB_EFFECTS),
    }
)

ADDRESSABLE_LIGHT_SCHEMA = RGB_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(AddressableLightState),
        cv.Optional(CONF_EFFECTS): validate_effects(ADDRESSABLE_EFFECTS),
        cv.Optional(CONF_COLOR_CORRECT): cv.All(
            [cv.percentage], cv.Length(min=3, max=4)
        ),
        cv.Optional(CONF_POWER_SUPPLY): cv.use_id(power_supply.PowerSupply),
    }
)


class LightType(enum.IntEnum):
    """Light type enum."""

    BINARY = 0
    BRIGHTNESS_ONLY = 1
    RGB = 2
    ADDRESSABLE = 3


def light_schema(
    class_: MockObjClass,
    type_: LightType,
    *,
    entity_category: str = cv.UNDEFINED,
    icon: str = cv.UNDEFINED,
    default_restore_mode: str = cv.UNDEFINED,
) -> cv.Schema:
    schema = {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(class_),
    }

    for key, default, validator in [
        (CONF_ENTITY_CATEGORY, entity_category, cv.entity_category),
        (CONF_ICON, icon, cv.icon),
        (
            CONF_RESTORE_MODE,
            default_restore_mode,
            cv.enum(RESTORE_MODES, upper=True, space="_"),
        ),
    ]:
        if default is not cv.UNDEFINED:
            schema[cv.Optional(key, default=default)] = validator

    if type_ == LightType.BINARY:
        return BINARY_LIGHT_SCHEMA.extend(schema)
    if type_ == LightType.BRIGHTNESS_ONLY:
        return BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(schema)
    if type_ == LightType.RGB:
        return RGB_LIGHT_SCHEMA.extend(schema)
    if type_ == LightType.ADDRESSABLE:
        return ADDRESSABLE_LIGHT_SCHEMA.extend(schema)

    raise ValueError(f"Invalid light type: {type_}")


def validate_color_temperature_channels(value):
    if (
        CONF_COLD_WHITE_COLOR_TEMPERATURE in value
        and CONF_WARM_WHITE_COLOR_TEMPERATURE in value
        and value[CONF_COLD_WHITE_COLOR_TEMPERATURE]
        >= value[CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ):
        raise cv.Invalid(
            "Color temperature of the cold white channel must be colder than that of the warm white channel.",
            path=[CONF_COLD_WHITE_COLOR_TEMPERATURE],
        )
    return value


async def setup_light_core_(light_var, output_var, config):
    await setup_entity(light_var, config, "light")

    cg.add(light_var.set_restore_mode(config[CONF_RESTORE_MODE]))

    if (initial_state_config := config.get(CONF_INITIAL_STATE)) is not None:
        initial_state = LightStateRTCState(
            initial_state_config.get(CONF_COLOR_MODE, ColorMode.UNKNOWN),
            initial_state_config.get(CONF_STATE, False),
            initial_state_config.get(CONF_BRIGHTNESS, 1.0),
            initial_state_config.get(CONF_COLOR_BRIGHTNESS, 1.0),
            initial_state_config.get(CONF_RED, 1.0),
            initial_state_config.get(CONF_GREEN, 1.0),
            initial_state_config.get(CONF_BLUE, 1.0),
            initial_state_config.get(CONF_WHITE, 1.0),
            initial_state_config.get(CONF_COLOR_TEMPERATURE, 1.0),
            initial_state_config.get(CONF_COLD_WHITE, 1.0),
            initial_state_config.get(CONF_WARM_WHITE, 1.0),
        )
        cg.add(light_var.set_initial_state(initial_state))

    if (
        default_transition_length := config.get(CONF_DEFAULT_TRANSITION_LENGTH)
    ) is not None:
        cg.add(light_var.set_default_transition_length(default_transition_length))
    if (
        flash_transition_length := config.get(CONF_FLASH_TRANSITION_LENGTH)
    ) is not None:
        cg.add(light_var.set_flash_transition_length(flash_transition_length))
    if (gamma_correct := config.get(CONF_GAMMA_CORRECT)) is not None:
        cg.add(light_var.set_gamma_correct(gamma_correct))
    effects = await cg.build_registry_list(
        EFFECTS_REGISTRY, config.get(CONF_EFFECTS, [])
    )
    cg.add(light_var.add_effects(effects))

    for conf in config.get(CONF_ON_TURN_ON, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], light_var)
        await auto.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_TURN_OFF, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], light_var)
        await auto.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_STATE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], light_var)
        await auto.build_automation(trigger, [], conf)

    if (color_correct := config.get(CONF_COLOR_CORRECT)) is not None:
        cg.add(output_var.set_correction(*color_correct))

    if (power_supply_id := config.get(CONF_POWER_SUPPLY)) is not None:
        var_ = await cg.get_variable(power_supply_id)
        cg.add(output_var.set_power_supply(var_))

    if (mqtt_id := config.get(CONF_MQTT_ID)) is not None:
        mqtt_ = cg.new_Pvariable(mqtt_id, light_var)
        await mqtt.register_mqtt_component(mqtt_, config)

    if web_server_config := config.get(CONF_WEB_SERVER):
        await web_server.add_entity_config(light_var, web_server_config)

    # set forced hash if it exists
    if "forced_hash" in config:
        cg.add(light_var.set_forced_hash(config["forced_hash"]))

    if "forced_addr" in config:
        cg.add(light_var.set_forced_addr(config["forced_addr"]))

    if "global_addr" in config:
        ga = await cg.get_variable(config["global_addr"])
        cg.add(light_var.set_global_addr(ga))


async def register_light(output_var, config):
    light_var = cg.new_Pvariable(config[CONF_ID], output_var)
    cg.add(cg.App.register_light(light_var))
    CORE.register_platform_component("light", light_var)
    await cg.register_component(light_var, config)
    await setup_light_core_(light_var, output_var, config)


async def new_light(config, *args):
    output_var = cg.new_Pvariable(config[CONF_OUTPUT_ID], *args)
    await register_light(output_var, config)
    return output_var


@coroutine_with_priority(100.0)
async def to_code(config):
    cg.add_define("USE_LIGHT")
    cg.add_global(light_ns.using)
