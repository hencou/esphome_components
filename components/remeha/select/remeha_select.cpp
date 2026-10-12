#include "remeha_select.h"
#include "../remeha.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remeha {

void RemehaSelect::control(const std::string &value) {
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < options.size(); i++) {
    if (options[i] == value) {
      uint8_t raw = (uint8_t) (i + this->value_offset_);
      this->parent_->write_sdo(this->sdo_index_, this->sdo_subindex_, raw, 1);
      if (this->sdo_index_ == 0x341F)
        this->parent_->set_zone_mode_cache(this->sdo_subindex_, raw);
      return;
    }
  }
  char reason[64];
  snprintf(reason, sizeof(reason), "unknown option for 0x%04X: %s", this->sdo_index_, value.c_str());
  this->parent_->report_write_rejected(reason);
}

}  // namespace remeha
}  // namespace esphome
