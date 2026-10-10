#include "remeha_select.h"
#include "../remeha.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remeha {

void RemehaSelect::control(const std::string &value) {
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < options.size(); i++) {
    if (options[i] == value) {
      this->parent_->write_sdo(this->sdo_index_, this->sdo_subindex_, (uint32_t)(i + this->value_offset_), 1);
      return;
    }
  }
  char reason[64];
  snprintf(reason, sizeof(reason), "unknown option for 0x%04X: %s", this->sdo_index_, value.c_str());
  this->parent_->report_write_rejected(reason);
}

}  // namespace remeha
}  // namespace esphome
