import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    UNIT_CELSIUS,
)
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

RemehaNumber = remeha_ns.class_("RemehaNumber", number.Number, cg.Component)

CONF_CP510_SETPOINT = "cp510_setpoint"
CONF_DHW_COMFORT_SETPOINT = "dhw_comfort_setpoint"
CONF_DHW_REDUCED_SETPOINT = "dhw_reduced_setpoint"
CONF_NIGHT_SETPOINT = "night_setpoint"
CONF_HOLIDAY_SETPOINT = "holiday_setpoint"
#CONF_SUMMER_WINTER_THRESHOLD = "summer_winter_threshold"
#CONF_HEATING_CURVE_SLOPE = "heating_curve_slope"
#CONF_CH_MAX_FLOW_TEMPERATURE = "ch_max_flow_temperature"
#CONF_ROOM_SENSOR_CALIBRATION = "room_sensor_calibration"
#CONF_ANTI_LEGIONELLA_SETPOINT = "anti_legionella_setpoint"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_CP510_SETPOINT): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:home-thermometer",
        ),
        cv.Optional(CONF_DHW_COMFORT_SETPOINT): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:water-thermometer",
        ),
        cv.Optional(CONF_DHW_REDUCED_SETPOINT): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:water-thermometer-outline",
        ),
        cv.Optional(CONF_NIGHT_SETPOINT): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:weather-night",
        ),
        cv.Optional(CONF_HOLIDAY_SETPOINT): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:palm-tree",
        ),
        #cv.Optional(CONF_SUMMER_WINTER_THRESHOLD): number.number_schema(
        #    RemehaNumber,
        #    unit_of_measurement=UNIT_CELSIUS,
        #    icon="mdi:sun-snowflake-variant",
        #),
        #cv.Optional(CONF_HEATING_CURVE_SLOPE): number.number_schema(
        #    RemehaNumber,
        #    icon="mdi:chart-line",
        #),
        #cv.Optional(CONF_CH_MAX_FLOW_TEMPERATURE): number.number_schema(
        #    RemehaNumber,
        #    unit_of_measurement=UNIT_CELSIUS,
        #    icon="mdi:thermometer-chevron-up",
        #),
        #cv.Optional(CONF_ROOM_SENSOR_CALIBRATION): number.number_schema(
        #    RemehaNumber,
        #    unit_of_measurement=UNIT_CELSIUS,
        #    icon="mdi:tune",
        #),
        #cv.Optional(CONF_ANTI_LEGIONELLA_SETPOINT): number.number_schema(
        #    RemehaNumber,
        #    unit_of_measurement=UNIT_CELSIUS,
        #    icon="mdi:bacteria-outline",
        #),
    }
)

# SDO parameters: (index, subindex, size_bytes, scale, min, max, step, is_signed)
NUMBER_PARAMS = {
    CONF_CP510_SETPOINT: (0x3451, 0x01, 2, 0.1, 5.0, 30.0, 0.5, False),
    CONF_DHW_COMFORT_SETPOINT: (0x3654, 0x01, 2, 0.01, 40.0, 65.0, 1.0, False),
    CONF_DHW_REDUCED_SETPOINT: (0x3655, 0x01, 2, 0.01, 10.0, 60.0, 1.0, False),
    CONF_NIGHT_SETPOINT: (0x340B, 0x01, 2, 0.1, 5.0, 30.0, 0.5, False),
    CONF_HOLIDAY_SETPOINT: (0x340A, 0x01, 2, 0.1, 0.5, 20.0, 0.5, False),
    #CONF_SUMMER_WINTER_THRESHOLD: (0x303A, 0x00, 2, 0.1, 15.0, 30.5, 0.5, False),
    #CONF_HEATING_CURVE_SLOPE: (0x3416, 0x01, 1, 0.1, 0.0, 4.0, 0.1, False),
    #CONF_CH_MAX_FLOW_TEMPERATURE: (0x3030, 0x00, 2, 0.01, 20.0, 90.0, 1.0, False),
    #CONF_ROOM_SENSOR_CALIBRATION: (0x3418, 0x01, 1, 0.1, -5.0, 5.0, 0.1, True),
    #CONF_ANTI_LEGIONELLA_SETPOINT: (0x365D, 0x01, 1, 1.0, 60.0, 90.0, 1.0, False),
}

# Setter methods on the parent Remeha class
PARENT_SETTERS = {
    CONF_CP510_SETPOINT: "set_cp510_setpoint_number",
    CONF_DHW_COMFORT_SETPOINT: "set_dhw_comfort_setpoint_number",
    CONF_DHW_REDUCED_SETPOINT: "set_dhw_reduced_setpoint_number",
    CONF_NIGHT_SETPOINT: "set_night_setpoint_number",
    CONF_HOLIDAY_SETPOINT: "set_holiday_setpoint_number",
    #CONF_SUMMER_WINTER_THRESHOLD: "set_summer_winter_threshold_number",
    #CONF_HEATING_CURVE_SLOPE: "set_heating_curve_slope_number",
    #CONF_CH_MAX_FLOW_TEMPERATURE: "set_ch_max_flow_temperature_number",
    #CONF_ROOM_SENSOR_CALIBRATION: "set_room_sensor_calibration_number",
    #CONF_ANTI_LEGIONELLA_SETPOINT: "set_anti_legionella_setpoint_number",
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_REMEHA_ID])

    for conf_key, (sdo_idx, sdo_sub, sdo_size, scale, min_val, max_val, step, is_signed) in NUMBER_PARAMS.items():
        if conf_key in config:
            num = await number.new_number(
                config[conf_key],
                min_value=min_val,
                max_value=max_val,
                step=step,
            )
            await cg.register_component(num, config[conf_key])
            cg.add(num.set_parent(parent))
            cg.add(num.set_sdo_index(sdo_idx))
            cg.add(num.set_sdo_subindex(sdo_sub))
            cg.add(num.set_sdo_size(sdo_size))
            cg.add(num.set_scale(scale))
            if is_signed:
                cg.add(num.set_is_signed(True))
            cg.add(getattr(parent, PARENT_SETTERS[conf_key])(num))
            # Register SDO poll for read-back
            cg.add(parent.add_sdo_poll(sdo_idx, sdo_sub))
