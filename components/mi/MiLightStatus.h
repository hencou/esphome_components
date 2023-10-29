#pragma once

//#include <ArduinoJson.h>
#include "esphome/components/json/json_util.h"

enum MiLightStatus {
  ON = 0,
  OFF = 1
};

MiLightStatus parseMilightStatus(JsonVariant s);