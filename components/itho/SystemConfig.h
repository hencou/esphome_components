#pragma once

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>

namespace esphome
{
  namespace itho
  {

    class SystemConfig
    {
    public:
      SystemConfig() {};
      ~SystemConfig() {};

      uint8_t getSysSHT30() { return syssht30; }
      void setSysSHT30(uint8_t value) { syssht30 = value; }

      uint8_t getIthoLow() { return itho_low; }
      void setIthoLow(uint8_t value) { itho_low = value; }

      uint8_t getIthoMedium() { return itho_medium; }
      void setIthoMedium(uint8_t value) { itho_medium = value; }

      uint8_t getIthoHigh() { return itho_high; }
      void setIthoHigh(uint8_t value) { itho_high = value; }

      uint16_t getIthoTimer1() { return itho_timer1; }
      void setIthoTimer1(uint16_t value) { itho_timer1 = value; }

      uint16_t getIthoTimer2() { return itho_timer2; }
      void setIthoTimer2(uint16_t value) { itho_timer2 = value; }

      uint16_t getIthoTimer3() { return itho_timer3; }
      void setIthoTimer3(uint16_t value) { itho_timer3 = value; }

    private:
      uint8_t syssht30 = 2;

      uint8_t itho_low = 20;
      uint8_t itho_medium = 120;
      uint8_t itho_high = 220;
      uint16_t itho_timer1 = 10;
      uint16_t itho_timer2 = 20;
      uint16_t itho_timer3 = 30;
    };
    
  }
}