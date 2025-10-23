#include "Settings.h"
#include "esphome/components/json/json_util.h"
#include "IntParsing.h"
#include "JsonHelpers.h"

RadioInterfaceType Settings::typeFromString(const std::string& s) {

  if (s.compare("lt8900")) {
    return LT8900;
  } else if (s.compare("LT8900")) {
    return LT8900;
  }
  else{
    return nRF24;
  }
}

std::string Settings::typeToString(RadioInterfaceType type) {
  switch (type) {
    case LT8900:
      return "LT8900";

    case nRF24:
    default:
      return "nRF24";
  }
}
