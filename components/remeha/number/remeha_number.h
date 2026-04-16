#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"

namespace esphome {
namespace remeha {

class Remeha;  // forward declaration

class RemehaNumber : public number::Number, public Component {
 public:
  void set_parent(Remeha *parent) { this->parent_ = parent; }
  void set_sdo_index(uint16_t index) { this->sdo_index_ = index; }
  void set_sdo_subindex(uint8_t subindex) { this->sdo_subindex_ = subindex; }
  void set_sdo_size(uint8_t size) { this->sdo_size_ = size; }
  void set_scale(float scale) { this->scale_ = scale; }
  void set_is_signed(bool is_signed) { this->is_signed_ = is_signed; }

  void setup() override {}
  void dump_config() override {}
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;

  Remeha *parent_{nullptr};
  uint16_t sdo_index_{0};
  uint8_t sdo_subindex_{0};
  uint8_t sdo_size_{2};
  float scale_{1.0f};
  bool is_signed_{false};
};

}  // namespace remeha
}  // namespace esphome
