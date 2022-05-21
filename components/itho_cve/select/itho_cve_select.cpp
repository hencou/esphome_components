#include "itho_cve_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_cve {

static const char *const TAG = "itho_cve.select";

void IthoCVE_Select::setup() {

  if (this->f_.has_value())
    return;

  std::string value;
  ESP_LOGD(TAG, "Setting up IthoCVE Select");
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

  this->publish_state(value);
}

void IthoCVE_Select::loop() {
  if (parent_->ithoGetFanInfo() != ithoFanInfo) {
    ithoFanInfo = parent_->ithoGetFanInfo();
    
    if (strcmp(ithoFanInfo.c_str(), "auto") == 0) {
      this->publish_state("medium");
    } else {
      this->publish_state(ithoFanInfo);
    }
  
    ESP_LOGD(TAG, "Updated ithoFanInfo: %s", ithoFanInfo.c_str());
  }
} 

void IthoCVE_Select::update() {}

void IthoCVE_Select::control(const std::string &value) {
  this->set_trigger_->trigger(value);

  if (this->restore_value_) {
    auto options = this->traits.get_options();
    size_t index = std::find(options.begin(), options.end(), value) - options.begin();

    this->pref_.save(&index);
  }

  bool success = parent_->ithoI2CCommand(0, value);
}

void IthoCVE_Select::dump_config() {
  LOG_SELECT("", "IthoCVE Select", this);
  LOG_UPDATE_INTERVAL(this);
  if (this->f_.has_value())
    return;
  ESP_LOGCONFIG(TAG, "  Restore Value: %s", YESNO(this->restore_value_));
}

}  // namespace itho_cve
}  // namespace esphome
