//#include <Arduino.h>
#include <vector>
#include <string>

#ifndef _RF24_CHANNELS_H
#define _RF24_CHANNELS_H

enum class RF24Channel {
  RF24_LOW = 0,
  RF24_MID = 1,
  RF24_HIGH = 2
};

class RF24ChannelHelpers {
public:
  static std::string nameFromValue(const RF24Channel& value);
  static RF24Channel valueFromName(const std::string& name);
  static RF24Channel defaultValue();
  static std::vector<RF24Channel> allValues();
};

#endif