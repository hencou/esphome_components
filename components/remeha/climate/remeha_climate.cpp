#include "remeha_climate.h"
#include "../remeha.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remeha {

void RemehaClimate::setup() {
  // Restore previous state if available
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 20.0f;
  }
  this->publish_state();
  this->set_supported_custom_presets({"Klokprogramma 1", "Klokprogramma 2", "Klokprogramma 3"});
}

void RemehaClimate::dump_config() {
  LOG_CLIMATE("", "Remeha Climate", this);
}

climate::ClimateTraits RemehaClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
  //traits.set_supported_custom_presets({"Klokprogramma 1", "Klokprogramma 2", "Klokprogramma 3"});
  traits.set_visual_min_temperature(5.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(0.5f);
  return traits;
}

void RemehaClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto mode = *call.get_mode();
    uint8_t zone_mode;
    switch (mode) {
      case climate::CLIMATE_MODE_OFF:
        zone_mode = 2;
        break;
      case climate::CLIMATE_MODE_HEAT:
        zone_mode = 1;
        break;
      case climate::CLIMATE_MODE_HEAT_COOL:
        zone_mode = 0;
        break;
      default:
        zone_mode = 2;
        break;
    }
    if (this->parent_ != nullptr) {
      // Write zone mode via SDO (0x341F sub 1, uint8)
      this->parent_->write_sdo(0x341F, 0x01, zone_mode, 1);
    }
    this->mode = mode;
  }

  if (call.get_target_temperature().has_value()) {
    float target = *call.get_target_temperature();
    if (this->parent_ != nullptr) {
      // Write CP510 setpoint via SDO (0x3451 sub 1, uint16, scale x10)
      uint16_t raw = (uint16_t)(target * 10.0f);
      this->parent_->write_sdo(0x3451, 0x01, raw, 2);
    }
    this->target_temperature = target;
  }

  if (call.has_custom_preset()) {
    auto custom_preset = call.get_custom_preset();
    if (this->parent_ != nullptr) {
      if (custom_preset == "Klokprogramma 1") {
        this->parent_->write_sdo(0x3458, 0x01, 0, 1);
        this->set_custom_preset_("Klokprogramma 1");
      } else if (custom_preset == "Klokprogramma 2") {
        this->parent_->write_sdo(0x3458, 0x01, 1, 1);
        this->set_custom_preset_("Klokprogramma 2");
      } else if (custom_preset == "Klokprogramma 3") {
        this->parent_->write_sdo(0x3458, 0x01, 2, 1);
        this->set_custom_preset_("Klokprogramma 3");
      }
    }
  }

  this->publish_state();
}

void RemehaClimate::update_current_temperature(float temp) {
  this->current_temperature = temp;
  this->publish_state();
}

void RemehaClimate::update_target_temperature(float temp) {
  this->target_temperature = temp;
  this->publish_state();
}

void RemehaClimate::update_zone_mode(uint8_t mode) {
  switch (mode) {
    case 2:
      this->mode = climate::CLIMATE_MODE_OFF;
      break;
    case 1:
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;
    case 0:
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
      break;
    default:
      break;
  }
  this->publish_state();
}

void RemehaClimate::update_action(uint8_t status_code) {
  switch (status_code) {
    case 0:   // Standby
    case 5:   // Burner off
    case 6:   // Pump active
    case 8:   // Burner shut down
    case 16:  // Frost protection
      this->action = (this->mode == climate::CLIMATE_MODE_OFF)
                         ? climate::CLIMATE_ACTION_OFF
                         : climate::CLIMATE_ACTION_IDLE;
      break;
    case 1:   // Heat request
    case 2:   // Burner ignition
    case 3:   // Heating mode
    case 11:  // Test heat min
    case 12:  // Test heat max
    case 15:  // Manual heat
      this->action = climate::CLIMATE_ACTION_HEATING;
      break;
    case 4:   // DHW mode
    case 13:  // Test DHW max
      // DHW is not directly heating the zone, treat as idle
      this->action = (this->mode == climate::CLIMATE_MODE_OFF)
                         ? climate::CLIMATE_ACTION_OFF
                         : climate::CLIMATE_ACTION_IDLE;
      break;
    case 9:   // Temporary fault
    case 10:  // Permanent fault
      this->action = climate::CLIMATE_ACTION_OFF;
      break;
    default:
      this->action = climate::CLIMATE_ACTION_IDLE;
      break;
  }
  this->publish_state();
}

void RemehaClimate::update_time_program(uint8_t program) {
  switch (program) {
    case 0:
      this->set_custom_preset_("Klokprogramma 1");
      break;
    case 1:
      this->set_custom_preset_("Klokprogramma 2");
      break;
    case 2:
      this->set_custom_preset_("Klokprogramma 3");
      break;
    default:
      this->clear_custom_preset_();
      break;
  }
  this->publish_state();
}

}  // namespace remeha
}  // namespace esphome
