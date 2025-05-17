#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/itho/itho.h"

namespace esphome {
namespace itho {

class Itho_Sensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_error_sensor(sensor::Sensor *error_sensor) { error_sensor_ = error_sensor; }
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }
  void set_fan_setpoint_sensor(sensor::Sensor *fan_setpoint_sensor) { fan_setpoint_sensor_ = fan_setpoint_sensor; }
  void set_fan_speed_sensor(sensor::Sensor *fan_speed_sensor) { fan_speed_sensor_ = fan_speed_sensor; }

  void setup() override;
  void dump_config() override;
  void update() override;
  
  void set_itho_parent(Itho *parent) { this->parent_ = parent; }

 protected:
  sensor::Sensor *error_sensor_;
  sensor::Sensor *temperature_sensor_;
  sensor::Sensor *humidity_sensor_;
  sensor::Sensor *fan_setpoint_sensor_;
  sensor::Sensor *fan_speed_sensor_;
  
  Itho *parent_;
};

}  // namespace itho
}  // namespace esphome
