#include "FUT022PacketFormatter.h"
#include "Units.h"
#include "GroupStateField.h"
#include "MiLightCommands.h"
#include "esphome/core/log.h"

// ============================================================
// SENDING — generate packets to control the CCT receiver
// ============================================================

void FUT022PacketFormatter::updateStatus(MiLightStatus status, uint8_t groupId) {
  if (status == ON) {
    command(static_cast<uint8_t>(FUT022Command::ON), 0);
  } else {
    command(static_cast<uint8_t>(FUT022Command::OFF), 0);
  }
}

void FUT022PacketFormatter::increaseBrightness() {
  command(static_cast<uint8_t>(FUT022Command::BRIGHTNESS_UP), 0);
}

void FUT022PacketFormatter::decreaseBrightness() {
  command(static_cast<uint8_t>(FUT022Command::BRIGHTNESS_DOWN), 0);
}

void FUT022PacketFormatter::updateBrightness(uint8_t value) {
  // MiLightClient passes brightness as 0-100.
  // Map to ring brightness range (RING_BRIGHTNESS_MIN-RING_BRIGHTNESS_MAX).
  uint8_t ringValue = Units::rescale<uint8_t, uint16_t>(value, RING_BRIGHTNESS_MAX - RING_BRIGHTNESS_MIN, 100);
  ringValue += RING_BRIGHTNESS_MIN;
  command(static_cast<uint8_t>(FUT022Command::RING), ringValue);
}

void FUT022PacketFormatter::increaseTemperature() {
  // No dedicated temp+ button on FUT022 — simulate via ring (warmer)
  command(static_cast<uint8_t>(FUT022Command::RING), RING_TEMP_MIN);
}

void FUT022PacketFormatter::decreaseTemperature() {
  // No dedicated temp- button — simulate via ring (cooler)
  command(static_cast<uint8_t>(FUT022Command::RING), RING_TEMP_MAX);
}

void FUT022PacketFormatter::updateTemperature(uint8_t value) {
  // MiLightClient passes temperature as 0-100 (0=cold, 100=warm).
  // Ring: RING_TEMP_MAX=cold, RING_TEMP_MIN=warm.
  uint8_t ringValue = RING_TEMP_MAX - Units::rescale<uint8_t, uint16_t>(value, RING_TEMP_MAX - RING_TEMP_MIN, 100);
  command(static_cast<uint8_t>(FUT022Command::RING), ringValue);
}

void FUT022PacketFormatter::updateColorRaw(uint8_t value) {
  // FUT022 is CCT-only — no color. Treat as ring position for compatibility.
  command(static_cast<uint8_t>(FUT022Command::RING), value);
}

// ============================================================
// RECEIVING — parse packets from FUT022 remote into JSON state
// ============================================================

BulbId FUT022PacketFormatter::parsePacket(const uint8_t* packet, JsonObject result) {
  uint8_t cmdByte = packet[FUT02xPacketFormatter::FUT02X_COMMAND_INDEX];
  FUT022Command cmd = static_cast<FUT022Command>(cmdByte & 0x0F);
  uint8_t arg = packet[FUT02xPacketFormatter::FUT02X_ARGUMENT_INDEX];

  BulbId bulbId(
    (packet[1] << 8) | packet[2],
    0,
    REMOTE_TYPE_FUT022
  );

  switch (cmd) {
    case FUT022Command::ON:
      result[GroupStateFieldNames::STATE] = "ON";
      break;

    case FUT022Command::OFF:
      result[GroupStateFieldNames::STATE] = "OFF";
      break;

    case FUT022Command::BRIGHTNESS_UP:
      result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::BRIGHTNESS_UP;
      break;

    case FUT022Command::BRIGHTNESS_DOWN:
      result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::BRIGHTNESS_DOWN;
      break;

    case FUT022Command::MEDIUM:
      // 50% brightness — emit absolute value
      result[GroupStateFieldNames::STATE] = "ON";
      result[GroupStateFieldNames::BRIGHTNESS] = 128;
      break;

    case FUT022Command::RING:
      ESP_LOGW("FUT022", "RING raw arg=0x%02X (%d)", arg, arg);
      if (arg >= RING_BRIGHTNESS_MIN) {
        // Left half of ring → absolute brightness
        uint8_t clamped = constrain(arg, RING_BRIGHTNESS_MIN, RING_BRIGHTNESS_MAX);
        uint8_t brightness = Units::rescale<uint8_t, uint16_t>(
          clamped - RING_BRIGHTNESS_MIN,
          255,
          RING_BRIGHTNESS_MAX - RING_BRIGHTNESS_MIN
        );
        if (brightness < 1) brightness = 1;
        result[GroupStateFieldNames::BRIGHTNESS] = brightness;
      } else {
        // Right half of ring → absolute color temperature
        // RING_TEMP_MAX = coldest, RING_TEMP_MIN = warmest
        uint8_t clamped = constrain(arg, RING_TEMP_MIN, RING_TEMP_MAX);
        uint16_t mireds = Units::rescale<uint16_t, uint16_t>(
          RING_TEMP_MAX - clamped, 370 - 153, RING_TEMP_MAX - RING_TEMP_MIN
        );
        mireds += 153;
        result[GroupStateFieldNames::COLOR_TEMP] = mireds;
      }
      break;
  }

  return bulbId;
}
