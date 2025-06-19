#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#include <Arduino.h>

namespace esphome {
namespace canbus_gvret {

const char *const TAG = "canbus_gvret";

class CanbusGVRET : public canbus::Canbus {

 bool is_initialized = false;
 
 public:
  canbus::Canbus *canbus;

  CanbusGVRET(canbus::Canbus *canbus) {
    this->canbus = canbus;
  };

  void displayFrame(uint32_t can_id, bool use_extended_id, bool remote_transmission_request, const std::vector<uint8_t> &data);

  float get_setup_priority() const override;
  void setup();
  void loop() override;

 protected:
  bool setup_internal() override;
  canbus::Error send_message(struct canbus::CanFrame *frame) override;
  canbus::Error read_message(struct canbus::CanFrame *frame) override;

 private:
  void trigger(uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
               const std::vector<uint8_t> &data);

};
}  // namespace canbus_gvret
}  // namespace esphome
