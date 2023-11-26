#include "S2PacketFormatter.h"
#include "V2RFEncoding.h"
#include "Units.h"
#include "MiLightCommands.h"

void S2PacketFormatter::updateMode(uint8_t mode) {
  lastMode = mode;
  command(S2_MODE, mode);
}

void S2PacketFormatter::nextMode() {
  updateMode((lastMode+1)%S2_NUM_MODES);
}

void S2PacketFormatter::previousMode() {
  updateMode((lastMode-1)%S2_NUM_MODES);
}

void S2PacketFormatter::updateBrightness(uint8_t brightness) {
  command(S2_BRIGHTNESS, brightness);
}

// change the hue (which may also change to color mode).
void S2PacketFormatter::updateHue(uint16_t value) {
  uint8_t remapped = Units::rescale(value, 255, 360);
  updateColorRaw(remapped);
}

void S2PacketFormatter::updateColorRaw(uint8_t value) {
  command(S2_COLOR, S2_COLOR_OFFSET + value);
}

void S2PacketFormatter::updateTemperature(uint8_t value) {
  // when updating temperature, the bulb switches to white.  If we are not already
  // in white mode, that makes changing temperature annoying because the current hue/mode
  // is lost.  So lookup our current bulb mode, and if needed, reset the hue/mode after
  // changing the temperature
  const GroupState* ourState = this->stateStore->get(this->deviceId, this->groupId, REMOTE_TYPE_S2);

  // now make the temperature change
  command(S2_KELVIN, value);

  // and return to our original mode
  if (ourState != NULL) {
    BulbMode originalBulbMode = ourState->getBulbMode();

    if ((settings->enableAutomaticModeSwitching) && (originalBulbMode != BulbMode::BULB_MODE_WHITE)) {
      switchMode(*ourState, originalBulbMode);
    }
  }
}

// update saturation.  This only works when in Color mode, so if not in color we switch to color,
// make the change, and switch back again.
void S2PacketFormatter::updateSaturation(uint8_t value) {
   // look up our current mode
  const GroupState* ourState = this->stateStore->get(this->deviceId, this->groupId, REMOTE_TYPE_S2);
  BulbMode originalBulbMode = BulbMode::BULB_MODE_WHITE;

  if (ourState != NULL) {
    originalBulbMode = ourState->getBulbMode();

    // are we already in white?  If not, change to white
    if ((settings->enableAutomaticModeSwitching) && (originalBulbMode != BulbMode::BULB_MODE_COLOR)) {
      updateHue(ourState->getHue());
    }
  }

  // now make the saturation change
  command(S2_SATURATION, value);

  if (ourState != NULL) {
    if ((settings->enableAutomaticModeSwitching) && (originalBulbMode != BulbMode::BULB_MODE_COLOR)) {
      switchMode(*ourState, originalBulbMode);
    }
  }
}

void S2PacketFormatter::updateColorWhite() {
  // there is no direct white command, so let's look up our prior temperature and set that, which
  // causes the bulb to go white
  const GroupState* ourState = this->stateStore->get(this->deviceId, this->groupId, REMOTE_TYPE_S2);
  uint8_t value =
    ourState == NULL
      ? 0
      : ourState->getKelvin();

  // issue command to set kelvin to prior value, which will drive to white
  command(S2_KELVIN, value);
}

void S2PacketFormatter::enableNightMode() {
  uint8_t arg = groupCommandArg(OFF, groupId);
  command(S2_ON_OFF | 0x80, arg);
}

BulbId S2PacketFormatter::parsePacket(const uint8_t *packet, JsonObject result) {
  uint8_t packetCopy[V2_PACKET_LEN];
  memcpy(packetCopy, packet, V2_PACKET_LEN);
  V2RFEncoding::decodeV2Packet(packetCopy);

  BulbId bulbId(
    (packetCopy[2] << 8) | packetCopy[3],
    packetCopy[7],
    REMOTE_TYPE_S2
  );

  uint8_t command = (packetCopy[V2_COMMAND_INDEX] & 0x7F);
  uint8_t arg = packetCopy[V2_ARGUMENT_INDEX];

  if (command == S2_ON_OFF) {
    if ((packetCopy[V2_COMMAND_INDEX] & 0x80) == 0x80) {
      result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::NIGHT_MODE;
    } else if (arg == 0x01) {
      result[GroupStateFieldNames::STATE] = "ON";
    } else if (arg == 0x0A) {
      result[GroupStateFieldNames::STATE] = "OFF";
    } else if (arg == 0x14) {
      // todo: this detects pressing of "palette key" on my remote
      // don't really know how to map it...
      result["button_id"] = "palette";
    }
  } else if (command == S2_COLOR) {
    uint16_t hue = Units::rescale<uint16_t, uint16_t>(arg, 360, 255.0);
    result[GroupStateFieldNames::HUE] = hue;
  } else if (command == S2_KELVIN) {
    uint8_t temperature = arg;
    result[GroupStateFieldNames::KELVIN] = Units::whiteValToMireds(temperature, 100);
  } else if (command == S2_BRIGHTNESS) {
    result[GroupStateFieldNames::BRIGHTNESS] = Units::rescale<uint8_t, uint8_t>(arg, 255, 100);
  } else if (command == S2_SATURATION) {
    result[GroupStateFieldNames::SATURATION] = arg;
  } else if (command == S2_MODE) {
    result[GroupStateFieldNames::MODE] = arg;
  } else {
    result["button_id"] = command;
    result["argument"] = arg;
  }

  return bulbId;
}
