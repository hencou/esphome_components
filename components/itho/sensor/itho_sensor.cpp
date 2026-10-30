#include "itho_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho {

static const char *const TAG = "itho.sensor";

void Itho_Sensor::setup() {
    ESP_LOGD(TAG, "Sensor initialized");
}

void Itho_Sensor::dump_config() {
    ESP_LOGCONFIG(TAG, "itho:");
  
    LOG_UPDATE_INTERVAL(this);
    LOG_SENSOR(TAG, "Error", this->error_sensor_);
    LOG_SENSOR(TAG, "Startup counter", this->startup_counter_sensor_);
    LOG_SENSOR(TAG, "Operation time", this->operation_time_sensor_);
    LOG_SENSOR(TAG, "Temperature", this->temperature_sensor_);
    LOG_SENSOR(TAG, "Humidity", this->humidity_sensor_);
    LOG_SENSOR(TAG, "Fan setpoint", this->fan_setpoint_sensor_);
    LOG_SENSOR(TAG, "Fan speed", this->fan_speed_sensor_);
    LOG_SENSOR(TAG, "Air quality", this->air_quality_sensor_);
    LOG_SENSOR(TAG, "CO2", this->co2_sensor_);
    LOG_SENSOR(TAG, "Indoor humidity", this->indoor_humidity_sensor_);
    LOG_SENSOR(TAG, "Outdoor humidity", this->outdoor_humidity_sensor_);
    LOG_SENSOR(TAG, "Exhaust temperature", this->exhaust_temperature_sensor_);
    LOG_SENSOR(TAG, "Supply temperature", this->supply_temperature_sensor_);
    LOG_SENSOR(TAG, "Indoor temperature", this->indoor_temperature_sensor_);
    LOG_SENSOR(TAG, "Outdoor temperature", this->outdoor_temperature_sensor_);
    LOG_SENSOR(TAG, "Speed cap", this->speed_cap_sensor_);
    LOG_SENSOR(TAG, "Bypass position", this->bypass_position_sensor_);
    LOG_SENSOR(TAG, "Exhaust fan speed", this->exhaust_fan_speed_sensor_);
    LOG_SENSOR(TAG, "Supply fan speed", this->supply_fan_speed_sensor_);
    LOG_SENSOR(TAG, "Remaining time", this->remaining_time_sensor_);
    LOG_SENSOR(TAG, "Post heat", this->post_heat_sensor_);
    LOG_SENSOR(TAG, "Pre heat", this->pre_heat_sensor_);
    LOG_SENSOR(TAG, "Supply flow", this->supply_flow_sensor_);
    LOG_SENSOR(TAG, "Exhaust flow", this->exhaust_flow_sensor_);
    LOG_SENSOR(TAG, "Internal fault", this->internal_fault_sensor_);
    LOG_SENSOR(TAG, "Frost cycle", this->frost_cycle_sensor_);
    LOG_SENSOR(TAG, "Filter dirty", this->filter_dirty_sensor_);
}

void Itho_Sensor::update() {
    if (this->error_sensor_ != nullptr) {
        const float error = parent_->getIthoError();
        this->error_sensor_->publish_state(error);
    }

    if (this->startup_counter_sensor_ != nullptr) {
        const float startup_counter = parent_->getIthoStartupCounter();
        this->startup_counter_sensor_->publish_state(startup_counter);
    }

    if (this->operation_time_sensor_ != nullptr) {
        const float operation_time = parent_->getIthoOperationTime();
        this->operation_time_sensor_->publish_state(operation_time);
    }

    if (this->temperature_sensor_ != nullptr) {
        const float temperature = parent_->getIthoTemperature();
        this->temperature_sensor_->publish_state(temperature);
    }

    if (this->humidity_sensor_ != nullptr) {
        const float humidity = parent_->getIthoHumidity();
        this->humidity_sensor_->publish_state(humidity);
    }

    if (this->fan_setpoint_sensor_ != nullptr) {
        const float fan_setpoint = parent_->getIthoFanSetpoint();
        this->fan_setpoint_sensor_->publish_state(fan_setpoint);
    }

    if (this->fan_speed_sensor_ != nullptr) {
        const float fan_speed = parent_->getIthoFanSpeed();
        this->fan_speed_sensor_->publish_state(fan_speed);
    }

    if (this->air_quality_sensor_ != nullptr) {
        this->air_quality_sensor_->publish_state(parent_->getIthoAirQuality());
    }

    if (this->co2_sensor_ != nullptr) {
        this->co2_sensor_->publish_state(parent_->getIthoCO2());
    }

    if (this->indoor_humidity_sensor_ != nullptr) {
        this->indoor_humidity_sensor_->publish_state(parent_->getIthoIndoorHumidity());
    }

    if (this->outdoor_humidity_sensor_ != nullptr) {
        this->outdoor_humidity_sensor_->publish_state(parent_->getIthoOutdoorHumidity());
    }

    if (this->exhaust_temperature_sensor_ != nullptr) {
        this->exhaust_temperature_sensor_->publish_state(parent_->getIthoExhaustTemperature());
    }

    if (this->supply_temperature_sensor_ != nullptr) {
        this->supply_temperature_sensor_->publish_state(parent_->getIthoSupplyTemperature());
    }

    if (this->indoor_temperature_sensor_ != nullptr) {
        this->indoor_temperature_sensor_->publish_state(parent_->getIthoIndoorTemperature());
    }

    if (this->outdoor_temperature_sensor_ != nullptr) {
        this->outdoor_temperature_sensor_->publish_state(parent_->getIthoOutdoorTemperature());
    }

    if (this->speed_cap_sensor_ != nullptr) {
        this->speed_cap_sensor_->publish_state(parent_->getIthoSpeedCap());
    }

    if (this->bypass_position_sensor_ != nullptr) {
        this->bypass_position_sensor_->publish_state(parent_->getIthoBypassPosition());
    }

    if (this->exhaust_fan_speed_sensor_ != nullptr) {
        this->exhaust_fan_speed_sensor_->publish_state(parent_->getIthoExhaustFanSpeed());
    }

    if (this->supply_fan_speed_sensor_ != nullptr) {
        this->supply_fan_speed_sensor_->publish_state(parent_->getIthoSupplyFanSpeed());
    }

    if (this->remaining_time_sensor_ != nullptr) {
        this->remaining_time_sensor_->publish_state(parent_->getIthoRemainingTime());
    }

    if (this->post_heat_sensor_ != nullptr) {
        this->post_heat_sensor_->publish_state(parent_->getIthoPostHeat());
    }

    if (this->pre_heat_sensor_ != nullptr) {
        this->pre_heat_sensor_->publish_state(parent_->getIthoPreHeat());
    }

    if (this->supply_flow_sensor_ != nullptr) {
        this->supply_flow_sensor_->publish_state(parent_->getIthoSupplyFlow());
    }

    if (this->exhaust_flow_sensor_ != nullptr) {
        this->exhaust_flow_sensor_->publish_state(parent_->getIthoExhaustFlow());
    }

    if (this->internal_fault_sensor_ != nullptr) {
        this->internal_fault_sensor_->publish_state(parent_->getIthoInternalFault());
    }

    if (this->frost_cycle_sensor_ != nullptr) {
        this->frost_cycle_sensor_->publish_state(parent_->getIthoFrostCycle());
    }

    if (this->filter_dirty_sensor_ != nullptr) {
        this->filter_dirty_sensor_->publish_state(parent_->getIthoFilterDirty());
    }
}

}  // namespace itho
}  // namespace esphome
