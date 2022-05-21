import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID
from .. import itho_cve_ns, CONF_ITHOCVE_ID, IthoCVE

DEPENDENCIES = ["itho_cve"]

IthoCVE_Fan = itho_cve_ns.class_("IthoCVE_Fan", fan.Fan, cg.Component)

CONFIG_SCHEMA = cv.All(
  fan.FAN_SCHEMA.extend(
    {
      cv.GenerateID(CONF_ID): cv.declare_id(IthoCVE_Fan),
      cv.GenerateID(CONF_ITHOCVE_ID): cv.use_id(IthoCVE),
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await fan.register_fan(var, config)
 
