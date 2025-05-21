import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID
from .. import itho_ns, CONF_ITHO_ID, Itho

DEPENDENCIES = ["itho"]

Itho_Fan = itho_ns.class_("Itho_Fan", fan.Fan, cg.Component)

CONFIG_SCHEMA = cv.All(
  fan.FAN_SCHEMA.extend(
    {
      cv.GenerateID(CONF_ID): cv.declare_id(Itho_Fan),
      cv.GenerateID(CONF_ITHO_ID): cv.use_id(Itho),
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await fan.register_fan(var, config)
  
  paren = await cg.get_variable(config[CONF_ITHO_ID])
  cg.add(var.set_itho_parent(paren))
