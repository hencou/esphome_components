import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_PRESSURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

CONF_FLOW_TEMPERATURE = "flow_temperature"
CONF_RETURN_TEMPERATURE = "return_temperature"
CONF_OUTSIDE_TEMPERATURE = "outside_temperature"
CONF_OUTSIDE_TEMPERATURE_3M_AVG = "outside_temperature_3m_avg"
CONF_OUTSIDE_TEMPERATURE_2H_AVG = "outside_temperature_2h_avg"
CONF_SETPOINT = "setpoint"
CONF_RELATIVE_POWER = "relative_power"
CONF_RELATIVE_POWER2 = "relative_power2"
CONF_STATUS_CODE = "status_code"
CONF_SUBSTATUS_CODE = "substatus_code"
CONF_WATER_PRESSURE = "water_pressure"
CONF_ROOM_TEMPERATURE = "room_temperature"
CONF_ROOM_SETPOINT = "room_setpoint"
CONF_CALCULATED_ROOM_TEMPERATURE = "calculated_room_temperature"
CONF_LOCKING_MODE = "locking_mode"
CONF_BLOCKING_MODE = "blocking_mode"
CONF_ERROR_HISTORY = "error_history"
CONF_DIAGNOSTICS = "diagnostics"
CONF_APPLIANCE_TYPE = "appliance_type"
CONF_APPLIANCE_VARIANT = "appliance_variant"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_FLOW_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_RETURN_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_OUTSIDE_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_OUTSIDE_TEMPERATURE_3M_AVG): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_OUTSIDE_TEMPERATURE_2H_AVG): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_SETPOINT): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_RELATIVE_POWER): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_RELATIVE_POWER2): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_STATUS_CODE): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_SUBSTATUS_CODE): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_WATER_PRESSURE): sensor.sensor_schema(
            unit_of_measurement="bar",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRESSURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:gauge",
        ),
        cv.Optional(CONF_ROOM_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:home-thermometer",
        ),
        cv.Optional(CONF_ROOM_SETPOINT): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermostat",
        ),
        cv.Optional(CONF_CALCULATED_ROOM_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:home-thermometer-outline",
        ),
        cv.Optional(CONF_LOCKING_MODE): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_BLOCKING_MODE): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_ERROR_HISTORY): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_DIAGNOSTICS): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_APPLIANCE_TYPE): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_APPLIANCE_VARIANT): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
    }
)

# SDO objects that need to be polled for each sensor
SDO_POLL_MAP = {
    CONF_LOCKING_MODE: (0x500F, 0x00),
    CONF_BLOCKING_MODE: (0x5011, 0x00),
    CONF_ERROR_HISTORY: (0x1003, 0x01),
    CONF_DIAGNOSTICS: (0x2004, 0x01),
    CONF_APPLIANCE_TYPE: (0x502C, 0x00),
    CONF_APPLIANCE_VARIANT: (0x5037, 0x00),
    CONF_WATER_PRESSURE: (0x501D, 0x00),
    CONF_ROOM_TEMPERATURE: (0x501D, 0x00),
    CONF_ROOM_SETPOINT: (0x501D, 0x00),
    CONF_CALCULATED_ROOM_TEMPERATURE: (0x501D, 0x00),
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_REMEHA_ID])

    sensor_configs = {
        CONF_FLOW_TEMPERATURE: "set_flow_temperature_sensor",
        CONF_RETURN_TEMPERATURE: "set_return_temperature_sensor",
        CONF_OUTSIDE_TEMPERATURE: "set_outside_temperature_sensor",
        CONF_OUTSIDE_TEMPERATURE_3M_AVG: "set_outside_temperature_3m_avg_sensor",
        CONF_OUTSIDE_TEMPERATURE_2H_AVG: "set_outside_temperature_2h_avg_sensor",
        CONF_SETPOINT: "set_setpoint_sensor",
        CONF_RELATIVE_POWER: "set_relative_power_sensor",
        CONF_RELATIVE_POWER2: "set_relative_power2_sensor",
        CONF_STATUS_CODE: "set_status_code_sensor",
        CONF_SUBSTATUS_CODE: "set_substatus_code_sensor",
        CONF_WATER_PRESSURE: "set_water_pressure_sensor",
        CONF_ROOM_TEMPERATURE: "set_room_temperature_sensor",
        CONF_ROOM_SETPOINT: "set_room_setpoint_sensor",
        CONF_CALCULATED_ROOM_TEMPERATURE: "set_calculated_room_temperature_sensor",
        CONF_LOCKING_MODE: "set_locking_mode_sensor",
        CONF_BLOCKING_MODE: "set_blocking_mode_sensor",
        CONF_ERROR_HISTORY: "set_error_history_sensor",
        CONF_DIAGNOSTICS: "set_diagnostics_sensor",
        CONF_APPLIANCE_TYPE: "set_appliance_type_sensor",
        CONF_APPLIANCE_VARIANT: "set_appliance_variant_sensor",
    }

    for conf_key, setter in sensor_configs.items():
        if conf_key in config:
            sens = await sensor.new_sensor(config[conf_key])
            cg.add(getattr(parent, setter)(sens))
            # Register SDO poll if this sensor needs it
            if conf_key in SDO_POLL_MAP:
                idx, sub = SDO_POLL_MAP[conf_key]
                cg.add(parent.add_sdo_poll(idx, sub))
