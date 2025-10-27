#pragma once

#include <utility>
#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/hal.h"
#include "esphome/core/defines.h"

#ifndef USE_ARDUINO
#include <esp_timer.h>
#endif


class MiHelpers {
public:
  MiHelpers() { }
  ~MiHelpers() { }

  uint32_t mi_millis() {
      #ifdef USE_ARDUINO
      #include <Arduino.h>
        return millis();
      #else
        return esp_timer_get_time() / 1000;
      #endif
    }
};
