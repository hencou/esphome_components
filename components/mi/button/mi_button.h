#pragma once

#include "esphome/core/component.h"
#include "esphome/components/mi/mi.h"
#include "esphome/components/button/button.h"

namespace esphome {
  namespace mi {

    class MiButton : public button::Button, public Component {
     public:
      void setup() override;
      void dump_config() override;
      void set_button_id(uint8_t button_id) { this->button_id_ = button_id; }

      void set_mi_parent(Mi *parent) { this->parent_ = parent; }
      
      void set_button(uint16_t deviceId, uint8_t groupId, std::string remoteType, std::string command);

     protected:
      void press_action() override;
      
      BulbId bulbId = {0, 0, REMOTE_TYPE_RGB_CCT};
      Mi *parent_;
      std::string command_;
      uint8_t button_id_{0};
    };
  }  // namespace mi
}  // namespace esphome
