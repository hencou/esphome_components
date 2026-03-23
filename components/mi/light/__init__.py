import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import light
from esphome.components.light.types import LambdaLightEffect, LightEffect
from esphome.components.light import effects as light_effects
from esphome.core import CORE, ID, CoroPriority, HexInt, coroutine_with_priority
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_ID,
    CONF_NAME,
    CONF_EFFECTS,
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

OUTPUT_NAME = ""

CONF_MI_EFFECT_ID = "effect_id"

RGB_TYPES = [
    "rgb_cct",
    "rgb",
    "rgbw",
    "fut089",
    "fut020",
]

MI_EFFECTS = [
    (0, "Mi 00: Night Mode"),
    (1, "Mi 01: Rainbow"),
    (2, "Mi 02: Low-Sat Rainbow"),
    (3, "Mi 03: No-Red Rainbow"),
    (4, "Mi 04: Fast Rainbow"),
    (5, "Mi 05: Red-White-Blue"),
    (6, "Mi 06: Pink-Blue-White"),
    (7, "Mi 07: Green-Blue-Red-White"),
    (8, "Mi 08: Slow Random Fade"),
    (9, "Mi 09: Slow Blue-Green Fade"),
]

MiLight = mi_ns.class_("MiLight", light.LightOutput, cg.Component)
LambdaLightEffect = cg.esphome_ns.namespace("light").class_("LambdaLightEffect")
  
@light_effects.register_rgb_effect(
    "mi_effect",
    LambdaLightEffect,
    "MiLight",
    {
        cv.Required(CONF_NAME): cv.string,
        cv.Required(CONF_MI_EFFECT_ID): cv.uint8_t,
        
    },
)

async def mi_light_effect_to_code(config, effect_id):

    lambda_code = cg.RawExpression(f"""
        [](bool initial_run) -> void {{
          if (initial_run) {{
            auto call = {OUTPUT_NAME}->make_call();
            call.set_effect("{config[CONF_NAME]}");
            call.perform();
          }}
      }}, 0xffffffff
    """)

    var_effect = cg.new_Pvariable(
        effect_id,
        config[CONF_NAME],
        lambda_code
    )
    return var_effect

def _inject_mi_effects(value):
    
    print("DEBUG value:", value)
    remote_type = value.get("remote_type")
    print("DEBUG remote_type:", remote_type)
    
    effects = list(value.get(CONF_EFFECTS, []))

    existing = {
        entry.get("mi_effect", {}).get(CONF_MI_EFFECT_ID)
        for entry in effects
            if isinstance(entry, dict) and "mi_effect" in entry
    }
    
    if remote_type in RGB_TYPES:
        for effect_id, name in MI_EFFECTS:
            if effect_id in existing:
                continue

            effects.append({
                "mi_effect": {
                    CONF_NAME: name,
                    CONF_MI_EFFECT_ID: effect_id,
                }
            })
    else:
        
        effect_id = 0
        name = MI_EFFECTS[effect_id][1]
        if effect_id not in existing:
            effects.append({
                "mi_effect": {
                    CONF_NAME: name,
                    CONF_MI_EFFECT_ID: effect_id,
                }
            })

    value[CONF_EFFECTS] = effects
    return value


_BASE_SCHEMA = (
    cv.Schema(
        light.light_schema(MiLight, light.LightType.RGB).extend(
            {
                cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MiLight),
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
)
CONFIG_SCHEMA = cv.All(_inject_mi_effects, _BASE_SCHEMA)


async def to_code(config):
    output_var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(output_var, config)
    
    light_var = cg.new_Pvariable(config[CONF_ID], output_var)

    global OUTPUT_NAME
    OUTPUT_NAME = light_var
    
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
 
   
