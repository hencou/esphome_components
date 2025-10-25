#include "MiLightStatus.h"
//#include <ArduinoJson.h>
#include "esphome/components/json/json_util.h"
#include "esphome/core/helpers.h"

MiLightStatus parseMilightStatus(JsonVariant val) {
  if (val.is<bool>()) {
    return val.as<bool>() ? ON : OFF;
  } else if (val.is<uint16_t>()) {
    return static_cast<MiLightStatus>(val.as<uint16_t>());
  } else {
    std::string strStatus(val.as<const char*>());
    return (esphome::str_equals_case_insensitive(strStatus, "on") || esphome::str_equals_case_insensitive(strStatus, "true")) ? ON : OFF;
  }
}