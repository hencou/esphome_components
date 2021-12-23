import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID


mi_ns = cg.esphome_ns.namespace("mi")
Mi = mi_ns.class_("Mi", cg.Component)

CONF_MI_ID = "mi_id"
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Mi),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add_library("SPI", None)
    cg.add_library("RF24", None)
    cg.add_library("https://github.com/ratkins/RGBConverter", None)
    cg.add_library("PathVariableHandlers", None)
    cg.add_library("https://github.com/luisllamasbinaburo/Arduino-List", None)
    cg.add_library("bblanchon/ArduinoJson", None)
    cg.add_library("ListLib", None)
