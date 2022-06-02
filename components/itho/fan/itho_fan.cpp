#include "itho_fan.h"
#include "esphome/core/log.h"

#include "esphome.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/component.h"
#include "esphome/components/fan/fan_state.h"

namespace esphome {
namespace itho {

static const char *const TAG = "itho.fan";

Itho_Fan::Itho_Fan() {}

void Itho_Fan::loop() {
  if (parent_->getIthoSpeed() != this->speed) {
    this->speed = parent_->getIthoSpeed();
    if(this->speed > 0) {this->state = true;}
    this->publish_state();
  }
}

void Itho_Fan::setup() {
  ESP_LOGD(TAG, "Setup Itho Fan finished");
}

void Itho_Fan::dump_config() { LOG_FAN("", "Speed Fan", this); }

fan::FanTraits Itho_Fan::get_traits() {
  return fan::FanTraits(false, true, false, 255);
}

void Itho_Fan::control(const fan::FanCall &call) {
  if (call.get_speed().has_value()) {
    int speed = *call.get_speed();
    bool success = parent_->setIthoSpeed(speed);
  }
}

}  // namespace itho
}  // namespace esphome
