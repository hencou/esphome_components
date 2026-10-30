#pragma once

#include "FUT02xPacketFormatter.h"

// FUT022 CCT Remote — command mapping (reverse-engineered from real FUT022 captures)
// Uses same radio config as FUT020 (syncword 0x50A0/0xAA55, channels 6/41/76, 6-byte packets)
// but button-to-command assignment differs:
//
// Physical button → Command byte (low nibble):
//   ON              → 0x02
//   OFF             → 0x05
//   Brightness+     → 0x03
//   Brightness-     → 0x01
//   Medium (50%)    → 0x04
//   Touch Ring      → 0x00  (arg = position: ≥0x80 = brightness, <0x80 = color temp)
//
// Bit 4 of command byte = "held" flag (set on repeat while button is held)

enum class FUT022Command {
  RING            = 0x00,   // Touch ring — arg encodes brightness or color temp
  BRIGHTNESS_DOWN = 0x01,
  ON              = 0x02,
  BRIGHTNESS_UP   = 0x03,
  MEDIUM          = 0x04,   // Set brightness to ~50%
  OFF             = 0x05
};

class FUT022PacketFormatter : public FUT02xPacketFormatter {
public:
  FUT022PacketFormatter()
    : FUT02xPacketFormatter(REMOTE_TYPE_FUT022)
  { }

  // --- Sending commands ---
  virtual void updateStatus(MiLightStatus status, uint8_t groupId) override;
  virtual void updateBrightness(uint8_t value) override;
  virtual void increaseBrightness() override;
  virtual void decreaseBrightness() override;
  virtual void updateColorRaw(uint8_t value) override;
  virtual void increaseTemperature() override;
  virtual void decreaseTemperature() override;
  virtual void updateTemperature(uint8_t value) override;

  // --- Receiving/parsing ---
  virtual BulbId parsePacket(const uint8_t* packet, JsonObject result) override;

  // Override canHandle to return false — prevents fromReceivedPacket() from
  // auto-detecting as FUT022 (would conflict with FUT020 which has same radio config).
  // Instead, the configured output's remoteConfig is used as fallback.
  virtual bool canHandle(const uint8_t* packet, const size_t len) override { return false; }

  // Ring arg value ranges (calibrated from physical remote captures)
  static const uint8_t RING_BRIGHTNESS_MIN = 0x90;  // left half of ring — dimmest
  static const uint8_t RING_BRIGHTNESS_MAX = 0xF5;  // left half of ring — brightest
  static const uint8_t RING_TEMP_MIN       = 0x15;  // right half of ring — warmest
  static const uint8_t RING_TEMP_MAX       = 0x75;  // right half of ring — coldest
};
