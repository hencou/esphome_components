#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/itho/itho.h"

namespace esphome {
namespace itho {

class Itho_Sensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_error_sensor(sensor::Sensor *error_sensor) { error_sensor_ = error_sensor; }
  void set_startup_counter_sensor(sensor::Sensor *startup_counter_sensor) { startup_counter_sensor_ = startup_counter_sensor; }
  void set_operation_time_sensor(sensor::Sensor *operation_time_sensor) { operation_time_sensor_ = operation_time_sensor; }
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }
  void set_fan_setpoint_sensor(sensor::Sensor *fan_setpoint_sensor) { fan_setpoint_sensor_ = fan_setpoint_sensor; }
  void set_fan_speed_sensor(sensor::Sensor *fan_speed_sensor) { fan_speed_sensor_ = fan_speed_sensor; }
  void set_air_quality_sensor(sensor::Sensor *air_quality_sensor) { air_quality_sensor_ = air_quality_sensor; }
  void set_co2_sensor(sensor::Sensor *co2_sensor) { co2_sensor_ = co2_sensor; }
  void set_indoor_humidity_sensor(sensor::Sensor *indoor_humidity_sensor) { indoor_humidity_sensor_ = indoor_humidity_sensor; }
  void set_outdoor_humidity_sensor(sensor::Sensor *outdoor_humidity_sensor) { outdoor_humidity_sensor_ = outdoor_humidity_sensor; }
  void set_exhaust_temperature_sensor(sensor::Sensor *exhaust_temperature_sensor) { exhaust_temperature_sensor_ = exhaust_temperature_sensor; }
  void set_supply_temperature_sensor(sensor::Sensor *supply_temperature_sensor) { supply_temperature_sensor_ = supply_temperature_sensor; }
  void set_indoor_temperature_sensor(sensor::Sensor *indoor_temperature_sensor) { indoor_temperature_sensor_ = indoor_temperature_sensor; }
  void set_outdoor_temperature_sensor(sensor::Sensor *outdoor_temperature_sensor) { outdoor_temperature_sensor_ = outdoor_temperature_sensor; }
  void set_speed_cap_sensor(sensor::Sensor *speed_cap_sensor) { speed_cap_sensor_ = speed_cap_sensor; }
  void set_bypass_position_sensor(sensor::Sensor *bypass_position_sensor) { bypass_position_sensor_ = bypass_position_sensor; }
  void set_exhaust_fan_speed_sensor(sensor::Sensor *exhaust_fan_speed_sensor) { exhaust_fan_speed_sensor_ = exhaust_fan_speed_sensor; }
  void set_supply_fan_speed_sensor(sensor::Sensor *supply_fan_speed_sensor) { supply_fan_speed_sensor_ = supply_fan_speed_sensor; }
  void set_remaining_time_sensor(sensor::Sensor *remaining_time_sensor) { remaining_time_sensor_ = remaining_time_sensor; }
  void set_post_heat_sensor(sensor::Sensor *post_heat_sensor) { post_heat_sensor_ = post_heat_sensor; }
  void set_pre_heat_sensor(sensor::Sensor *pre_heat_sensor) { pre_heat_sensor_ = pre_heat_sensor; }
  void set_supply_flow_sensor(sensor::Sensor *supply_flow_sensor) { supply_flow_sensor_ = supply_flow_sensor; }
  void set_exhaust_flow_sensor(sensor::Sensor *exhaust_flow_sensor) { exhaust_flow_sensor_ = exhaust_flow_sensor; }
  void set_internal_fault_sensor(sensor::Sensor *internal_fault_sensor) { internal_fault_sensor_ = internal_fault_sensor; }
  void set_frost_cycle_sensor(sensor::Sensor *frost_cycle_sensor) { frost_cycle_sensor_ = frost_cycle_sensor; }
  void set_filter_dirty_sensor(sensor::Sensor *filter_dirty_sensor) { filter_dirty_sensor_ = filter_dirty_sensor; }

  void setup() override;
  void dump_config() override;
  void update() override;
  
  void set_itho_parent(Itho *parent) { this->parent_ = parent; }

 protected:
  sensor::Sensor *error_sensor_;
  sensor::Sensor *startup_counter_sensor_;
  sensor::Sensor *operation_time_sensor_;
  sensor::Sensor *temperature_sensor_;
  sensor::Sensor *humidity_sensor_;
  sensor::Sensor *fan_setpoint_sensor_;
  sensor::Sensor *fan_speed_sensor_;
  sensor::Sensor *air_quality_sensor_{nullptr};
  sensor::Sensor *co2_sensor_{nullptr};
  sensor::Sensor *indoor_humidity_sensor_{nullptr};
  sensor::Sensor *outdoor_humidity_sensor_{nullptr};
  sensor::Sensor *exhaust_temperature_sensor_{nullptr};
  sensor::Sensor *supply_temperature_sensor_{nullptr};
  sensor::Sensor *indoor_temperature_sensor_{nullptr};
  sensor::Sensor *outdoor_temperature_sensor_{nullptr};
  sensor::Sensor *speed_cap_sensor_{nullptr};
  sensor::Sensor *bypass_position_sensor_{nullptr};
  sensor::Sensor *exhaust_fan_speed_sensor_{nullptr};
  sensor::Sensor *supply_fan_speed_sensor_{nullptr};
  sensor::Sensor *remaining_time_sensor_{nullptr};
  sensor::Sensor *post_heat_sensor_{nullptr};
  sensor::Sensor *pre_heat_sensor_{nullptr};
  sensor::Sensor *supply_flow_sensor_{nullptr};
  sensor::Sensor *exhaust_flow_sensor_{nullptr};
  sensor::Sensor *internal_fault_sensor_{nullptr};
  sensor::Sensor *frost_cycle_sensor_{nullptr};
  sensor::Sensor *filter_dirty_sensor_{nullptr};
  
  Itho *parent_;
};

}  // namespace itho
}  // namespace esphome
