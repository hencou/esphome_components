import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_CO2,
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_AQI,
    DEVICE_CLASS_CARBON_DIOXIDE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_FREQUENCY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    UNIT_HOUR,
    UNIT_CELSIUS,
    UNIT_MINUTE,
    UNIT_PARTS_PER_MILLION,
    UNIT_PERCENT,
)
from .. import itho_ns, CONF_ITHO_ID, Itho

CONF_ERROR = "error"
CONF_STARTUP_COUNTER = "startup_counter"
CONF_OPERATION_TIME = "operation_time"
CONF_FAN_SETPOINT = "fan_setpoint"
CONF_FAN_SPEED = "fan_speed"
UNIT_RPM = "rpm"
UNIT_LITRE_PER_SECOND = "l/s"


def _percentage_sensor():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _temperature_sensor():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _humidity_sensor():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_HUMIDITY,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _flow_sensor():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_LITRE_PER_SECOND,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _diagnostic_sensor():
    return sensor.sensor_schema(
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )


# Values parsed from the 31DA/31D9 responses that are already requested, keyed
# by config key with the setter that binds the sensor.
EXTRA_SENSORS = {
    "air_quality": (
        sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_AQI,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        lambda var, sens: var.set_air_quality_sensor(sens),
    ),
    CONF_CO2: (
        sensor.sensor_schema(
            unit_of_measurement=UNIT_PARTS_PER_MILLION,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_CARBON_DIOXIDE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        lambda var, sens: var.set_co2_sensor(sens),
    ),
    "indoor_humidity": (_humidity_sensor(), lambda var, sens: var.set_indoor_humidity_sensor(sens)),
    "outdoor_humidity": (_humidity_sensor(), lambda var, sens: var.set_outdoor_humidity_sensor(sens)),
    "exhaust_temperature": (_temperature_sensor(), lambda var, sens: var.set_exhaust_temperature_sensor(sens)),
    "supply_temperature": (_temperature_sensor(), lambda var, sens: var.set_supply_temperature_sensor(sens)),
    "indoor_temperature": (_temperature_sensor(), lambda var, sens: var.set_indoor_temperature_sensor(sens)),
    "outdoor_temperature": (_temperature_sensor(), lambda var, sens: var.set_outdoor_temperature_sensor(sens)),
    "speed_cap": (_diagnostic_sensor(), lambda var, sens: var.set_speed_cap_sensor(sens)),
    "bypass_position": (_percentage_sensor(), lambda var, sens: var.set_bypass_position_sensor(sens)),
    "exhaust_fan_speed": (_percentage_sensor(), lambda var, sens: var.set_exhaust_fan_speed_sensor(sens)),
    "supply_fan_speed": (_percentage_sensor(), lambda var, sens: var.set_supply_fan_speed_sensor(sens)),
    "remaining_time": (
        sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        lambda var, sens: var.set_remaining_time_sensor(sens),
    ),
    "post_heat": (_percentage_sensor(), lambda var, sens: var.set_post_heat_sensor(sens)),
    "pre_heat": (_percentage_sensor(), lambda var, sens: var.set_pre_heat_sensor(sens)),
    "supply_flow": (_flow_sensor(), lambda var, sens: var.set_supply_flow_sensor(sens)),
    "exhaust_flow": (_flow_sensor(), lambda var, sens: var.set_exhaust_flow_sensor(sens)),
    "internal_fault": (_diagnostic_sensor(), lambda var, sens: var.set_internal_fault_sensor(sens)),
    "frost_cycle": (_diagnostic_sensor(), lambda var, sens: var.set_frost_cycle_sensor(sens)),
    "filter_dirty": (_diagnostic_sensor(), lambda var, sens: var.set_filter_dirty_sensor(sens)),
}

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
            cv.Optional(CONF_STARTUP_COUNTER): sensor.sensor_schema(
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
            **{
                cv.Optional(key): schema
                for key, (schema, _) in EXTRA_SENSORS.items()
            },
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

    if CONF_STARTUP_COUNTER in config:
        sens = await sensor.new_sensor(config[CONF_STARTUP_COUNTER])
        cg.add(var.set_startup_counter_sensor(sens))

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

    for key, (_, setter) in EXTRA_SENSORS.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(setter(var, sens))
    
    paren = await cg.get_variable(config[CONF_ITHO_ID])
    cg.add(var.set_itho_parent(paren))
