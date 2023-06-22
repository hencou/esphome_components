#include "dcf77_component.h"
#include "esphome/core/log.h"

namespace esphome {
  namespace dcf77 {

    static const char *const TAG = "dcf77";

    void DCF77Component::setup() {
      ESP_LOGCONFIG(TAG, "Setting up DCF77...");

      DCF.Start();
      setSyncInterval(60);
      setSyncProvider(DCF.getUTCTime);
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
      
      esphome::ESPTime val{};
      val.year = year();
      val.month = month();
      val.day_of_month = day();
      // Set these to valid value for  recalc_timestamp_utc - it's not used for calculation
      val.day_of_week = 1;
      val.day_of_year = 1;

      val.hour = hour();
      val.minute = minute();
      val.second = second();
      val.recalc_timestamp_utc(false);
      this->synchronize_epoch_(val.timestamp);
      this->has_time_ = true;
    }
  }  // namespace dcf77
}  // namespace esphome
