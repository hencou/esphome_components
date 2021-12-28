#include "../Settings/Settings.h"
#include <ArduinoJson.h>
#include "../Helpers/IntParsing.h"
#include <algorithm>
#include "../Helpers/JsonHelpers.h"

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
