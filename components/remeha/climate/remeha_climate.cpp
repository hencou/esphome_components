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
  this->set_supported_custom_presets({this->time_program_names_[0].c_str(), this->time_program_names_[1].c_str(),
                                      this->time_program_names_[2].c_str(), this->time_program_names_[3].c_str()});
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
  traits.add_supported_mode(climate::CLIMATE_MODE_AUTO);
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
      case climate::CLIMATE_MODE_AUTO:
        zone_mode = 0;
        break;
      default:
        zone_mode = 2;
        break;
    }
    if (this->parent_ != nullptr) {
      // Write zone mode via SDO (0x341F, uint8)
      this->parent_->write_sdo(0x341F, this->zone_, zone_mode, 1);
    }
    this->mode = mode;
  }

  if (call.get_target_temperature().has_value()) {
    float target = *call.get_target_temperature();
    if (this->parent_ != nullptr) {
      // Write room setpoint via SDO (0x3451, uint16, scale x10)
      uint16_t raw = (uint16_t)(target * 10.0f);
      this->parent_->write_sdo(0x3451, this->zone_, raw, 2);
    }
    this->target_temperature = target;
  }

  if (call.has_custom_preset()) {
    auto custom_preset = call.get_custom_preset();
    if (this->parent_ != nullptr) {
      for (int i = 0; i < 4; i++) {
        if (custom_preset == this->time_program_names_[i]) {
          this->parent_->write_sdo(0x3458, this->zone_, i, 1);
          this->set_custom_preset_(this->time_program_names_[i].c_str());
          break;
        }
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
    case 3:  // Temporary override of the schedule
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;
    case 0:
      this->mode = climate::CLIMATE_MODE_AUTO;
      break;
    default:
      break;
  }
  this->publish_state();
}

void RemehaClimate::update_action(uint8_t status_code) {
  switch (status_code) {
    case 0:   // Standby
    case 5:   // Generator stop
    case 6:   // Pump post run
    case 8:   // Controlled stop
    case 16:  // Frost protection
      this->action = (this->mode == climate::CLIMATE_MODE_OFF)
                         ? climate::CLIMATE_ACTION_OFF
                         : climate::CLIMATE_ACTION_IDLE;
      break;
    case 1:   // Heat demand
    case 2:   // Generator start
    case 3:   // Generator CH
    case 11:  // Load test min
    case 12:  // Load test CH max
    case 15:  // Manual heat demand
      this->action = climate::CLIMATE_ACTION_HEATING;
      break;
    case 4:   // Generator DHW
    case 13:  // Load test DHW max
      // DHW is not directly heating the zone, treat as idle
      this->action = (this->mode == climate::CLIMATE_MODE_OFF)
                         ? climate::CLIMATE_ACTION_OFF
                         : climate::CLIMATE_ACTION_IDLE;
      break;
    case 9:   // Blocking mode
    case 10:  // Locking mode
      this->action = climate::CLIMATE_ACTION_OFF;
      break;
    default:
      this->action = climate::CLIMATE_ACTION_IDLE;
      break;
  }
  this->publish_state();
}

void RemehaClimate::update_time_program(uint8_t program) {
  if (program < 4) {
    this->set_custom_preset_(this->time_program_names_[program].c_str());
  } else {
    this->clear_custom_preset_();
  }
  this->publish_state();
}

}  // namespace remeha
}  // namespace esphome
