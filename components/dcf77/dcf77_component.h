#pragma once

#include "esphome/core/hal.h"
#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"


#include <DCF77.h>       //https://github.com/thijse/Arduino-Libraries/downloads
#include <TimeLib.h>        //http://www.arduino.cc/playground/Code/Time //https://github.com/PaulStoffregen/Time

#define DCF_INTERRUPT 0		 // Interrupt number associated with pin

namespace esphome {
  namespace dcf77 {

  /// The DCF77 component allows you to configure local timekeeping via a DCF77 antenna.
  ///
  /// \note
  /// The C library (newlib) available on ESPs only supports TZ strings that specify an offset and DST info;
  /// you cannot specify zone names or paths to zoneinfo files.
  /// \see https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
  class DCF77Component : public time::RealTimeClock {
   public:
    void setup() override;
    void dump_config() override;  

    void update() override;
    void loop() override;

    void set_pin(esphome::GPIOPin *pin) { pin_ = pin; }

   private:

    bool has_time_{false};

    DCF77 DCF = DCF77((int)pin_, DCF_INTERRUPT);
    int syncHour = 0;
    boolean isSyncTime = false;

    GPIOPin *pin_;
  };

  }  // namespace dcf77
}  // namespace esphome
