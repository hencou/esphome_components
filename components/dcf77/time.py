from esphome.components import time as time_
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import pins
from esphome.core import CORE
from esphome.const import CONF_ID, CONF_SERVERS, CONF_PIN


dcf77_ns = cg.esphome_ns.namespace("dcf77")
DCF77Component = dcf77_ns.class_("DCF77Component", time_.RealTimeClock)

CONFIG_SCHEMA = time_.TIME_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(DCF77Component),
        cv.Required(CONF_PIN): pins.gpio_output_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await time_.register_time(var, config)
    
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))

    cg.add_library("https://github.com/hencou/Arduino-DCF77", None)
    cg.add_library("https://github.com/PaulStoffregen/Time", None)