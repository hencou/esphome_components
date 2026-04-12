#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome {
namespace remeha {

class Remeha;  // forward declaration

class RemehaSelect : public select::Select, public Component {
 public:
  void set_parent(Remeha *parent) { this->parent_ = parent; }
  void set_sdo_index(uint16_t index) { this->sdo_index_ = index; }
  void set_sdo_subindex(uint8_t subindex) { this->sdo_subindex_ = subindex; }
  void set_value_offset(uint8_t offset) { this->value_offset_ = offset; }

  void setup() override {}
  void dump_config() override {}
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(const std::string &value) override;

  Remeha *parent_{nullptr};
  uint16_t sdo_index_{0};
  uint8_t sdo_subindex_{0};
  uint8_t value_offset_{0};
};

}  // namespace remeha
}  // namespace esphome
