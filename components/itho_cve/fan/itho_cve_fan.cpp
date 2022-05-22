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
  if (parent_->ithoGetSpeed() != this->speed && !parent_->ithoGetSpeedUpdated()) {
    this->speed = parent_->ithoGetSpeed();
    if(this->speed > 0) {this->state = true;}
    this->publish_state();
  }
}

void IthoCVE_Fan::setup() {
  ESP_LOGD(TAG, "Setup Itho Fan finished");
}

void IthoCVE_Fan::dump_config() { LOG_FAN("", "Speed Fan", this); }

fan::FanTraits IthoCVE_Fan::get_traits() {
  return fan::FanTraits(false, true, false, 255);
}

void IthoCVE_Fan::control(const fan::FanCall &call) {
  if (call.get_speed().has_value()) {
    int speed = *call.get_speed();
    bool success = parent_->ithoSetSpeed(speed);
  }
}

}  // namespace itho_cve
}  // namespace esphome
