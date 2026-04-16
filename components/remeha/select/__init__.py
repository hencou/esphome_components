import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

RemehaSelect = remeha_ns.class_("RemehaSelect", select.Select, cg.Component)

CONF_ZONE_MODE = "zone_mode"
CONF_TIME_PROGRAM = "time_program"
CONF_DHW_ENABLED = "dhw_enabled"
CONF_ANTI_LEGIONELLA_MODE = "anti_legionella_mode"
CONF_FIREPLACE_MODE = "fireplace_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_ZONE_MODE): select.select_schema(
            RemehaSelect,
            icon="mdi:home-switch",
        ),
        cv.Optional(CONF_TIME_PROGRAM): select.select_schema(
            RemehaSelect,
            icon="mdi:clock-outline",
        ),
        cv.Optional(CONF_DHW_ENABLED): select.select_schema(
            RemehaSelect,
            icon="mdi:water-boiler",
        ),
        cv.Optional(CONF_ANTI_LEGIONELLA_MODE): select.select_schema(
            RemehaSelect,
            icon="mdi:bacteria-outline",
        ),
        cv.Optional(CONF_FIREPLACE_MODE): select.select_schema(
            RemehaSelect,
            icon="mdi:fireplace",
        ),
    }
)

# SDO parameters and options for each select
SELECT_PARAMS = {
    CONF_ZONE_MODE: {
        "sdo_index": 0x341F,
        "sdo_subindex": 0x01,
        "options": ["Auto", "Heat", "Off"],
        "setter": "set_zone_mode_select",
        "value_offset": 0,
    },
    CONF_TIME_PROGRAM: {
        "sdo_index": 0x3458,
        "sdo_subindex": 0x01,
        "options": ["Klokprogramma 1", "Klokprogramma 2", "Klokprogramma 3"],
        "setter": "set_time_program_select",
        "value_offset": 0,
    },
    CONF_DHW_ENABLED: {
        "sdo_index": 0x3013,
        "sdo_subindex": 0x00,
        "options": ["Uit", "Aan"],
        "setter": "set_dhw_enabled_select",
        "value_offset": 0,
    },
    CONF_ANTI_LEGIONELLA_MODE: {
        "sdo_index": 0x3604,
        "sdo_subindex": 0x00,
        "options": ["Uit", "Aan", "Auto"],
        "setter": "set_anti_legionella_mode_select",
        "value_offset": 0,
    },
    CONF_FIREPLACE_MODE: {
        "sdo_index": 0x3455,
        "sdo_subindex": 0x01,
        "options": ["Uit", "Aan"],
        "setter": "set_fireplace_mode_select",
        "value_offset": 0,
    },
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_REMEHA_ID])

    for conf_key, params in SELECT_PARAMS.items():
        if conf_key in config:
            sel = await select.new_select(
                config[conf_key],
                options=params["options"],
            )
            await cg.register_component(sel, config[conf_key])
            cg.add(sel.set_parent(parent))
            cg.add(sel.set_sdo_index(params["sdo_index"]))
            cg.add(sel.set_sdo_subindex(params["sdo_subindex"]))
            cg.add(sel.set_value_offset(params["value_offset"]))
            cg.add(getattr(parent, params["setter"])(sel))
            # Register SDO poll for read-back
            cg.add(parent.add_sdo_poll(params["sdo_index"], params["sdo_subindex"]))
