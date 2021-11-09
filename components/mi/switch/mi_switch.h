#pragma once

#include "esphome/core/component.h"
#include "esphome/components/mi/mi.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace mi {

class MiSwitch : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_switch_id(uint8_t switch_id) { this->switch_id_ = switch_id; }

  void set_mi_parent(Mi *parent) { this->parent_ = parent; }
  
  void set_switch(uint16_t deviceId, uint8_t groupId, String remoteType, String command);

 protected:
  void write_state(bool state) override;
  
  BulbId bulbId = {0, 0, REMOTE_TYPE_RGB_CCT};
  Mi *parent_;
  String command_;
  uint8_t switch_id_{0};
};

}  // namespace mi
}  // namespace esphome
