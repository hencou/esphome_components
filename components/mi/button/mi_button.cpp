#include "esphome/core/log.h"
#include "mi_button.h"

namespace esphome {
  namespace mi {

    static const char *const TAG = "mi.button";

    void MiButton::setup() {}

    void MiButton::press_action() {
      parent_->handleCommand(bulbId, command_);
    }

    void MiButton::dump_config() {
      LOG_BUTTON("", "Mi Button", this);
    }
    
    void MiButton::set_button(uint16_t deviceId, uint8_t groupId, String remoteType, String command) {
      MiButton::bulbId = {
        deviceId, 
        groupId, 
        MiLightRemoteConfig::fromType(remoteType)->type
      };
      command_ = command;
    }
  }  // namespace mi
}  // namespace esphome
