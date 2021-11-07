#include "dcf77_component.h"
#include "esphome/core/log.h"

namespace esphome {
  namespace dcf77 {

  static const char *const TAG = "dcf77";

  void DCF77Component::setup() {
    ESP_LOGCONFIG(TAG, "Setting up DCF77...");

    DCF.Start();
    setSyncInterval(60);
    setSyncProvider(DCF.getTime);
  }
  void DCF77Component::dump_config() {
    ESP_LOGCONFIG(TAG, "  Timezone: '%s'", this->timezone_.c_str());
  }
  void DCF77Component::update() {
    
    //sync clock only at 0 o'clock to prevent wifi and DCF interrupt pin conflicts
    if (syncHour == hour()) {
      if (isSyncTime == false) {
        DCF.Start();
      }
      isSyncTime = true;
    } else {
      if (isSyncTime == true) {
        DCF.Stop();
      }
      isSyncTime = false;
    }
    this->has_time_ = false;
    ESP_LOGD(TAG, "DCF77 time: %d-%d-%d %d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  }
  void DCF77Component::loop() {
    if (this->has_time_)
      return;
    
    time.year = year();
    time.month = month();
    time.day_of_month = day();
    time.hour = hour();
    time.minute = minute();
    time.second = second();
    
    auto time = this->now();
    if (!time.is_valid())
      return;

    ESP_LOGD(TAG, "Synchronized time: %d-%d-%d %d:%02d:%02d", time.year, time.month, time.day_of_month, time.hour, time.minute, time.second);
    this->time_sync_callback_.call();
    this->has_time_ = true;
  }

  }  // namespace dcf77
}  // namespace esphome
