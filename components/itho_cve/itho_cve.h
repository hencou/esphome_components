#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Ticker.h>

#include "IthoSystem.h"
#include "i2c_esp32.h"
#include "SystemConfig.h"
#include "IthoQueue.h"

namespace esphome {
namespace itho_cve {

  #define WIFILED      17
  #define STATUSPIN    16
  #define ITHOSTATUS   13

  #define STACK_SIZE 4096
  #define STACK_SIZE_LARGE 8192
  #define TASK_MAIN_PRIO 5
  #define TASK_CONFIG_AND_LOG_PRIO 5
  #define TASK_SYS_CONTROL_PRIO 5

  void TaskInit( void *parameter );
  void TaskSysControl( void *parameter );
  void startTaskSysControl();
  void execSystemControlTasks();
  void init_vRemote();
  void hardwareInit();
  void i2cInit();
  bool ithoInitCheck();
  bool writeIthoVal(uint16_t value);
  bool ithoExecCommand(const char* command);
  bool ithoI2CCommand(uint8_t remoteIndex, const char* command);
  bool loadVirtualRemotesConfig();

  class IthoCVE : public Component {
  public:

    void setup() override;
    void loop() override;

    bool ithoI2CCommand(uint8_t remoteIndex, const std::string &command);
    std::string ithoGetFanInfo();
    bool ithoSetSpeed(uint16_t speed);
    bool ithoGetSpeedUpdated() {return ithoQueue.ithoSpeedUpdated;}
    uint16_t ithoGetSpeed();

    bool ithoSetTimer(uint16_t timer);
    bool ithoSetSpeedTimer(uint16_t speed, uint16_t timer);

    double ithoGetTemperature() {return ithoTemp;}
    double ithoGetHumidity() {return ithoHum;}
  
  private:
    
  };

}  // namespace itho_cve
}  // namespace esphome
