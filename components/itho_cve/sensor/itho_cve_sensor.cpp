#include "itho_cve_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace itho_cve {

static const char *const TAG = "itho_cve.sensor";

void IthoCVE_Sensor::setup() {
    ESP_LOGD(TAG, "Sensor initialized");
}

void IthoCVE_Sensor::dump_config() {
    ESP_LOGCONFIG(TAG, "itho_cve:");
  
    LOG_UPDATE_INTERVAL(this);
    LOG_SENSOR(TAG, "Temperature", this->temperature_sensor_);
    LOG_SENSOR(TAG, "Humidity", this->humidity_sensor_);
}

void IthoCVE_Sensor::update() {
    if (this->temperature_sensor_ != nullptr) {
        const float temperature = parent_->ithoGetTemperature();
        this->temperature_sensor_->publish_state(temperature);
    }

    if (this->humidity_sensor_ != nullptr) {
        const float humidity = parent_->ithoGetHumidity();
        this->humidity_sensor_->publish_state(humidity);
    }
}

}  // namespace itho_cve
}  // namespace esphome
