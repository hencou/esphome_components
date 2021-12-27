import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

mi_ns = cg.esphome_ns.namespace("mi")
Mi = mi_ns.class_("Mi", cg.Component)

CONF_CE_PIN = "ce_pin"
CONF_CSN_PIN = "csn_pin"
CONF_RESET_PIN = "reset_pin"
  
CONF_MI_ID = "mi_id"
CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(Mi),
      cv.Required(CONF_CE_PIN): pins.gpio_output_pin_schema,
      cv.Required(CONF_CSN_PIN): pins.gpio_output_pin_schema,
      cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add_library("SPI", None)
    cg.add_library("RF24", None)
    cg.add_library("PathVariableHandlers", None)
    cg.add_library("https://github.com/luisllamasbinaburo/Arduino-List", None)
    cg.add_library("bblanchon/ArduinoJson", None)
    
    ce_pin = await cg.gpio_pin_expression(config[CONF_CE_PIN])
    cg.add(var.set_ce_pin(ce_pin))
    csn_pin = await cg.gpio_pin_expression(config[CONF_CSN_PIN])
    cg.add(var.set_csn_pin(csn_pin))
    
    if CONF_RESET_PIN in config:
      reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
      cg.add(var.set_reset_pin(reset_pin))

