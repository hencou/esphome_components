import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_FREQUENCY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    UNIT_HOUR,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)
from .. import itho_ns, CONF_ITHO_ID, Itho

CONF_ERROR = "error"
CONF_OPERATION_TIME = "operation_time"
CONF_FAN_SETPOINT = "fan_setpoint"
CONF_FAN_SPEED = "fan_speed"
UNIT_RPM = "rpm"

#DEPENDENCIES = ["itho"]
AUTO_LOAD = ["itho"]

Itho_Sensor = itho_ns.class_("Itho_Sensor", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Itho_Sensor),
            cv.GenerateID(CONF_ITHO_ID): cv.use_id(Itho),
            cv.Optional(CONF_ERROR): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_OPERATION_TIME): sensor.sensor_schema(
                unit_of_measurement=UNIT_HOUR,
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FAN_SETPOINT): sensor.sensor_schema(
                unit_of_measurement=UNIT_RPM,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_FREQUENCY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_FAN_SPEED): sensor.sensor_schema(
                unit_of_measurement=UNIT_RPM,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_FREQUENCY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
  )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    if CONF_ERROR in config:
        sens = await sensor.new_sensor(config[CONF_ERROR])
        cg.add(var.set_error_sensor(sens))

    if CONF_OPERATION_TIME in config:
        sens = await sensor.new_sensor(config[CONF_OPERATION_TIME])
        cg.add(var.set_operation_time_sensor(sens))

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))

    if CONF_HUMIDITY in config:
        sens = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity_sensor(sens))

    if CONF_FAN_SETPOINT in config:
        sens = await sensor.new_sensor(config[CONF_FAN_SETPOINT])
        cg.add(var.set_fan_setpoint_sensor(sens))

    if CONF_FAN_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_FAN_SPEED])
        cg.add(var.set_fan_speed_sensor(sens))
    
    paren = await cg.get_variable(config[CONF_ITHO_ID])
    cg.add(var.set_itho_parent(paren))
