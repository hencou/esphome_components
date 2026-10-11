#pragma once

#include "esphome/core/component.h"

#include "IthoSystem.h"
#include "IthoQueue.h"
#include "SystemConfig.h"

namespace esphome
{
  namespace itho
  {

#define SPEED_COMMAND_DELAY 1000

    class Itho : public Component
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

      double getIthoError() { return ithoSystem->getIthoError(); }
      double getIthoStartupCounter() { return ithoSystem->getIthoStartupCounter(); }
      double getIthoOperationTime() { return ithoSystem->getIthoOperationTime(); }
      double getIthoTemperature() { return ithoSystem->getIthoTemperature(); }
      double getIthoHumidity() { return ithoSystem->getIthoHumidity(); }
      double getIthoFanSetpoint() { return ithoSystem->getIthoFanSetpoint(); }
      double getIthoFanSpeed() { return ithoSystem->getIthoFanSpeed(); }
      uint16_t getIthoSpeed() { return ithoSystem->getIthoSpeed(); }
      std::string getIthoFanInfo() { return ithoSystem->getIthoFanInfo(); }

      double getIthoAirQuality() { return ithoSystem->getIthoMeasurement("AirQuality (%)"); }
      double getIthoCO2() { return ithoSystem->getIthoMeasurement("CO2level (ppm)"); }
      double getIthoIndoorHumidity() { return ithoSystem->getIthoMeasurement("Indoorhumidity (%)"); }
      double getIthoOutdoorHumidity() { return ithoSystem->getIthoMeasurement("Outdoorhumidity (%)"); }
      double getIthoExhaustTemperature() { return ithoSystem->getIthoMeasurement("Exhausttemp (°C)"); }
      double getIthoSupplyTemperature() { return ithoSystem->getIthoMeasurement("SupplyTemp (°C)"); }
      double getIthoIndoorTemperature() { return ithoSystem->getIthoMeasurement("IndoorTemp (°C)"); }
      double getIthoOutdoorTemperature() { return ithoSystem->getIthoMeasurement("OutdoorTemp (°C)"); }
      double getIthoSpeedCap() { return ithoSystem->getIthoMeasurement("SpeedCap"); }
      double getIthoBypassPosition() { return ithoSystem->getIthoMeasurement("BypassPos (%)"); }
      double getIthoExhaustFanSpeed() { return ithoSystem->getIthoMeasurement("ExhFanSpeed (%)"); }
      double getIthoSupplyFanSpeed() { return ithoSystem->getIthoMeasurement("SpeedInFan (%)"); }
      double getIthoRemainingTime() { return ithoSystem->getIthoMeasurement("RemainingTime (min)"); }
      double getIthoPostHeat() { return ithoSystem->getIthoMeasurement("PostHeat (%)"); }
      double getIthoPreHeat() { return ithoSystem->getIthoMeasurement("PreHeat (%)"); }
      double getIthoSupplyFlow() { return ithoSystem->getIthoMeasurement("InFlow (l sec)"); }
      double getIthoExhaustFlow() { return ithoSystem->getIthoMeasurement("ExhFlow (l sec)"); }
      double getIthoInternalFault() { return ithoSystem->getIthoMeasurement("Internal fault"); }
      double getIthoFrostCycle() { return ithoSystem->getIthoMeasurement("Frost cycle"); }
      double getIthoFilterDirty() { return ithoSystem->getIthoMeasurement("Filter dirty"); }

      void setSysSHT30(uint8_t value) { systemConfig->setSysSHT30(value); }
      void setSysSHT30_Address(uint8_t value) { systemConfig->setSysSHT30_Address(value); }
      void setI2C_SDA_Pin(uint8_t value) { systemConfig->setI2C_SDA_Pin(value); }
      void setI2C_SCL_Pin(uint8_t value) { systemConfig->setI2C_SCL_Pin(value); }
      void setStatusInputPin(uint8_t value) { statusInputPin = value; }
      void setStatusOutputPin(uint8_t value) { statusOutputPin = value; }
      void setRemoteType(uint16_t value) { remoteType = static_cast<RemoteTypes>(value); }

      uint8_t getSCLPin() { return systemConfig->getI2C_SCL_Pin(); }

      volatile bool lowSCL = false;

    private:
      void execSystemControlTasks();
      bool ithoInitCheck();
      bool ithoExecCommand(const char *command);
      bool ithoI2CCommand(uint8_t remoteIndex, const char *command);
      bool loadVirtualRemotesConfig();
      
      IthoRemote virtualRemotes;
      IthoSystem *ithoSystem;
      SystemConfig *systemConfig;
      IthoQueue *ithoQueue;

      bool IthoInit = false;
      bool i2cStartCommands = false;
      bool joinSend = false;
      unsigned long lastI2CinitRequest = 0;
      int8_t ithoInitResult = 0;
      unsigned long lastVersionCheck;
      unsigned long lastSCLLowTime = 0;
      bool loopSystemControlTasks = true;
      unsigned long loopSystemControlTasksTime = 0;
      unsigned long speedCommandTime = 0;
      uint8_t statusInputPin = 16;
      uint8_t statusOutputPin = 13;
      RemoteTypes remoteType = RemoteTypes::RFTCVE;
    };

  } // namespace itho
} // namespace esphome
