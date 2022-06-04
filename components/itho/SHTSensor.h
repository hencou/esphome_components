#pragma once

#include <cinttypes>
#include "IthoI2C.h"

namespace esphome
{
  namespace itho
  {

    class SHTSensor
    {
    public:

      static const uint16_t SHT3X_ACCURACY_HIGH = 0x2400;
      static const uint16_t SHT3X_ACCURACY_MEDIUM = 0x240b;
      static const uint16_t SHT3X_ACCURACY_LOW = 0x2416;

      static const uint8_t SHT3X_ACCURACY_HIGH_DURATION = 15;
      static const uint8_t SHT3X_ACCURACY_MEDIUM_DURATION = 6;
      static const uint8_t SHT3X_ACCURACY_LOW_DURATION = 4;

      SHTSensor(IthoI2C*& ithoI2C, uint8_t i2cAddress);
      ~SHTSensor();

      bool readSample();

      float getHumidity() { return mHumidity; }
      float getTemperature() { return mTemperature; }

      enum SHTAccuracy
      {
        SHT_ACCURACY_HIGH,
        SHT_ACCURACY_MEDIUM,
        SHT_ACCURACY_LOW
      };

      bool setAccuracy(SHTAccuracy newAccuracy);

    private:
      const uint8_t CMD_SIZE = 2;
      const uint8_t EXPECTED_DATA_SIZE = 6;

      IthoI2C*& mIthoI2C;

      float mTemperature;
      float mHumidity;

      static uint8_t crc8(const uint8_t *data, uint8_t len);
      bool readFromI2c(const uint8_t *i2cCommand,
                              uint8_t commandLength, uint8_t *data,
                              uint8_t dataLength, uint8_t duration);

      const uint8_t mI2cAddress;
      uint16_t mI2cCommand;
      uint8_t mDuration;
      float mA = -45;
      float mB = 175;
      float mC = 65535;
      float mX = 100;
      float mY = 65535;
    };

  }
}