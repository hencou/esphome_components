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
    }
)

# SDO parameters: (index, subindex, size_bytes, scale, min, max, step)
NUMBER_PARAMS = {
    CONF_CP510_SETPOINT: (0x3451, 0x01, 2, 0.1, 5.0, 30.0, 0.5),
    CONF_DHW_COMFORT_SETPOINT: (0x3654, 0x01, 1, 1.0, 40.0, 65.0, 1.0),
}

# Setter methods on the parent Remeha class
PARENT_SETTERS = {
    CONF_CP510_SETPOINT: "set_cp510_setpoint_number",
    CONF_DHW_COMFORT_SETPOINT: "set_dhw_comfort_setpoint_number",
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
