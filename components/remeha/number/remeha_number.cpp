#include "remeha_number.h"
#include "../remeha.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remeha {

void RemehaNumber::control(float value) {
  uint32_t raw;
  if (this->is_signed_) {
    int32_t sraw = (int32_t)(value / this->scale_);
    raw = (uint32_t)sraw;  // two's complement for negative values
  } else if (this->scale_ != 1.0f) {
    raw = (uint32_t)(value / this->scale_);
  } else {
    raw = (uint32_t)value;
  }
  this->parent_->write_sdo(this->sdo_index_, this->sdo_subindex_, raw, this->sdo_size_);
}

}  // namespace remeha
}  // namespace esphome
