import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, pins
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_OUTPUT,
    CONF_SCL,
    CONF_SDA,
)

try:
    from esphome.components.esp32 import include_builtin_idf_component
except ImportError:
    include_builtin_idf_component = None

AUTO_LOAD = ["json"]

itho_ns = cg.esphome_ns.namespace("itho")
Itho = itho_ns.class_("Itho", cg.Component)

SYSSHT30_VALUES = {
  "enable" : 2,
  "disable" : 1,
}

REMOTE_TYPES = {
  "RFTCVE": 0x22F1,
  "RFTAUTO": 0x22F3,
  "DEMANDFLOW": 0x22F8,
  "RFTRV": 0x12A0,
  "RFTCO2": 0x1298,
}

CONF_SYSSHT30_VALUE = "syssht30"
CONF_SYSSHT30_ADDRESS = "syssht30_address"
CONF_REMOTE_TYPE = "remote_type"
CONF_STATUS_INPUT_PIN = "status_input_pin"
CONF_STATUS_OUTPUT_PIN = "status_output_pin"

CONF_ITHO_ID = "itho_id"
CONFIG_SCHEMA = (
  cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Itho),
        cv.Optional(CONF_SYSSHT30_VALUE) : cv.enum(SYSSHT30_VALUES),
        cv.Optional(CONF_SYSSHT30_ADDRESS) : cv.i2c_address,
        cv.Optional(CONF_SDA, default="SDA"): pins.internal_gpio_pin_number,
        cv.Optional(CONF_SCL, default="SCL"): pins.internal_gpio_pin_number,
        cv.Optional(CONF_REMOTE_TYPE, default="RFTCVE"): cv.enum(REMOTE_TYPES, upper=True),
        cv.Optional(CONF_STATUS_INPUT_PIN, default=16): pins.internal_gpio_pin_number,
        cv.Optional(CONF_STATUS_OUTPUT_PIN, default=13): pins.internal_gpio_pin_number,
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  
  if core.CORE.using_arduino:
    cg.add_library("Ticker", None)

  if core.CORE.is_esp32 and include_builtin_idf_component is not None:
    include_builtin_idf_component("driver")

  if CONF_SYSSHT30_VALUE in config:
    cg.add(var.setSysSHT30(config[CONF_SYSSHT30_VALUE]))
    
  if CONF_SYSSHT30_ADDRESS in config:
    cg.add(var.setSysSHT30_Address(config[CONF_SYSSHT30_ADDRESS]))
    
  cg.add(var.setI2C_SDA_Pin(config[CONF_SDA]))
  cg.add(var.setI2C_SCL_Pin(config[CONF_SCL]))
  cg.add(var.setRemoteType(config[CONF_REMOTE_TYPE]))
  cg.add(var.setStatusInputPin(config[CONF_STATUS_INPUT_PIN]))
  cg.add(var.setStatusOutputPin(config[CONF_STATUS_OUTPUT_PIN]))
