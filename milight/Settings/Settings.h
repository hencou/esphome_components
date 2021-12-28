#include "../Types/RF24PowerLevel.h"
#include "../Types/RF24Channel.h"
#include <vector>

#ifndef _SETTINGS_H_INCLUDED
#define _SETTINGS_H_INCLUDED

enum RadioInterfaceType {
  nRF24 = 0,
  LT8900 = 1,
};

class Settings {
public:
  Settings() :
    // CE and CSN pins from nrf24l01
    cePin(4),
    csnPin(15),
    resetPin(0),
    radioInterfaceType(nRF24),
    packetRepeats(50),
    listenRepeats(20),
    stateFlushInterval(10000),
    packetRepeatThrottleThreshold(200),
    packetRepeatThrottleSensitivity(0),
    packetRepeatMinimum(3),
    enableAutomaticModeSwitching(false),
    rf24PowerLevel(RF24PowerLevelHelpers::defaultValue()),
    rf24Channels(RF24ChannelHelpers::allValues()),
    rf24ListenChannel(RF24Channel::RF24_LOW),
    packetRepeatsPerLoop(10)
  { }

  ~Settings() { }


  static RadioInterfaceType typeFromString(const String& s);
  static String typeToString(RadioInterfaceType type);
  static std::vector<RF24Channel> defaultListenChannels();

  uint8_t cePin;
  uint8_t csnPin;
  uint8_t resetPin;
  RadioInterfaceType radioInterfaceType;
  size_t packetRepeats;
  uint8_t listenRepeats;
  size_t stateFlushInterval;
  size_t packetRepeatThrottleThreshold;
  size_t packetRepeatThrottleSensitivity;
  size_t packetRepeatMinimum;
  bool enableAutomaticModeSwitching;
  RF24PowerLevel rf24PowerLevel;
  std::vector<RF24Channel> rf24Channels;
  RF24Channel rf24ListenChannel;
  size_t packetRepeatsPerLoop;
};

#endif
