import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

itho_ns = cg.esphome_ns.namespace("itho")
Itho = itho_ns.class_("Itho", cg.Component)

SYSSHT30_VALUES = {
  "enable" : 2,
  "disable" : 1,
}

CONF_SYSSHT30_VALUE = "syssht30"
CONF_SYSSHT30_ADDRESS = "syssht30_address"

CONF_ITHO_ID = "itho_id"
CONFIG_SCHEMA = (
  cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Itho),
        cv.Optional(CONF_SYSSHT30_VALUE) : cv.enum(SYSSHT30_VALUES),
        cv.Optional(CONF_SYSSHT30_ADDRESS) : cv.i2c_address
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  
  cg.add_library("Ticker", None)
  cg.add_library("ArduinoJson", None)
  
  if CONF_SYSSHT30_VALUE in config:
    cg.add(var.setSysSHT30(config[CONF_SYSSHT30_VALUE]))
    
  if CONF_SYSSHT30_ADDRESS in config:
    cg.add(var.setSysSHT30_Address(config[CONF_SYSSHT30_ADDRESS]))