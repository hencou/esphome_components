#include <stdint.h>
//#include <ArduinoJson.h>
#include "esphome/components/json/json_util.h"

#pragma once

struct ParsedColor {
  bool success;
  uint16_t hue, r, g, b;
  uint8_t saturation;

  static ParsedColor fromRgb(uint16_t r, uint16_t g, uint16_t b);
  static ParsedColor fromJson(JsonVariant json);
};