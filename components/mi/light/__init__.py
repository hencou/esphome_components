import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import light, output
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_GAMMA_CORRECT,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
)
from .. import mi_ns, CONF_MI_ID, Mi

DEPENDENCIES = ["mi"]

REMOTE_TYPES = {
    "rgb_cct" : "rgb_cct",
    "cct" : "cct",
    "rgb" : "rgb",
    "rgbw" : "rgbw",
    "fut089" : "fut089",
    "fut091" : "fut091",
    "fut020" : "fut020",
    "s2" : "s2",
}


CODEOWNERS = ["@hencou"]
CONF_DEVICEID = "device_id"
CONF_GROUPID = "group_id"
CONF_REMOTETYPE = "remote_type"

MiLight = mi_ns.class_("MiLight", light.LightOutput, cg.Component)

CONFIG_SCHEMA = cv.All(
    light.light_schema(MiLight, light.LightType.RGB).extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MiLight),
            cv.GenerateID(CONF_MI_ID): cv.use_id(Mi), 
            cv.Required(CONF_DEVICEID): cv.uint16_t,
            cv.Required(CONF_GROUPID): cv.uint8_t,
            cv.Required(CONF_REMOTETYPE): cv.enum(REMOTE_TYPES),
            
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="200ms"
            ): cv.positive_time_period_milliseconds,

            cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE, default="153 mireds"): cv.color_temperature,
            cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE, default="370 mireds"): cv.color_temperature,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    light.validate_color_temperature_channels,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    
    paren = await cg.get_variable(config[CONF_MI_ID])
    cg.add(var.set_mi_parent(paren))
    
    cg.add(var.set_bulb_id(config[CONF_DEVICEID], config[CONF_GROUPID], config[CONF_REMOTETYPE]))

    cg.add(var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))
    cg.add(var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
   
