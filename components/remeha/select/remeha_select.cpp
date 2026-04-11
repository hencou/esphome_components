#include "remeha_select.h"
#include "../remeha.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remeha {

void RemehaSelect::control(const std::string &value) {
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < options.size(); i++) {
    if (options[i] == value) {
      this->parent_->write_sdo(this->sdo_index_, this->sdo_subindex_, (uint32_t) i, 1);
      return;
    }
  }
  ESP_LOGW(TAG, "Unknown select option: %s", value.c_str());
}

}  // namespace remeha
}  // namespace esphome
