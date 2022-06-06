#include <cinttypes>
#include <Arduino.h>
#include "SHTSensor.h"

#include "esphome/core/log.h"
#include "esphome.h"

namespace esphome
{
  namespace itho
  {
    static const char *const TAG = "itho.sensor";

    SHTSensor::SHTSensor(IthoI2C *&ithoI2C, uint8_t i2cAddress)
        : mIthoI2C(ithoI2C), mI2cAddress(i2cAddress) {}

    bool SHTSensor::readFromI2c(const uint8_t *i2cCommand,
                                uint8_t commandLength, uint8_t *data,
                                uint8_t dataLength,
                                uint8_t duration)
    {
      
      std::string addr;
      addr += toHex(mI2cAddress >> 4);
      addr += ' ';
      addr += toHex(mI2cAddress & 0xF);

      esp_err_t rc;
      rc = mIthoI2C->i2c_master_send_command(mI2cAddress, i2cCommand, commandLength);
      if (rc) {return false;}

      vTaskDelay(duration / portTICK_RATE_MS);

      rc = mIthoI2C->i2c_master_read_slave(mI2cAddress, data, dataLength);
      if (rc) {return false;}

      return true;
    }

    uint8_t SHTSensor::crc8(const uint8_t *data, uint8_t len)
    {

      uint8_t crc = 0xff;
      uint8_t byteCtr;
      for (byteCtr = 0; byteCtr < len; ++byteCtr)
      {
        crc ^= data[byteCtr];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
          if (crc & 0x80)
          {
            crc = (crc << 1) ^ 0x31;
          }
          else
          {
            crc = (crc << 1);
          }
        }
      }
      return crc;
    }

    bool SHTSensor::readSample()
    {

      uint8_t data[EXPECTED_DATA_SIZE];
      uint8_t cmd[CMD_SIZE];

      cmd[0] = mI2cCommand >> 8;
      cmd[1] = mI2cCommand & 0xff;

      if (!readFromI2c(cmd, CMD_SIZE, data,
                       EXPECTED_DATA_SIZE, mDuration))
      {
        ESP_LOGE(TAG, "SHTSensor::readSample readFromI2c returned false!");
        return false;
      }

      if (crc8(&data[0], 2) != data[2] || crc8(&data[3], 2) != data[5])
      {
        ESP_LOGE(TAG, "SHTSensor::readSample returned crc false!");
        return false;
      }

      uint16_t val;
      val = (data[0] << 8) + data[1];
      mTemperature = mA + mB * (val / mC);

      val = (data[3] << 8) + data[4];
      mHumidity = mX * (val / mY);

      return true;
    }

    bool SHTSensor::setAccuracy(SHTAccuracy newAccuracy)
    {
      switch (newAccuracy)
      {
      case SHTSensor::SHT_ACCURACY_HIGH:
        mI2cCommand = SHT3X_ACCURACY_HIGH;
        mDuration = SHT3X_ACCURACY_HIGH_DURATION;
        break;
      case SHTSensor::SHT_ACCURACY_MEDIUM:
        mI2cCommand = SHT3X_ACCURACY_MEDIUM;
        mDuration = SHT3X_ACCURACY_MEDIUM_DURATION;
        break;
      case SHTSensor::SHT_ACCURACY_LOW:
        mI2cCommand = SHT3X_ACCURACY_LOW;
        mDuration = SHT3X_ACCURACY_LOW_DURATION;
        break;
      default:
        return false;
      }
      return true;
    }

    char SHTSensor::toHex(uint8_t c)
    {
      return c < 10 ? c + '0' : c + 'A' - 10;
    }

    std::string SHTSensor::i2cbuf2string(const uint8_t *data, size_t len)
    {
      std::string s;
      s.reserve(len * 3 + 2);
      for (size_t i = 0; i < len; ++i)
      {
        if (i)
          s += ' ';
        s += toHex(data[i] >> 4);
        s += toHex(data[i] & 0xF);
      }
      return s;
    }

  }
}