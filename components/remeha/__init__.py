import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.canbus import CanbusComponent, CONF_CANBUS_ID
from esphome.const import CONF_ID

CODEOWNERS = ["@hencou"]
DEPENDENCIES = ["canbus"]

remeha_ns = cg.esphome_ns.namespace("remeha")
Remeha = remeha_ns.class_("Remeha", cg.Component)

CONF_REMEHA_ID = "remeha_id"
CONF_BOOT_DELAY = "boot_delay"
CONF_USER_LEVEL = "user_level"
CONF_AUTH_KEY = "auth_key"


def auth_key(value):
    """Validate the 32-bit TEA key word as a hex value.

    Accepts an int or a (secret) string such as "0x1234ABCD" or "1234ABCD".
    """
    if isinstance(value, str):
        text = value.strip().lower()
        if text.startswith("0x"):
            text = text[2:]
        try:
            value = int(text, 16)
        except ValueError as err:
            raise cv.Invalid(
                f"{CONF_AUTH_KEY} must be a 32-bit hex value like 0x1234ABCD, got '{value}'"
            ) from err
    value = cv.int_(value)
    if not 0 <= value <= 0xFFFFFFFF:
        raise cv.Invalid(f"{CONF_AUTH_KEY} must fit in 32 bits")
    return value


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Remeha),
        cv.Required(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Required(CONF_AUTH_KEY): auth_key,
        cv.Optional(CONF_BOOT_DELAY, default="10s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_USER_LEVEL, default=2): cv.int_range(min=1, max=3),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    canbus = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus))

    cg.add(var.set_boot_delay(config[CONF_BOOT_DELAY]))
    cg.add(var.set_user_level(config[CONF_USER_LEVEL]))
    cg.add(var.set_auth_key(config[CONF_AUTH_KEY]))
