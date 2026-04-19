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
  void set_time_program_name(int index, const std::string &name) {
    if (index >= 0 && index < 3) this->time_program_names_[index] = name;
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
  std::string time_program_names_[3] = {"Time Program 1", "Time Program 2", "Time Program 3"};
};

}  // namespace remeha
}  // namespace esphome
