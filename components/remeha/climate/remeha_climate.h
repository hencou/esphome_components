#pragma once

#include "esphome/components/climate/climate.h"

namespace esphome {
namespace remeha {

class Remeha;

class RemehaClimate : public climate::Climate, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_parent(Remeha *parent) { this->parent_ = parent; }
  void set_zone(uint8_t zone) { this->zone_ = zone; }
  void set_time_program_name(int index, const std::string &name) {
    if (index >= 0 && index < 4) this->time_program_names_[index] = name;
  }

  // Called by the parent Remeha component when new data arrives
  void update_current_temperature(float temp);
  void update_target_temperature(float temp);
  void update_zone_mode(uint8_t mode);
  void update_action(uint8_t status_code);
  void update_time_program(uint8_t program);

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  Remeha *parent_{nullptr};
  uint8_t zone_{1};
  std::string time_program_names_[4] = {"Schedule 1", "Schedule 2", "Schedule 3", "Cooling"};
};

}  // namespace remeha
}  // namespace esphome
