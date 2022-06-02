#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/itho/itho.h"

namespace esphome {
namespace itho {

class Itho_Select : public select::Select, public PollingComponent {
  
 public:
  void set_itho(std::function<optional<std::string>()> &&f) { this->f_ = f; }

  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  Trigger<std::string> *get_set_trigger() const { return this->set_trigger_; }
  void set_initial_option(const std::string &initial_option) { this->initial_option_ = initial_option; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

  void set_itho_parent(Itho *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  std::string initial_option_ = "medium";
  bool restore_value_ = false;
  Trigger<std::string> *set_trigger_ = new Trigger<std::string>();
  optional<std::function<optional<std::string>()>> f_;

  ESPPreferenceObject pref_;

  Itho *parent_;
  std::string ithoFanInfo;
};

}  // namespace itho
}  // namespace esphome
