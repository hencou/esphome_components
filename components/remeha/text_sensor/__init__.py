import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

CONF_STATUS_TEXT = "status_text"
CONF_SUBSTATUS_TEXT = "substatus_text"
CONF_WRITE_STATUS = "write_status"
CONF_LAST_ERROR = "last_error"
ERROR_SLOT_KEYS = [f"error_{i + 1}" for i in range(5)]

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
        cv.Optional(CONF_LAST_ERROR): text_sensor.text_sensor_schema(
            icon="mdi:alert-circle-outline",
        ),
        **{
            cv.Optional(key): text_sensor.text_sensor_schema(
                icon="mdi:history",
            )
            for key in ERROR_SLOT_KEYS
        },
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

    if CONF_LAST_ERROR in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_ERROR])
        cg.add(parent.set_last_error_text_sensor(sens))

    for slot, key in enumerate(ERROR_SLOT_KEYS):
        if key in config:
            sens = await text_sensor.new_text_sensor(config[key])
            cg.add(parent.set_error_slot_text_sensor(slot, sens))
