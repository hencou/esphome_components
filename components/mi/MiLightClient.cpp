#include "MiLightClient.h"
#include "MiLightRadioConfig.h"
#include <Arduino.h>
#include "Units.h"
#include <TokenIterator.h>
#include "ParsedColor.h"
#include "MiLightCommands.h"
#include <functional>

using namespace std::placeholders;

static const uint8_t STATUS_UNDEFINED = 255;

const char* MiLightClient::FIELD_ORDERINGS[] = {
  // These are handled manually
  // GroupStateFieldNames::STATE,
  // GroupStateFieldNames::STATUS,
  GroupStateFieldNames::HUE,
  GroupStateFieldNames::SATURATION,
  GroupStateFieldNames::KELVIN,
  GroupStateFieldNames::TEMPERATURE,
  GroupStateFieldNames::COLOR_TEMP,
  GroupStateFieldNames::MODE,
  GroupStateFieldNames::EFFECT,
  GroupStateFieldNames::COLOR,
  // Level/Brightness must be processed last because they're specific to a particular bulb mode.
  // So make sure bulb mode is set before applying level/brightness.
  GroupStateFieldNames::LEVEL,
  GroupStateFieldNames::BRIGHTNESS,
  GroupStateFieldNames::COMMAND,
  GroupStateFieldNames::COMMANDS
};

const std::map<const char*, std::function<void(MiLightClient*, JsonVariant)>, MiLightClient::cmp_str> MiLightClient::FIELD_SETTERS = {
  {
    GroupStateFieldNames::STATUS,
    [](MiLightClient* client, JsonVariant val) {
      client->updateStatus(parseMilightStatus(val));
    }
  },
  {GroupStateFieldNames::LEVEL, &MiLightClient::updateBrightness},
  {
    GroupStateFieldNames::BRIGHTNESS,
    [](MiLightClient* client, uint16_t arg) {
      client->updateBrightness(Units::rescale<uint16_t, uint16_t>(arg, 100, 255));
    }
  },
  {GroupStateFieldNames::HUE, &MiLightClient::updateHue},
  {GroupStateFieldNames::SATURATION, &MiLightClient::updateSaturation},
  {GroupStateFieldNames::KELVIN, &MiLightClient::updateTemperature},
  {GroupStateFieldNames::TEMPERATURE, &MiLightClient::updateTemperature},
  {
    GroupStateFieldNames::COLOR_TEMP,
    [](MiLightClient* client, uint16_t arg) {
      client->updateTemperature(Units::miredsToWhiteVal(arg, 100));
    }
  },
  {GroupStateFieldNames::MODE, &MiLightClient::updateMode},
  {GroupStateFieldNames::COLOR, &MiLightClient::updateColor},
  {GroupStateFieldNames::EFFECT, &MiLightClient::handleEffect},
  {GroupStateFieldNames::COMMAND, &MiLightClient::handleCommand},
  {GroupStateFieldNames::COMMANDS, &MiLightClient::handleCommands}
};

MiLightClient::MiLightClient(
  RadioSwitchboard& radioSwitchboard,
  PacketSender& packetSender,
  GroupStateStore* stateStore,
  Settings& settings
) : radioSwitchboard(radioSwitchboard)
  , updateBeginHandler(NULL)
  , updateEndHandler(NULL)
  , stateStore(stateStore)
  , settings(settings)
  , packetSender(packetSender)
  , repeatsOverride(0)
{ }

void MiLightClient::setHeld(bool held) {
  currentRemote->packetFormatter->setHeld(held);
}

void MiLightClient::prepare(
  const MiLightRemoteConfig* config,
  const uint16_t deviceId,
  const uint8_t groupId
) {
  this->currentRemote = config;

  if (deviceId >= 0 && groupId >= 0) {
    currentRemote->packetFormatter->prepare(deviceId, groupId);
  }

  this->currentState = stateStore->get(deviceId, groupId, config->type);
}

void MiLightClient::prepare(
  const MiLightRemoteType type,
  const uint16_t deviceId,
  const uint8_t groupId
) {
  prepare(MiLightRemoteConfig::fromType(type), deviceId, groupId);
}

void MiLightClient::updateColorRaw(const uint8_t color) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateColorRaw: Change color to %d\n"), color);
#endif
  currentRemote->packetFormatter->updateColorRaw(color);
  flushPacket();
}

void MiLightClient::updateHue(const uint16_t hue) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateHue: Change hue to %d\n"), hue);
#endif
  currentRemote->packetFormatter->updateHue(hue);
  flushPacket();
}

void MiLightClient::updateBrightness(const uint8_t brightness) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateBrightness: Change brightness to %d\n"), brightness);
#endif
  currentRemote->packetFormatter->updateBrightness(brightness);
  flushPacket();
}

void MiLightClient::updateMode(uint8_t mode) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateMode: Change mode to %d\n"), mode);
#endif
  currentRemote->packetFormatter->updateMode(mode);
  flushPacket();
}

void MiLightClient::nextMode() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::nextMode: Switch to next mode"));
#endif
  currentRemote->packetFormatter->nextMode();
  flushPacket();
}

void MiLightClient::previousMode() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::previousMode: Switch to previous mode"));
#endif
  currentRemote->packetFormatter->previousMode();
  flushPacket();
}

void MiLightClient::modeSpeedDown() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::modeSpeedDown: Speed down\n"));
#endif
  currentRemote->packetFormatter->modeSpeedDown();
  flushPacket();
}
void MiLightClient::modeSpeedUp() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::modeSpeedUp: Speed up"));
#endif
  currentRemote->packetFormatter->modeSpeedUp();
  flushPacket();
}

void MiLightClient::updateStatus(MiLightStatus status, uint8_t groupId) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateStatus: Status %s, groupId %d\n"), status == MiLightStatus::OFF ? "OFF" : "ON", groupId);
#endif

  //<added by HC>
  //if (status == MiLightStatus::OFF) {
  //  this->updateBrightness(0);
  //}
  //</added by HC>

  currentRemote->packetFormatter->updateStatus(status, groupId);
  flushPacket();
}

void MiLightClient::updateStatus(MiLightStatus status) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateStatus: Status %s\n"), status == MiLightStatus::OFF ? "OFF" : "ON");
#endif

  //<added by HC>
  //if (status == MiLightStatus::OFF) {
  //  this->updateBrightness(0);
  //}
  //</added by HC>

  currentRemote->packetFormatter->updateStatus(status);
  flushPacket();
}

void MiLightClient::updateSaturation(const uint8_t value) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateSaturation: Saturation %d\n"), value);
#endif
  currentRemote->packetFormatter->updateSaturation(value);
  flushPacket();
}

void MiLightClient::updateColorWhite() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::updateColorWhite: Color white"));
#endif
  currentRemote->packetFormatter->updateColorWhite();
  flushPacket();
}

void MiLightClient::enableNightMode() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::enableNightMode: Night mode"));
#endif

  //<added by HC>
  this->updateBrightness(0);
  //</added by HC>

  currentRemote->packetFormatter->enableNightMode();
  flushPacket();
}

void MiLightClient::pair() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::pair: Pair"));
#endif
  currentRemote->packetFormatter->pair();
  flushPacket();
}

void MiLightClient::unpair() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::unpair: Unpair"));
#endif
  currentRemote->packetFormatter->unpair();
  flushPacket();
}

void MiLightClient::increaseBrightness() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::increaseBrightness: Increase brightness"));
#endif
  currentRemote->packetFormatter->increaseBrightness();
  flushPacket();
}

void MiLightClient::decreaseBrightness() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::decreaseBrightness: Decrease brightness"));
#endif
  currentRemote->packetFormatter->decreaseBrightness();
  flushPacket();
}

void MiLightClient::increaseTemperature() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::increaseTemperature: Increase temperature"));
#endif
  currentRemote->packetFormatter->increaseTemperature();
  flushPacket();
}

void MiLightClient::decreaseTemperature() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.println(F("MiLightClient::decreaseTemperature: Decrease temperature"));
#endif
  currentRemote->packetFormatter->decreaseTemperature();
  flushPacket();
}

void MiLightClient::updateTemperature(const uint8_t temperature) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::updateTemperature: Set temperature to %d\n"), temperature);
#endif
  currentRemote->packetFormatter->updateTemperature(temperature);
  flushPacket();
}

void MiLightClient::command(uint8_t command, uint8_t arg) {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::command: Execute command %d, argument %d\n"), command, arg);
#endif
  currentRemote->packetFormatter->command(command, arg);
  flushPacket();
}

void MiLightClient::toggleStatus() {
#ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("MiLightClient::toggleStatus"));
#endif
  currentRemote->packetFormatter->toggleStatus();
  flushPacket();
}

void MiLightClient::updateColor(JsonVariant json) {
  ParsedColor color = ParsedColor::fromJson(json);

  if (!color.success) {
    Serial.println(F("Error parsing color field, unrecognized format"));
    return;
  }

  // We consider an RGB color "white" if all color intensities are roughly the
  // same value.  An unscientific value of 10 (~4%) is chosen.
  if ( abs(color.r - color.g) < RGB_WHITE_THRESHOLD
    && abs(color.g - color.b) < RGB_WHITE_THRESHOLD
    && abs(color.r - color.b) < RGB_WHITE_THRESHOLD) {
      this->updateColorWhite();
  } else {
    this->updateHue(color.hue);
    this->updateSaturation(color.saturation);
  }
}

void MiLightClient::update(JsonObject request) {
  if (this->updateBeginHandler) {
    this->updateBeginHandler();
  }

  //<Added by HC remove ON/OFF state commands when receiving night mode command>
  if (request.containsKey(GroupStateFieldNames::EFFECT)) {
    if (request[GroupStateFieldNames::EFFECT] == MiLightCommandNames::NIGHT_MODE) {
      if (request.containsKey(GroupStateFieldNames::STATE)) {
        request.remove(GroupStateFieldNames::STATE);
      }
      if (request.containsKey(GroupStateFieldNames::STATUS)) {
        request.remove(GroupStateFieldNames::STATUS);
      }
    }
    //remove "white" effect, to prevent unwanted color_temp changes from warm to cold white, white is already set by color_temp
    if (request[GroupStateFieldNames::EFFECT] == "white_mode" || request[GroupStateFieldNames::EFFECT] == "white") {
      request.remove(GroupStateFieldNames::EFFECT);
    }

    if (request.containsKey(GroupStateFieldNames::COLOR_TEMP) || request.containsKey(GroupStateFieldNames::TEMPERATURE)) {
      this->updateColorWhite();
    }
  }
  //</Added by HC>

  const JsonVariant status = this->extractStatus(request);
  const uint8_t parsedStatus = this->parseStatus(status);

  JsonVariant brightness = request[GroupStateFieldNames::BRIGHTNESS];
  JsonVariant level = request[GroupStateFieldNames::LEVEL];
  const bool isBrightnessDefined = !brightness.isNull() || !level.isNull();

  // Always turn on first
  if (parsedStatus == ON) {
    this->updateStatus(ON);
  }

  for (const char* fieldName : FIELD_ORDERINGS) {
    if (request.containsKey(fieldName)) {
      auto handler = FIELD_SETTERS.find(fieldName);
      JsonVariant value = request[fieldName];

      if (handler != FIELD_SETTERS.end()) {
        handler->second(this, value);
      }
    }
  }

  // Raw packet command/args
  if (request.containsKey("button_id") && request.containsKey("argument")) {
    this->command(request["button_id"], request["argument"]);
  }

  // Always turn off last
  if (parsedStatus == OFF) {
    this->updateStatus(OFF);
  }

  if (this->updateEndHandler) {
    this->updateEndHandler();
  }
}

void MiLightClient::handleCommands(JsonArray commands) {
  if (! commands.isNull()) {
    for (size_t i = 0; i < commands.size(); i++) {
      this->handleCommand(commands[i]);
    }
  }
}

void MiLightClient::handleCommand(JsonVariant command) {
  String cmdName;
  JsonObject args;

  if (command.is<JsonObject>()) {
    JsonObject cmdObj = command.as<JsonObject>();
    cmdName = cmdObj[GroupStateFieldNames::COMMAND].as<const char*>();
    args = cmdObj["args"];
  } else if (command.is<const char*>()) {
    cmdName = command.as<const char*>();
  }

  if (cmdName == MiLightCommandNames::UNPAIR) {
    this->unpair();
  } else if (cmdName == MiLightCommandNames::PAIR) {
    this->pair();
  } else if (cmdName == MiLightCommandNames::SET_WHITE) {
    this->updateColorWhite();
  } else if (cmdName == MiLightCommandNames::NIGHT_MODE) {
    this->enableNightMode();
  } else if (cmdName == MiLightCommandNames::LEVEL_UP) {
    this->increaseBrightness();
  } else if (cmdName == MiLightCommandNames::LEVEL_DOWN) {
    this->decreaseBrightness();
  } else if (cmdName == MiLightCommandNames::BRIGHTNESS_UP) {
    this->increaseBrightness();
  } else if (cmdName == MiLightCommandNames::BRIGHTNESS_DOWN) {
    this->decreaseBrightness();
  } else if (cmdName == MiLightCommandNames::TEMPERATURE_UP) {
    this->increaseTemperature();
  } else if (cmdName == MiLightCommandNames::TEMPERATURE_DOWN) {
    this->decreaseTemperature();
  } else if (cmdName == MiLightCommandNames::NEXT_MODE) {
    this->nextMode();
  } else if (cmdName == MiLightCommandNames::PREVIOUS_MODE) {
    this->previousMode();
  } else if (cmdName == MiLightCommandNames::MODE_SPEED_DOWN) {
    this->modeSpeedDown();
  } else if (cmdName == MiLightCommandNames::MODE_SPEED_UP) {
    this->modeSpeedUp();
  } else if (cmdName == MiLightCommandNames::TOGGLE) {
    this->toggleStatus();
  }
}

void MiLightClient::handleEffect(const String& effect) {
  #ifdef DEBUG_CLIENT_COMMANDS
  Serial.printf_P(PSTR("Request to handle effect '%s' in MiLight component."), effect);
  #endif
  if (effect.startsWith("Mi ") != true) {
    // This is not a MiLight built-in effect. We don't need to handle it here.
    #ifdef DEBUG_CLIENT_COMMANDS
    Serial.printf_P(PSTR("This is not a MiLight built-in effect."));
    #endif
    return;
  }
  int effectId = effect.substring(3, 5).toInt();
  if (effectId < 0 || effectId > 9) {
    #ifdef DEBUG_CLIENT_COMMANDS
    Serial.printf_P(PSTR("Invalid effect ID at position 3-4: %d. MiLights only support effect ids 1-9."), effectId);
    #endif
    return;
  }
  if (effectId == 0) {
    this->enableNightMode();
  } else {
    this->updateMode((uint8_t) (effectId & 0xFF));
  }
}

JsonVariant MiLightClient::extractStatus(JsonObject object) {
  JsonVariant status;

  if (object.containsKey(FPSTR(GroupStateFieldNames::STATUS))) {
    return object[FPSTR(GroupStateFieldNames::STATUS)];
  } else {
    return object[FPSTR(GroupStateFieldNames::STATE)];
  }
}

uint8_t MiLightClient::parseStatus(JsonVariant val) {
  if (val.isNull()) {
    return STATUS_UNDEFINED;
  }

  return parseMilightStatus(val);
}

void MiLightClient::setRepeatsOverride(size_t repeats) {
  this->repeatsOverride = repeats;
}

void MiLightClient::clearRepeatsOverride() {
  this->repeatsOverride = PacketSender::DEFAULT_PACKET_SENDS_VALUE;
}

void MiLightClient::flushPacket() {
  PacketStream& stream = currentRemote->packetFormatter->buildPackets();

  while (stream.hasNext()) {
    packetSender.enqueue(stream.next(), currentRemote, repeatsOverride);
  }

  currentRemote->packetFormatter->reset();
}

void MiLightClient::onUpdateBegin(EventHandler handler) {
  this->updateBeginHandler = handler;
}

void MiLightClient::onUpdateEnd(EventHandler handler) {
  this->updateEndHandler = handler;
}
