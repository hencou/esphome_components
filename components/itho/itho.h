#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c_esp32/i2c_esp32.h"

#include <Arduino.h>
#include <Ticker.h>

#include "IthoSystem.h"
#include "SystemConfig.h"

namespace esphome
{
  namespace itho
  {

#define WIFILED 17
#define STATUSPIN 16
#define ITHOSTATUS 13


    class Itho : public Component, public i2c_esp32::I2CDevice
    {
    public:
      Itho();
      ~Itho();

      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const { return setup_priority::DATA; }

      bool ithoI2CCommand(uint8_t remoteIndex, const std::string &command);

      bool setIthoSpeed(uint16_t speed);

      double getIthoTemperature() { return ithoSystem->getIthoTemperature(); }
      double getIthoHumidity() { return ithoSystem->getIthoHumidity(); }
      uint16_t getIthoSpeed() { return ithoSystem->getIthoSpeed(); }
      std::string getIthoFanInfo() { return ithoSystem->getIthoFanInfo(); }

      void setSysSHT30(uint8_t value) { systemConfig->setSysSHT30(value); }
      void setIthoLow(uint8_t value) { systemConfig->setIthoLow(value); }
      void setIthoMedium(uint8_t value) { systemConfig->setIthoMedium(value); }
      void setIthoHigh(uint8_t value) { systemConfig->setIthoHigh(value); }
      void setIthoTimer1(uint16_t value) { systemConfig->setIthoTimer1(value); }
      void setIthoTimer2(uint16_t value) { systemConfig->setIthoTimer2(value); }
      void setIthoTimer3(uint16_t value) { systemConfig->setIthoTimer3(value); }

      void write_bytes_raw_callback(const uint8_t *buffer, uint32_t len);
      void slave_receive_callback();

    private:
      void execSystemControlTasks();
      bool ithoInitCheck();
      bool ithoExecCommand(const char *command);
      bool ithoI2CCommand(uint8_t remoteIndex, const char *command);
      bool loadVirtualRemotesConfig();

      bool IthoInit = false;

      IthoRemote virtualRemotes;
      IthoSystem *ithoSystem;
      SystemConfig *systemConfig;

      bool i2cStartCommands = false;
      bool joinSend = false;
      unsigned long lastI2CinitRequest = 0;
      int8_t ithoInitResult = 0;
      unsigned long lastVersionCheck;
      unsigned long query2401time = 0;
    };

  } // namespace itho
} // namespace esphome
