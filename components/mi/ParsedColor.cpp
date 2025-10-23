#include "ParsedColor.h"
#include <TokenIterator.h>
#include "GroupStateField.h"
#include "IntParsing.h"
#include <cmath>
#include <algorithm>

ParsedColor ParsedColor::fromRgb(uint16_t r, uint16_t g, uint16_t b) {
  int hue = 0;
  float saturation;
  float value;

  //rgb_to_hsv(float red, float green, float blue, int &hue, float &saturation, float &value)
  // Adapted from https://esphome.io/api/namespaceesphome.html#a402f230bbe547e4a625ad28acf5e2647

  float red = r / 255.00;
  float green = g / 255.00;
  float blue = b / 255.00;
  float max_color_value = std::max(std::max(red, green), blue);
  float min_color_value = std::min(std::min(red, green), blue);
  float delta = max_color_value - min_color_value;

  if (delta == 0)
    hue = 0;
  else if (max_color_value == red)
    hue = int(fmod(((60 * ((green - blue) / delta)) + 360), 360));
  else if (max_color_value == green)
    hue = int(fmod(((60 * ((blue - red) / delta)) + 120), 360));
  else if (max_color_value == blue)
    hue = int(fmod(((60 * ((red - green) / delta)) + 240), 360));

  if (max_color_value == 0)
    saturation = 0;
  else
    saturation = delta / max_color_value;

  value = max_color_value;

  uint8_t sat = saturation * 100;

  return ParsedColor{
    .success = true,
    .hue = (uint16_t)hue,
    .r = r,
    .g = g,
    .b = b,
    .saturation = sat
  };
}

ParsedColor ParsedColor::fromJson(JsonVariant json) {
  uint16_t r, g, b;

  if (json.is<JsonObject>()) {
    JsonObject color = json.as<JsonObject>();

    r = color["r"];
    g = color["g"];
    b = color["b"];
  } else if (json.is<const char*>()) {
    const char* colorStr = json.as<const char*>();
    const size_t len = strlen(colorStr);

    if (colorStr[0] == '#' && len == 7) {
      uint8_t parsedHex[3];
      hexStrToBytes<uint8_t>(colorStr+1, len-1, parsedHex, 3);

      r = parsedHex[0];
      g = parsedHex[1];
      b = parsedHex[2];
    } else {
      char colorCStr[len+1];
      uint8_t parsedRgbColors[3] = {0, 0, 0};

      strcpy(colorCStr, colorStr);
      TokenIterator colorValueItr(colorCStr, len, ',');

      for (size_t i = 0; i < 3 && colorValueItr.hasNext(); ++i) {
        parsedRgbColors[i] = atoi(colorValueItr.nextToken());
      }

      r = parsedRgbColors[0];
      g = parsedRgbColors[1];
      b = parsedRgbColors[2];
    }
  } else {
    Serial.println(F("GroupState::parseJsonColor - unknown format for color"));
    return ParsedColor{ .success = false };
  }

  return ParsedColor::fromRgb(r, g, b);
}