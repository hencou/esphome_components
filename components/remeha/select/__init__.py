import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from .. import remeha_ns, CONF_REMEHA_ID, Remeha

AUTO_LOAD = ["remeha"]

RemehaSelect = remeha_ns.class_("RemehaSelect", select.Select, cg.Component)

CONF_ZONE_MODE = "zone_mode"
CONF_DHW_MODE = "dhw_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_ZONE_MODE): select.select_schema(
            RemehaSelect,
            icon="mdi:home-switch",
        ),
        cv.Optional(CONF_DHW_MODE): select.select_schema(
            RemehaSelect,
            icon="mdi:water-boiler",
        ),
    }
)

# SDO parameters and options for each select
SELECT_PARAMS = {
    CONF_ZONE_MODE: {
        "sdo_index": 0x341F,
        "sdo_subindex": 0x01,
        "options": ["Off", "Heat", "Auto"],
        "setter": "set_zone_mode_select",
    },
    CONF_DHW_MODE: {
        "sdo_index": 0x3661,
        "sdo_subindex": 0x01,
        "options": ["Off", "On", "Auto"],
        "setter": "set_dhw_mode_select",
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
            cg.add(getattr(parent, params["setter"])(sel))
            # Register SDO poll for read-back
            cg.add(parent.add_sdo_poll(params["sdo_index"], params["sdo_subindex"]))
