import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome import automation
from esphome.const import (
  CONF_ID,
  CONF_TRIGGER_ID,
)

AUTO_LOAD = ["json","spi"]

mi_ns = cg.esphome_ns.namespace("mi")
Mi = mi_ns.class_("Mi", cg.Component)

INTERFACE_TYPES = {
  "lt8900" : "lt8900",
  "nrf24" : "nrf24",
}

POWER_LEVELS = {
  "MIN" : "MIN",
  "LOW" : "LOW",
  "HIGH" : "HIGH",
  "MAX" : "MAX",
}

CHANNELS = {
  "LOW" : "LOW",
  "MID" : "MID",
  "HIGH" : "HIGH",
}

CONF_CE_PIN = "ce_pin"
CONF_CSN_PIN = "csn_pin"
CONF_RESET_PIN = "reset_pin"
CONF_RADIO_INTERFACE_TYPE = "radio_interface_type"
CONF_PACKET_REPEATS = "packet_repeats"
CONF_LISTEN_REPEATS = "listen_repeats"
CONF_STATE_FLUSH_INTERVAL = "state_flush_interval"
CONF_PACKET_REPEAT_THROTTLE_THRESHOLD = "packet_repeat_throttle_threshold"
CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY = "packet_repeat_throttle_sensitivity"
CONF_PACKET_REPEAT_MINIMUM = "packet_repeat_minimum"
CONF_ENABLE_AUTOMATIC_MODE_SWITCHING = "enable_automatic_mode_switching"
CONF_RF24_POWER_LEVEL = "rf24_power_level"
CONF_RF24_CHANNELS = "rf24_channels"
CONF_RF24_LISTEN_CHANNEL = "rf24_listen_channel"
CONF_PACKET_REPEATS_PER_LOOP = "packet_repeats_per_loop"
CONF_ON_COMMAND_RECEIVED = "on_command_received"
CONF_RESEND_LAST_COMMAND = "resend_last_command"

MiBridgeData = mi_ns.struct("MiBridgeData")
MiBridgeReceivedCodeTrigger = mi_ns.class_(
  "MiBridgeReceivedCodeTrigger", automation.Trigger.template(MiBridgeData)
)

CONF_MI_ID = "mi_id"
CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(Mi),
      cv.Required(CONF_CE_PIN): pins.gpio_output_pin_schema,
      cv.Required(CONF_CSN_PIN): pins.gpio_output_pin_schema,
      cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
      cv.Optional(CONF_RADIO_INTERFACE_TYPE) : cv.enum(INTERFACE_TYPES),
      cv.Optional(CONF_PACKET_REPEATS) : cv.uint16_t,
      cv.Optional(CONF_LISTEN_REPEATS) : cv.uint8_t,
      cv.Optional(CONF_STATE_FLUSH_INTERVAL): cv.int_range(min=1000, max=20000),
      cv.Optional(CONF_PACKET_REPEAT_THROTTLE_THRESHOLD) : cv.uint16_t,
      cv.Optional(CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY) : cv.uint16_t,
      cv.Optional(CONF_PACKET_REPEAT_MINIMUM) : cv.uint16_t,
      cv.Optional(CONF_ENABLE_AUTOMATIC_MODE_SWITCHING) : cv.boolean,
      cv.Optional(CONF_RF24_POWER_LEVEL) : cv.enum(POWER_LEVELS),
      cv.Optional(CONF_RF24_CHANNELS) : cv.All(cv.ensure_list(cv.enum(CHANNELS)), cv.Length(min=1, max=3)),
      cv.Optional(CONF_RF24_LISTEN_CHANNEL) : cv.enum(CHANNELS),
      cv.Optional(CONF_PACKET_REPEATS_PER_LOOP) : cv.uint16_t,
      cv.Optional(CONF_RESEND_LAST_COMMAND) : cv.boolean,
      cv.Optional(CONF_ON_COMMAND_RECEIVED): automation.validate_automation(
          {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                MiBridgeReceivedCodeTrigger
            ),
          }
      ),
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add_library("https://github.com/luisllamasbinaburo/Arduino-List", None)
    cg.add_library("nrf24/RF24", "1.4.5")
    cg.add_library("PathVariableHandlers", None)
    cg.add_library("StreamUtils", None)
    
    ce_pin = await cg.gpio_pin_expression(config[CONF_CE_PIN])
    cg.add(var.set_ce_pin(ce_pin))
    csn_pin = await cg.gpio_pin_expression(config[CONF_CSN_PIN])
    cg.add(var.set_csn_pin(csn_pin))
    
    if CONF_RESET_PIN in config:
      reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
      cg.add(var.set_reset_pin(reset_pin))

    if CONF_RADIO_INTERFACE_TYPE in config:
      cg.add(var.set_radio_interface_type(config[CONF_RADIO_INTERFACE_TYPE]))
      
    if CONF_PACKET_REPEATS in config:
      cg.add(var.set_packet_repeats(config[CONF_PACKET_REPEATS]))
      
    if CONF_LISTEN_REPEATS in config:
      cg.add(var.set_listen_repeats(config[CONF_LISTEN_REPEATS]))
      
    if CONF_STATE_FLUSH_INTERVAL in config:
      cg.add(var.set_state_flush_interval(config[CONF_STATE_FLUSH_INTERVAL]))
      
    if CONF_PACKET_REPEAT_THROTTLE_THRESHOLD in config:
      cg.add(var.set_packet_repeat_throttle_threshold(config[CONF_PACKET_REPEAT_THROTTLE_THRESHOLD]))
    
    if CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY in config:
      cg.add(var.set_packet_repeat_throttle_sensitivity(config[CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY]))
      
    if CONF_PACKET_REPEAT_MINIMUM in config:
      cg.add(var.set_packet_repeat_minimum(config[CONF_PACKET_REPEAT_MINIMUM]))
      
    if CONF_ENABLE_AUTOMATIC_MODE_SWITCHING in config:
      cg.add(var.set_enable_automatic_mode_switching(config[CONF_ENABLE_AUTOMATIC_MODE_SWITCHING]))
      
    if CONF_RF24_POWER_LEVEL in config:
      cg.add(var.set_rf24_power_level(config[CONF_RF24_POWER_LEVEL]))
      
    if CONF_RF24_LISTEN_CHANNEL in config:
      cg.add(var.set_rf24_listen_channel(config[CONF_RF24_LISTEN_CHANNEL]))
    
    if CONF_PACKET_REPEATS_PER_LOOP in config:
      cg.add(var.set_packet_repeats_per_loop(config[CONF_PACKET_REPEATS_PER_LOOP]))
      
    if CONF_RESEND_LAST_COMMAND in config:
      cg.add(var.set_resend_last_command(config[CONF_RESEND_LAST_COMMAND]))

    if CONF_RF24_CHANNELS in config:
      cg.add(var.del_rf24_channels())
      for channel in config.get(CONF_RF24_CHANNELS, []):
        cg.add(var.add_rf24_channel(channel))
    
    if CONF_ON_COMMAND_RECEIVED in config:
      for conf in config.get(CONF_ON_COMMAND_RECEIVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(MiBridgeData, "data")], conf)
