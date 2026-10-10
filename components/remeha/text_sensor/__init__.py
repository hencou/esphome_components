import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

CONF_STATUS_TEXT = "status_text"
CONF_SUBSTATUS_TEXT = "substatus_text"
CONF_WRITE_STATUS = "write_status"
CONF_ERROR_LOG = "error_log"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_STATUS_TEXT): text_sensor.text_sensor_schema(
            icon="mdi:information-outline",
        ),
        cv.Optional(CONF_SUBSTATUS_TEXT): text_sensor.text_sensor_schema(
            icon="mdi:information-outline",
        ),
        cv.Optional(CONF_WRITE_STATUS): text_sensor.text_sensor_schema(
            icon="mdi:clipboard-check-outline",
        ),
        cv.Optional(CONF_ERROR_LOG): text_sensor.text_sensor_schema(
            icon="mdi:alert-circle-outline",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_REMEHA_ID])

    if CONF_STATUS_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STATUS_TEXT])
        cg.add(parent.set_status_text_sensor(sens))

    if CONF_SUBSTATUS_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SUBSTATUS_TEXT])
        cg.add(parent.set_substatus_text_sensor(sens))

    if CONF_WRITE_STATUS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_WRITE_STATUS])
        cg.add(parent.set_write_status_text_sensor(sens))

    if CONF_ERROR_LOG in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ERROR_LOG])
        cg.add(parent.set_error_log_text_sensor(sens))
