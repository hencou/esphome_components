#pragma once
#include "esphome/core/component.h"
#include <Arduino.h>

namespace esphome
{
  namespace i2c_sniffer
  {

    class I2cSniffer : public Component
    {
    public:
      I2cSniffer();
      ~I2cSniffer();

      void setup() override;
      void loop() override;
      void dump_config() override;

      void set_sda_pin(uint8_t value) {sda_pin_ = value;}
      void set_scl_pin(uint8_t value) {scl_pin_ = value;}

    private:
      void processDataBuffer();
      void resetI2cVariable();
      char toHex(uint8_t c);

      unsigned long loopTimer;

      uint8_t sda_pin_ = 21;
      uint8_t scl_pin_ = 22;
    };

  } // namespace i2c_sniffer
} // namespace esphome
