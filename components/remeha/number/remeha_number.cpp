#include "remeha_number.h"
#include "../remeha.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace remeha {

void RemehaNumber::publish_from_sdo(uint32_t raw) {
  float value;
  if (this->sdo_size_ == 1) {
    value = this->is_signed_ ? (float) (int8_t) (raw & 0xFF) : (float) (raw & 0xFF);
  } else if (this->sdo_size_ == 2) {
    value = this->is_signed_ ? (float) (int16_t) (raw & 0xFFFF) : (float) (raw & 0xFFFF);
  } else {
    value = this->is_signed_ ? (float) (int32_t) raw : (float) raw;
  }
  value *= this->scale_;
  ESP_LOGD(TAG, "Number 0x%04X sub %d = %.2f (raw=%u)", this->sdo_index_, this->sdo_subindex_, value,
           (unsigned) raw);
  this->publish_state(value);
}

void RemehaNumber::control(float value) {
  int32_t scaled = (int32_t) lroundf(value / this->scale_);
  uint32_t raw = (uint32_t) scaled;  // two's complement for negative values
  this->parent_->write_sdo(this->sdo_index_, this->sdo_subindex_, raw, this->sdo_size_);
}

}  // namespace remeha
}  // namespace esphome
