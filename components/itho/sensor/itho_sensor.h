#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/itho/itho.h"

namespace esphome {
namespace itho {

class Itho_Sensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }

  void setup() override;
  void dump_config() override;
  void update() override;
  
  void set_itho_parent(Itho *parent) { this->parent_ = parent; }

 protected:
  sensor::Sensor *temperature_sensor_;
  sensor::Sensor *humidity_sensor_;
  
  Itho *parent_;
};

}  // namespace itho
}  // namespace esphome
