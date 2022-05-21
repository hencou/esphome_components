#pragma once

#include "esphome/core/component.h"
#include "esphome/components/itho_cve/itho_cve.h"
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace itho_cve {


  class IthoCVE_Fan : public Component, public fan::Fan {
  public:

       IthoCVE_Fan();
    ~IthoCVE_Fan();

    void setup() override;
    void loop() override;
    void dump_config() override;
    fan::FanTraits get_traits() override;

    void set_ithocve_parent(IthoCVE *parent) { this->parent_ = parent; }

  private:
    void control(const fan::FanCall &call) override;
    void write_state_();

    uint16_t ithoSpeed;

    IthoCVE *parent_;
  };

}  // namespace itho_cve
}  // namespace esphome
