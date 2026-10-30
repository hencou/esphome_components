#pragma once
#include "V2PacketFormatter.h"
#include "MiLightRemoteType.h"

#define S2_NUM_MODES 9

#define S2_COLOR_OFFSET 0x5F

enum S2Command {
  S2_ON_OFF = 0x01,
  S2_COLOR = 0x02,
  S2_KELVIN = 0x03,
  S2_SATURATION = 0x04,
  S2_BRIGHTNESS = 0x05,
  S2_MODE = 0x06
};

class S2PacketFormatter : public V2PacketFormatter {
public:
  S2PacketFormatter()
    : V2PacketFormatter(REMOTE_TYPE_S2, 0x21, 4),
      lastMode(0)
  { }

  virtual void updateBrightness(uint8_t value);
  virtual void updateHue(uint16_t value);
  virtual void updateColorRaw(uint8_t value);
  virtual void updateColorWhite();
  virtual void updateTemperature(uint8_t value);
  virtual void updateSaturation(uint8_t value);
  virtual void enableNightMode();

  virtual void updateMode(uint8_t mode);
  virtual void nextMode();
  virtual void previousMode();

  virtual BulbId parsePacket(const uint8_t* packet, JsonObject result);

protected:

  uint8_t lastMode;
};
