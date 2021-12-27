#include <Arduino.h>
#include "../Settings/StringStream.h"
#include <ArduinoJson.h>
#include "../Types/GroupStateField.h"
#include "../Types/RF24PowerLevel.h"
#include "../Types/RF24Channel.h"
#include "../Helpers/Size.h"

#include "../Types/MiLightRemoteType.h"
#include "../Types/BulbId.h"

#include <vector>
#include <memory>
#include <map>

#ifndef _SETTINGS_H_INCLUDED
#define _SETTINGS_H_INCLUDED

#ifndef MILIGHT_HUB_SETTINGS_BUFFER_SIZE
#define MILIGHT_HUB_SETTINGS_BUFFER_SIZE 4096
#endif

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifndef MILIGHT_MAX_STATE_ITEMS
#define MILIGHT_MAX_STATE_ITEMS 100
#endif

enum RadioInterfaceType {
  nRF24 = 0,
  LT8900 = 1,
};

static const std::vector<GroupStateField> DEFAULT_GROUP_STATE_FIELDS({
  GroupStateField::STATE,
  GroupStateField::BRIGHTNESS,
  GroupStateField::COMPUTED_COLOR,
  GroupStateField::MODE,
  GroupStateField::COLOR_TEMP,
  GroupStateField::BULB_MODE
});

class Settings {
public:
  Settings() :
    // CE and CSN pins from nrf24l01
    cePin(16),
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
    packetRepeatsPerLoop(10),
    defaultTransitionPeriod(500)
  { }

  ~Settings() { }


  static void load(Settings& settings);

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
  uint16_t defaultTransitionPeriod;

};

#endif
