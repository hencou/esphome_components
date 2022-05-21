import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

itho_cve_ns = cg.esphome_ns.namespace("itho_cve")
IthoCVE = itho_cve_ns.class_("IthoCVE", cg.Component)

CONF_ITHOCVE_ID = "itho_cve_id"
CONFIG_SCHEMA = (
  cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(IthoCVE),
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  
  cg.add_library("Ticker", None)
