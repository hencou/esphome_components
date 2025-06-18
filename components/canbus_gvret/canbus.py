from itertools import groupby
import logging
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import automation
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome.core import CORE
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.components.canbus import (
    CANBUS_SCHEMA,
    CONF_CANBUS_ID,
    CanbusComponent,
    register_canbus,
)

logger = logging.getLogger(__name__)

ns = cg.esphome_ns.namespace('canbus_gvret')
CanbusGVRET = ns.class_(
    'CanbusGVRET',
    CanbusComponent,
)

DEPENDENCIES = ["api"]

CONFIG_SCHEMA = CANBUS_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(CanbusGVRET),
    cv.Optional(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
})


async def to_code(config):
    if "canbus_id" in config:
        canbus = await cg.get_variable(config["canbus_id"])
    else:
        canbus = cg.RawExpression('nullptr')

    canbus_gvret = cg.new_Pvariable(
        config[CONF_ID],
        canbus,
    )
    await register_canbus(canbus_gvret, config)
    if CORE.target_framework == 'esp-idf':
        add_idf_sdkconfig_option("CONFIG_LWIP_SO_RCVBUF", True)
    else:
        logger.warning("RCVBUF cannot be set on arduino")
