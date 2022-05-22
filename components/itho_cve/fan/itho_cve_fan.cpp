#include "itho_cve_fan.h"
#include "esphome/core/log.h"

#include "esphome.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/component.h"
#include "esphome/components/fan/fan_state.h"

namespace esphome {
namespace itho_cve {

static const char *const TAG = "itho_cve.fan";

IthoCVE_Fan::IthoCVE_Fan() {}

void IthoCVE_Fan::loop() {
  if (parent_->ithoGetSpeed() != this->speed && !busy && !parent_->ithoGetSpeedUpdated()) {
    this->speed = parent_->ithoGetSpeed();
    if(this->speed > 0) {this->state = true;}
    this->publish_state();
    ESP_LOGD(TAG, "Updated this->speed: %d", this->speed);
  }
}

void IthoCVE_Fan::setup() {

  ESP_LOGD(TAG, "Setup Itho Fan start");
  
  busy = true;
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(*this);
    
    this->write_state_();
  }

  ESP_LOGD(TAG, "Setup Itho Fan finished");
}

void IthoCVE_Fan::dump_config() { LOG_FAN("", "Speed Fan", this); }

fan::FanTraits IthoCVE_Fan::get_traits() {
  return fan::FanTraits(false, true, false, 255);
}

void IthoCVE_Fan::control(const fan::FanCall &call) {
  busy = true;
  if (call.get_state().has_value())
    this->state = *call.get_state();
  if (call.get_speed().has_value())
    this->speed = *call.get_speed();

  this->write_state_();
  //this->publish_state();
  
}

void IthoCVE_Fan::write_state_() {
  int speed = this->state ? this->speed : 0;
  bool success = parent_->ithoSetSpeed(speed);
  busy = false;
}

}  // namespace itho_cve
}  // namespace esphome
