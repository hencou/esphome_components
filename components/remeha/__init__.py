import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.canbus import CanbusComponent, CONF_CANBUS_ID
from esphome.const import CONF_ID

CODEOWNERS = ["@hencou"]
DEPENDENCIES = ["canbus"]

remeha_ns = cg.esphome_ns.namespace("remeha")
Remeha = remeha_ns.class_("Remeha", cg.Component)

CONF_REMEHA_ID = "remeha_id"
CONF_BOOT_DELAY = "boot_delay"
CONF_USER_LEVEL = "user_level"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Remeha),
        cv.Required(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Optional(CONF_BOOT_DELAY, default="10s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_USER_LEVEL, default=2): cv.int_range(min=1, max=3),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    canbus = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus))

    cg.add(var.set_boot_delay(config[CONF_BOOT_DELAY]))
    cg.add(var.set_user_level(config[CONF_USER_LEVEL]))
