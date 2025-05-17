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
}

}  // namespace itho
}  // namespace esphome
