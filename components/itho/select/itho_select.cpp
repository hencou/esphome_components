#include "itho_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho {

static const char *const TAG = "itho.select";

void Itho_Select::setup() {

  if (this->f_.has_value())
    return;

  std::string value;
  ESP_LOGD(TAG, "Setting up Itho Select");
  if (!this->restore_value_) {
    value = this->initial_option_;
    ESP_LOGD(TAG, "State from initial: %s", value.c_str());
  } else {
    size_t index;
    this->pref_ = global_preferences->make_preference<size_t>(this->get_object_id_hash());
    if (!this->pref_.load(&index)) {
      value = this->initial_option_;
      ESP_LOGD(TAG, "State from initial (could not load): %s", value.c_str());
    } else {
      value = this->traits.get_options().at(index);
      ESP_LOGD(TAG, "State from restore: %s", value.c_str());
    }
  }
  if (value == "medium") value = "auto";

  this->publish_state(value);
}

void Itho_Select::loop() {
  if (parent_->getIthoFanInfo() != ithoFanInfo) {
    ESP_LOGD(TAG, "Updated ithoFanInfo this->state: %s", this->state.c_str());
    ESP_LOGD(TAG, "Updated ithoFanInfo parent_->getIthoFanInfo(): %s", parent_->getIthoFanInfo().c_str());
    ithoFanInfo = parent_->getIthoFanInfo();
    if (this->current_option() == "manual" && parent_->getIthoFanInfo() == "auto") {
      this->publish_state("manual");
    } else {
      this->publish_state(ithoFanInfo);
    }
    ESP_LOGD(TAG, "Updated ithoFanInfo: %s", ithoFanInfo.c_str());
  }
} 

void Itho_Select::update() {}

void Itho_Select::control(const std::string &value) {
  this->set_trigger_->trigger(value);

  if (this->restore_value_) {
    const auto &options = traits.get_options();
    size_t index = std::find(options.begin(), options.end(), value) - options.begin();

    this->pref_.save(&index);
  }
  
if (value == "manual" || value == "auto") {
    const std::string s = "auto";
    bool success = parent_->ithoI2CCommand(0, s);
    this->publish_state(value);
  } else {
    bool success = parent_->ithoI2CCommand(0, value);
  }
}

void Itho_Select::dump_config() {
  LOG_SELECT("", "Itho Select", this);
  LOG_UPDATE_INTERVAL(this);
  if (this->f_.has_value())
    return;
  ESP_LOGCONFIG(TAG, "  Restore Value: %s", YESNO(this->restore_value_));
}

}  // namespace itho
}  // namespace esphome
