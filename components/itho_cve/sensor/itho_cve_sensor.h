#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/itho_cve/itho_cve.h"

namespace esphome {
namespace itho_cve {

class IthoCVE_Sensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }

  void setup() override;
  void dump_config() override;
  void update() override;
  
  void set_itho_cve_parent(IthoCVE *parent) { this->parent_ = parent; }

 protected:
  sensor::Sensor *temperature_sensor_;
  sensor::Sensor *humidity_sensor_;
  
  IthoCVE *parent_;
};

}  // namespace itho_cve
}  // namespace esphome
