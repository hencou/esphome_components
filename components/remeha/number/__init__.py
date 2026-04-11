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
CONF_FLOW_SETPOINT = "flow_setpoint"
CONF_ROOM_SETPOINT1 = "room_setpoint1"

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
        cv.Optional(CONF_FLOW_SETPOINT): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer-water",
        ),
        cv.Optional(CONF_ROOM_SETPOINT1): number.number_schema(
            RemehaNumber,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:home-thermometer-outline",
        ),
    }
)

# SDO parameters: (index, subindex, size_bytes, scale, min, max, step)
NUMBER_PARAMS = {
    CONF_CP510_SETPOINT: (0x3451, 0x01, 2, 0.1, 5.0, 30.0, 0.5),
    CONF_DHW_COMFORT_SETPOINT: (0x3654, 0x01, 1, 1.0, 40.0, 65.0, 1.0),
    CONF_DHW_REDUCED_SETPOINT: (0x3655, 0x01, 1, 1.0, 10.0, 60.0, 1.0),
    CONF_FLOW_SETPOINT: (0x3402, 0x01, 1, 1.0, 7.0, 90.0, 1.0),
    CONF_ROOM_SETPOINT1: (0x340C, 0x01, 2, 0.1, 5.0, 30.0, 0.5),
}

# Setter methods on the parent Remeha class
PARENT_SETTERS = {
    CONF_CP510_SETPOINT: "set_cp510_setpoint_number",
    CONF_DHW_COMFORT_SETPOINT: "set_dhw_comfort_setpoint_number",
    CONF_DHW_REDUCED_SETPOINT: "set_dhw_reduced_setpoint_number",
    CONF_FLOW_SETPOINT: "set_flow_setpoint_number",
    CONF_ROOM_SETPOINT1: "set_room_setpoint1_number",
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_REMEHA_ID])

    for conf_key, (sdo_idx, sdo_sub, sdo_size, scale, min_val, max_val, step) in NUMBER_PARAMS.items():
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
            cg.add(getattr(parent, PARENT_SETTERS[conf_key])(num))
            # Register SDO poll for read-back
            cg.add(parent.add_sdo_poll(sdo_idx, sdo_sub))
