import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import light
from esphome.components.light.types import LambdaLightEffect
from esphome.core import CORE, ID, CoroPriority, HexInt, coroutine_with_priority
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_ID,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_GAMMA_CORRECT,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
)
from .. import mi_ns, CONF_MI_ID, Mi

DEPENDENCIES = ["mi"]

REMOTE_TYPES = {
    "rgb_cct": "rgb_cct",
    "cct": "cct",
    "rgb": "rgb",
    "rgbw": "rgbw",
    "fut089": "fut089",
    "fut091": "fut091",
    "fut020": "fut020",
    "s2": "s2",
}

CODEOWNERS = ["@hencou"]

CONF_DEVICEID = "mi_device_id"
CONF_GROUPID = "group_id"
CONF_REMOTETYPE = "remote_type"

CONF_EFFECT_ID = "effect_id"

MiLight = mi_ns.class_("MiLight", light.LightOutput, cg.Component)
LambdaLightEffect = cg.esphome_ns.namespace("light").class_("LambdaLightEffect")

CONFIG_SCHEMA = cv.All(
    light.light_schema(MiLight, light.LightType.RGB).extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MiLight),
            cv.GenerateID(CONF_EFFECT_ID): cv.declare_id(LambdaLightEffect),
            cv.GenerateID(CONF_MI_ID): cv.use_id(Mi),
            cv.Required(CONF_DEVICEID): cv.uint16_t,
            cv.Required(CONF_GROUPID): cv.uint8_t,
            cv.Required(CONF_REMOTETYPE): cv.enum(REMOTE_TYPES),

            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="200ms"
            ): cv.positive_time_period_milliseconds,

            cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE, default="153 mireds"): cv.color_temperature,
            cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE, default="370 mireds"): cv.color_temperature,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    light.validate_color_temperature_channels,
)


async def to_code(config):
    output_var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(output_var, config)
    
    light_var = cg.new_Pvariable(config[CONF_ID], output_var)
    cg.add(cg.App.register_light(light_var))
    CORE.register_platform_component("light", light_var)
    await cg.register_component(light_var, config)
    await light.setup_light_core_(light_var, config, output_var)

    parent = await cg.get_variable(config[CONF_MI_ID])
    cg.add(output_var.set_mi_parent(parent))

    cg.add(output_var.set_bulb_id(
        config[CONF_DEVICEID],
        config[CONF_GROUPID],
        config[CONF_REMOTETYPE],
    ))

    cg.add(output_var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))
    cg.add(output_var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))

    # =========================
    # 🔥 Custom Lambda Effect
    # =========================
    
    remote_type = config[CONF_REMOTETYPE]

    rgb_types = [
        "rgb_cct",
        "rgb",
        "rgbw",
        "fut089",
        "fut020",
    ]
    
    effect_names = [
        "Mi 00: Night Mode",
        "Mi 01: Rainbow",
        "Mi 02: Low-Sat Rainbow",
        "Mi 03: No-Red Rainbow",
        "Mi 04: Fast Rainbow",
        "Mi 05: Red-White-Blue",
        "Mi 06: Pink-Blue-White",
        "Mi 07: Green-Blue-Red-White",
        "Mi 08: Slow Random Fade",
        "Mi 09: Slow Blue-Green Fade",
    ]
    
    # if remote_type in rgb_types:
        # for i, effect_name in enumerate(effect_names):
            # lambda_code = cg.RawExpression(f"""
                # [](bool initial_run) -> void {{
                  # auto call = {light_var}->make_call();
                  # call.set_effect("{effect_name}");
                  # call.perform();
              # }}, 0xffffffff
            # """)

            # var_effect = cg.new_Pvariable(
                # config[CONF_EFFECT_ID],
                # effect_name,
                # lambda_code
            # )

            # cg.add(light_var.add_effects([var_effect]))
    # else:
        # effect_name = effect_names[0]
        
        # lambda_code = cg.RawExpression(f"""
            # [](bool initial_run) -> void {{
              # auto call = {light_var}->make_call();
              # call.set_effect("{effect_name}");
              # call.perform();
          # }}, 0xffffffff
        # """)

        # var_effect = cg.new_Pvariable(
            # config[CONF_EFFECT_ID],
            # effect_name,
            # lambda_code
        # )

        # cg.add(light_var.add_effects([var_effect]))
