import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

from .. import remeha_ns, Remeha, CONF_REMEHA_ID

CONF_TIME_PROGRAM_NAMES = "time_program_names"
CONF_ZONE = "zone"

DEPENDENCIES = ["remeha"]
CODEOWNERS = ["@hencou"]

RemehaClimate = remeha_ns.class_(
    "RemehaClimate", climate.Climate, cg.Component
)

CONFIG_SCHEMA = climate.climate_schema(RemehaClimate).extend(
    {
        cv.GenerateID(CONF_REMEHA_ID): cv.use_id(Remeha),
        cv.Optional(CONF_ZONE, default=1): cv.int_range(min=1, max=10),
        cv.Optional(CONF_TIME_PROGRAM_NAMES): cv.All(
            cv.ensure_list(cv.string), cv.Length(min=3, max=4)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_REMEHA_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_climate(var))
    cg.add(var.set_zone(config[CONF_ZONE]))

    if CONF_TIME_PROGRAM_NAMES in config:
        for i, name in enumerate(config[CONF_TIME_PROGRAM_NAMES]):
            cg.add(var.set_time_program_name(i, name))
 
    # Poll time program selection (0x3458) to display active time program
    cg.add(parent.add_sdo_poll(0x3458, config[CONF_ZONE]))
