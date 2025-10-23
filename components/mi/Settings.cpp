#include "Settings.h"
//#include <ArduinoJson.h>
#include "esphome/components/json/json_util.h"
#include "IntParsing.h"
#include <algorithm>
#include "JsonHelpers.h"

RadioInterfaceType Settings::typeFromString(const String& s) {
  if (s.equalsIgnoreCase("lt8900")) {
    return LT8900;
  } else {
    return nRF24;
  }
}

String Settings::typeToString(RadioInterfaceType type) {
  switch (type) {
    case LT8900:
      return "LT8900";

    case nRF24:
    default:
      return "nRF24";
  }
}
