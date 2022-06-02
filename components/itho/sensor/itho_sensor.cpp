#include "itho_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace itho {

static const char *const TAG = "itho.sensor";

void Itho_Sensor::setup() {
    ESP_LOGD(TAG, "Sensor initialized");
}

void Itho_Sensor::dump_config() {
    ESP_LOGCONFIG(TAG, "itho:");
  
    LOG_UPDATE_INTERVAL(this);
    LOG_SENSOR(TAG, "Temperature", this->temperature_sensor_);
    LOG_SENSOR(TAG, "Humidity", this->humidity_sensor_);
}

void Itho_Sensor::update() {
    if (this->temperature_sensor_ != nullptr) {
        const float temperature = parent_->getIthoTemperature();
        this->temperature_sensor_->publish_state(temperature);
    }

    if (this->humidity_sensor_ != nullptr) {
        const float humidity = parent_->getIthoHumidity();
        this->humidity_sensor_->publish_state(humidity);
    }
}

}  // namespace itho
}  // namespace esphome
