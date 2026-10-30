#pragma once

#include "esphome/core/component.h"
#include "esphome/components/itho/itho.h"
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace itho {


  class Itho_Fan : public Component, public fan::Fan {
  public:

       Itho_Fan();
    ~Itho_Fan();

    void setup() override;
    void loop() override;
    void dump_config() override;
    fan::FanTraits get_traits() override;

    void set_itho_parent(Itho *parent) { this->parent_ = parent; }

  private:
    void control(const fan::FanCall &call) override;
    Itho *parent_;
  };

}  // namespace itho
}  // namespace esphome
