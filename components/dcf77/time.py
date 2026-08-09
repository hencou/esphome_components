from esphome import pins
import esphome.codegen as cg
from esphome.components import time as time_
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN

CODEOWNERS = ["@hencou"]
DEPENDENCIES = []

dcf77_ns = cg.esphome_ns.namespace("dcf77")
DCF77Time = dcf77_ns.class_("DCF77Time", time_.RealTimeClock)

CONFIG_SCHEMA = time_.TIME_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(DCF77Time),
        cv.Required(CONF_PIN): cv.All(pins.internal_gpio_input_pin_schema),
    }
).extend(cv.polling_component_schema("30min"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await time_.register_time(var, config)
    await cg.register_component(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))
