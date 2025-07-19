from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import mi_ns, CONF_MI_ID, Mi

DEPENDENCIES = ["mi"]
CODEOWNERS = ["@hencou"]

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

COMMANDS = {
    'unpair' : '{"command":"unpair"}',
    'pair' : '{"command":"pair"}',
    'set_white' : '{"command":"set_white"}',
    'night_mode' : '{"command":"night_mode"}',
    'level_up' : '{"command":"level_up"}',
    'level_down' : '{"command":"level_down"}',
    'temperature_up' : '{"command":"temperature_up"}',
    'temperature_down' : '{"command":"temperature_down"}',
    'next_mode' : '{"command":"next_mode"}',
    'previous_mode' : '{"command":"previous_mode"}',
    'mode_speed_down' : '{"command":"mode_speed_down"}',
    'mode_speed_up' : '{"command":"mode_speed_up"}',
    'toggle' : '{"command":"toggle"}',
}

CODEOWNERS = ["@hencou"]
CONF_DEVICEID = "mi_device_id"
CONF_GROUPID = "group_id"
CONF_REMOTETYPE = "remote_type"
CONF_COMMAND = "command"

MiSwitch = mi_ns.class_("MiSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.switch_schema(MiSwitch).extend(
    {
        cv.GenerateID(): cv.declare_id(MiSwitch),
        cv.GenerateID(CONF_MI_ID): cv.use_id(Mi),
        cv.Required(CONF_DEVICEID): cv.uint16_t,
        cv.Required(CONF_GROUPID): cv.uint8_t,
        cv.Required(CONF_REMOTETYPE): cv.enum(REMOTE_TYPES),
        cv.Required(CONF_COMMAND): cv.enum(COMMANDS),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    paren = await cg.get_variable(config[CONF_MI_ID])
    cg.add(var.set_mi_parent(paren))
    
    cg.add(var.set_switch(config[CONF_DEVICEID], config[CONF_GROUPID], config[CONF_REMOTETYPE], config[CONF_COMMAND]))
