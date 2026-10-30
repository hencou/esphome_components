from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_RESTORE_VALUE,
)
from .. import itho_ns, CONF_ITHO_ID, Itho

DEPENDENCIES = ["itho"]

Itho_Select = itho_ns.class_("Itho_Select", select.Select, cg.PollingComponent)

CONF_SET_ACTION = "set_action"

def validate(config):
    if CONF_LAMBDA in config:
        if CONF_RESTORE_VALUE in config:
            raise cv.Invalid("restore_value cannot be used with lambda")

    return config


CONFIG_SCHEMA = cv.All(
    select.select_schema(Itho_Select).extend(
        {
            cv.GenerateID(): cv.declare_id(Itho_Select),
            cv.GenerateID(CONF_ITHO_ID): cv.use_id(Itho),
            cv.Optional(CONF_LAMBDA): cv.returning_lambda,
            cv.Optional(CONF_SET_ACTION): automation.validate_automation(single=True),
            cv.Optional(CONF_RESTORE_VALUE): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(cv.polling_component_schema("60s")),
    validate
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=["manual", "away", "low", "auto", "high"])

    if CONF_LAMBDA in config:
        itho_ = await cg.process_lambda(
            config[CONF_LAMBDA], [], return_type=cg.optional.itho(cg.std_string)
        )
        cg.add(var.set_itho(itho_))

    else:
        if CONF_RESTORE_VALUE in config:
            cg.add(var.set_restore_value(config[CONF_RESTORE_VALUE]))

    if CONF_SET_ACTION in config:
        await automation.build_automation(
            var.get_set_trigger(), [(cg.std_string, "x")], config[CONF_SET_ACTION]
        )
    
    paren = await cg.get_variable(config[CONF_ITHO_ID])
    cg.add(var.set_itho_parent(paren))
