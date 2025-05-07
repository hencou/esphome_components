import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_OUTPUT,
    CONF_SCL,
    CONF_SDA,
)

i2c_sniffer_ns = cg.esphome_ns.namespace("i2c_sniffer")
I2cSniffer = i2c_sniffer_ns.class_("I2cSniffer", cg.Component)


pin_with_input_and_output_support = cv.All(
    pins.internal_gpio_pin_number({CONF_INPUT: True}),
    pins.internal_gpio_pin_number({CONF_OUTPUT: True}),
)

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(I2cSniffer),
      #cv.Optional(CONF_SDA, default="SDA"): pin_with_input_and_output_support,
      #cv.Optional(CONF_SCL, default="SCL"): pin_with_input_and_output_support,
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  
  #cg.add(var.set_sda_pin(config[CONF_SDA]))
  #cg.add(var.set_scl_pin(config[CONF_SCL]))
