import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

RemehaSelect = remeha_ns.class_("RemehaSelect", select.Select, cg.Component)

CONF_ZONE_MODE = "zone_mode"
CONF_TIME_PROGRAM = "time_program"
CONF_CH_ENABLED = "ch_enabled"
CONF_DHW_ENABLED = "dhw_enabled"
CONF_ANTI_LEGIONELLA_MODE = "anti_legionella_mode"
CONF_FIREPLACE_MODE = "fireplace_mode"
CONF_ECO_MODE = "eco_mode"

CONF_OPTIONS = "options"
 
def _select_schema_with_options(icon):
    return select.select_schema(
        RemehaSelect,
        icon=icon,
    ).extend(
        {
            cv.Optional(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string), cv.Length(min=2)
            ),
        }
    )
 
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_ZONE_MODE): _select_schema_with_options("mdi:home-switch"),
        cv.Optional(CONF_TIME_PROGRAM): _select_schema_with_options("mdi:clock-outline"),
        cv.Optional(CONF_CH_ENABLED): _select_schema_with_options("mdi:radiator"),
        cv.Optional(CONF_DHW_ENABLED): _select_schema_with_options("mdi:water-boiler"),
        cv.Optional(CONF_ANTI_LEGIONELLA_MODE): _select_schema_with_options("mdi:bacteria-outline"),
        cv.Optional(CONF_FIREPLACE_MODE): _select_schema_with_options("mdi:fireplace"),
        cv.Optional(CONF_ECO_MODE): _select_schema_with_options("mdi:sprout"),
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
        "options": ["Time Program 1", "Time Program 2", "Time Program 3"],
        "setter": "set_time_program_select",
        "value_offset": 0,
    },
    CONF_CH_ENABLED: {
        "sdo_index": 0x3012,
        "sdo_subindex": 0x00,
        "options": ["Uit", "Aan"],
        "setter": "set_ch_enabled_select",
        "value_offset": 0,
    },
    CONF_DHW_ENABLED: {
        "sdo_index": 0x3013,
        "sdo_subindex": 0x00,
        "options": ["Off", "On"],
        "setter": "set_dhw_enabled_select",
        "value_offset": 0,
    },
    CONF_ANTI_LEGIONELLA_MODE: {
        "sdo_index": 0x3604,
        "sdo_subindex": 0x00,
        "options": ["Off", "On", "Auto"],
        "setter": "set_anti_legionella_mode_select",
        "value_offset": 0,
    },
    CONF_FIREPLACE_MODE: {
        "sdo_index": 0x3455,
        "sdo_subindex": 0x01,
        "options": ["Off", "On"],
        "setter": "set_fireplace_mode_select",
        "value_offset": 0,
    },
    CONF_ECO_MODE: {
        "sdo_index": 0x3015,
        "sdo_subindex": 0x00,
        "options": ["Off", "On"],
        "setter": "set_eco_mode_select",
        "value_offset": 0,
    },
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_REMEHA_ID])

    for conf_key, params in SELECT_PARAMS.items():
        if conf_key in config:
            # Use user-provided options if available, otherwise use defaults
            options = config[conf_key].get(CONF_OPTIONS, params["options"])
            
            sel = await select.new_select(
                config[conf_key],
                options=options,
            )
            await cg.register_component(sel, config[conf_key])
            cg.add(sel.set_parent(parent))
            cg.add(sel.set_sdo_index(params["sdo_index"]))
            cg.add(sel.set_sdo_subindex(params["sdo_subindex"]))
            cg.add(sel.set_value_offset(params["value_offset"]))
            cg.add(getattr(parent, params["setter"])(sel))
            # Register SDO poll for read-back
            cg.add(parent.add_sdo_poll(params["sdo_index"], params["sdo_subindex"]))
