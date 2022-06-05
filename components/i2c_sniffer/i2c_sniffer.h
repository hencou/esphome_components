#pragma once
#include "esphome/core/component.h"
#include <Arduino.h>

namespace esphome
{
  namespace i2c_sniffer
  {

    static uint8_t sda_pin = 21;
    static uint8_t scl_pin = 22;

    class I2cSniffer : public Component
    {
    public:
      I2cSniffer();
      ~I2cSniffer();

      void setup() override;
      void loop() override;
      void dump_config() override;

      void set_sda_pin(uint8_t value) {sda_pin = value;}
      void set_scl_pin(uint8_t value) {scl_pin = value;}

    private:
      bool buf[4096];
      bool *bufI = &buf[0];
      const uint32_t PAUSE = 1 << 20;

      uint32_t waitCl(bool expected);
    };

  } // namespace i2c_sniffer
} // namespace esphome
