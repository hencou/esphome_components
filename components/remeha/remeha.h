#pragma once

#include "esphome/core/component.h"
#include "esphome/components/canbus/canbus.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_CLIMATE
#include "esphome/components/climate/climate.h"
#endif

namespace esphome {
namespace remeha {

static const char *const TAG = "remeha";

#ifdef USE_CLIMATE
class RemehaClimate;
#endif
#ifdef USE_SELECT
class RemehaSelect;
#endif

class Remeha : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_canbus(canbus::Canbus *canbus) { this->canbus_ = canbus; }
  void set_boot_delay(uint32_t delay_ms) { this->boot_delay_ms_ = delay_ms; }
  void set_user_level(uint8_t level) { this->user_level_ = level; }

  // SDO write interface (called by RemehaNumber and RemehaSelect)
  bool write_sdo(uint16_t index, uint8_t subindex, uint32_t value, uint8_t size);

  // Register an SDO object for periodic polling
  void add_sdo_poll(uint16_t index, uint8_t subindex);

#ifdef USE_SENSOR
  void set_flow_temperature_sensor(sensor::Sensor *s) { this->flow_temperature_ = s; }
  void set_return_temperature_sensor(sensor::Sensor *s) { this->return_temperature_ = s; }
  void set_outside_temperature_sensor(sensor::Sensor *s) { this->outside_temperature_ = s; }
  void set_outside_temperature_3m_avg_sensor(sensor::Sensor *s) { this->outside_temperature_3m_avg_ = s; }
  void set_outside_temperature_2h_avg_sensor(sensor::Sensor *s) { this->outside_temperature_2h_avg_ = s; }
  void set_setpoint_sensor(sensor::Sensor *s) { this->setpoint_ = s; }
  void set_relative_power_sensor(sensor::Sensor *s) { this->relative_power_ = s; }
  void set_relative_power2_sensor(sensor::Sensor *s) { this->relative_power2_ = s; }
  void set_status_code_sensor(sensor::Sensor *s) { this->status_code_ = s; }
  void set_substatus_code_sensor(sensor::Sensor *s) { this->substatus_code_ = s; }
  void set_water_pressure_sensor(sensor::Sensor *s) { this->water_pressure_ = s; }
  void set_room_temperature_sensor(sensor::Sensor *s) { this->room_temperature_ = s; }
  void set_locking_mode_sensor(sensor::Sensor *s) { this->locking_mode_ = s; }
  void set_blocking_mode_sensor(sensor::Sensor *s) { this->blocking_mode_ = s; }
  void set_error_history_sensor(sensor::Sensor *s) { this->error_history_ = s; }
  void set_diagnostics_sensor(sensor::Sensor *s) { this->diagnostics_ = s; }
  void set_appliance_type_sensor(sensor::Sensor *s) { this->appliance_type_ = s; }
  void set_appliance_variant_sensor(sensor::Sensor *s) { this->appliance_variant_ = s; }
  void set_dhw_temperature_sensor(sensor::Sensor *s) { this->dhw_temperature_ = s; }
  void set_control_temperature_sensor(sensor::Sensor *s) { this->control_temperature_ = s; }
  void set_internal_setpoint_sensor(sensor::Sensor *s) { this->internal_setpoint_ = s; }
  void set_outside_temp_boiler_sensor(sensor::Sensor *s) { this->outside_temp_boiler_ = s; }
  void set_calculated_temperature_sensor(sensor::Sensor *s) { this->calculated_temperature_ = s; }
  void set_boiler_temperature_sensor(sensor::Sensor *s) { this->boiler_temperature_ = s; }
  void set_flue_gas_temperature_sensor(sensor::Sensor *s) { this->flue_gas_temperature_ = s; }
  void set_actual_modulation_sensor(sensor::Sensor *s) { this->actual_modulation_ = s; }
  void set_pump_speed_sensor(sensor::Sensor *s) { this->pump_speed_ = s; }
  void set_flame_current_sensor(sensor::Sensor *s) { this->flame_current_ = s; }
#endif

#ifdef USE_TEXT_SENSOR
  void set_status_text_sensor(text_sensor::TextSensor *s) { this->status_text_ = s; }
  void set_substatus_text_sensor(text_sensor::TextSensor *s) { this->substatus_text_ = s; }
  void set_write_status_text_sensor(text_sensor::TextSensor *s) { this->write_status_ = s; }
#endif

#ifdef USE_NUMBER
  void set_room_setpoint_number(number::Number *n) { this->room_setpoint_ = n; }
  void set_dhw_comfort_setpoint_number(number::Number *n) { this->dhw_comfort_setpoint_ = n; }
  void set_dhw_reduced_setpoint_number(number::Number *n) { this->dhw_reduced_setpoint_ = n; }
  void set_night_setpoint_number(number::Number *n) { this->night_setpoint_ = n; }
  void set_holiday_setpoint_number(number::Number *n) { this->holiday_setpoint_ = n; }
  void set_summer_winter_threshold_number(number::Number *n) { this->summer_winter_threshold_ = n; }
  void set_heating_curve_slope_number(number::Number *n) { this->heating_curve_slope_ = n; }
  void set_room_sensor_calibration_number(number::Number *n) { this->room_sensor_calibration_ = n; }
  void set_anti_legionella_setpoint_number(number::Number *n) { this->anti_legionella_setpoint_ = n; }
#endif

#ifdef USE_SELECT
  void set_zone_mode_select(RemehaSelect *s) { this->zone_mode_ = s; }
  void set_time_program_select(RemehaSelect *s) { this->time_program_ = s; }
  void set_ch_enabled_select(RemehaSelect *s) { this->ch_enabled_ = s; }
  void set_dhw_enabled_select(RemehaSelect *s) { this->dhw_enabled_ = s; }
  void set_anti_legionella_mode_select(RemehaSelect *s) { this->anti_legionella_mode_ = s; }
  void set_fireplace_mode_select(RemehaSelect *s) { this->fireplace_mode_ = s; }
#endif

#ifdef USE_CLIMATE
  void set_climate(RemehaClimate *climate) { this->climate_ = climate; }
#endif

 protected:
  void send_can_(uint32_t can_id, const uint8_t *data, size_t len);
  void handle_frame_(uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                     const std::vector<uint8_t> &data);
  void handle_0x581_(const std::vector<uint8_t> &x);
  void handle_0x1c1_(const std::vector<uint8_t> &x);
  void handle_pdo_0x282_(const std::vector<uint8_t> &x);
  void handle_pdo_0x381_(const std::vector<uint8_t> &x);
  void handle_pdo_0x382_(const std::vector<uint8_t> &x);
  void handle_pdo_0x481_(const std::vector<uint8_t> &x);
  void handle_pdo_0x482_(const std::vector<uint8_t> &x);

  void send_boot_sequence_();
  void start_auth_();
  void poll_next_sdo_();

  static void tea_encrypt_(uint32_t v[2], const uint32_t k[4]);
  static const char *get_status_text_(uint8_t status);
  static const char *get_substatus_text_(uint8_t substatus);

  canbus::Canbus *canbus_{nullptr};

  // Configuration
  uint32_t boot_delay_ms_{10000};
  uint8_t user_level_{2};

  // Boot state
  uint32_t boot_time_ms_{0};
  uint8_t boot_phase_{0};  // 0=waiting, 1=nmt_reset_sent, 2=nmt_start_sent, 3=gw_read_sent, 4=done

  // Timing
  uint32_t last_heartbeat_ms_{0};
  uint32_t last_poll_ms_{0};

  // Gateway state
  bool gateway_enabled_{false};

  // Auth state
  bool authenticated_{false};
  uint8_t auth_step_{0};
  uint32_t auth_start_ms_{0};
  uint32_t auth_serial_{0};
  uint32_t auth_sub1_{0};
  uint32_t auth_sub2_{0};

  // SDO read polling
  struct SdoPollEntry {
    uint16_t index;
    uint8_t subindex;
  };
  std::vector<SdoPollEntry> sdo_poll_list_;
  int sdo_read_step_{0};

  // SDO write state
  bool write_pending_{false};
  uint32_t write_start_ms_{0};

  // Segmented read state (for trending string via 0x501D)
  bool seg_read_active_{false};
  int seg_read_segment_{0};
  uint32_t seg_read_start_ms_{0};
  uint8_t seg_read_buffer_[100]{};
  int seg_read_buffer_pos_{0};

  void process_trending_data_();

#ifdef USE_SENSOR
  sensor::Sensor *flow_temperature_{nullptr};
  sensor::Sensor *return_temperature_{nullptr};
  sensor::Sensor *outside_temperature_{nullptr};
  sensor::Sensor *outside_temperature_3m_avg_{nullptr};
  sensor::Sensor *outside_temperature_2h_avg_{nullptr};
  sensor::Sensor *setpoint_{nullptr};
  sensor::Sensor *relative_power_{nullptr};
  sensor::Sensor *relative_power2_{nullptr};
  sensor::Sensor *status_code_{nullptr};
  sensor::Sensor *substatus_code_{nullptr};
  sensor::Sensor *water_pressure_{nullptr};
  sensor::Sensor *room_temperature_{nullptr};
  sensor::Sensor *locking_mode_{nullptr};
  sensor::Sensor *blocking_mode_{nullptr};
  sensor::Sensor *error_history_{nullptr};
  sensor::Sensor *diagnostics_{nullptr};
  sensor::Sensor *appliance_type_{nullptr};
  sensor::Sensor *appliance_variant_{nullptr};
  sensor::Sensor *dhw_temperature_{nullptr};
  sensor::Sensor *control_temperature_{nullptr};
  sensor::Sensor *internal_setpoint_{nullptr};
  sensor::Sensor *outside_temp_boiler_{nullptr};
  sensor::Sensor *calculated_temperature_{nullptr};
  sensor::Sensor *boiler_temperature_{nullptr};
  sensor::Sensor *flue_gas_temperature_{nullptr};
  sensor::Sensor *actual_modulation_{nullptr};
  sensor::Sensor *pump_speed_{nullptr};
  sensor::Sensor *flame_current_{nullptr};
#endif

#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *status_text_{nullptr};
  text_sensor::TextSensor *substatus_text_{nullptr};
  text_sensor::TextSensor *write_status_{nullptr};
#endif

#ifdef USE_NUMBER
  number::Number *room_setpoint_{nullptr};
  number::Number *dhw_comfort_setpoint_{nullptr};
  number::Number *dhw_reduced_setpoint_{nullptr};
  number::Number *night_setpoint_{nullptr};
  number::Number *holiday_setpoint_{nullptr};
  number::Number *summer_winter_threshold_{nullptr};
  number::Number *heating_curve_slope_{nullptr};
  number::Number *room_sensor_calibration_{nullptr};
  number::Number *anti_legionella_setpoint_{nullptr};
#endif

#ifdef USE_SELECT
  RemehaSelect *zone_mode_{nullptr};
  RemehaSelect *time_program_{nullptr};
  RemehaSelect *ch_enabled_{nullptr};
  RemehaSelect *dhw_enabled_{nullptr};
  RemehaSelect *anti_legionella_mode_{nullptr};
  RemehaSelect *fireplace_mode_{nullptr};
#endif

#ifdef USE_CLIMATE
  RemehaClimate *climate_{nullptr};
#endif
};

}  // namespace remeha
}  // namespace esphome
