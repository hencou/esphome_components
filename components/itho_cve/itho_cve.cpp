#include "itho_cve.h"

#include "esphome/core/log.h"
#include "esphome.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/component.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome {
namespace itho_cve {

static const char *const TAG = "itho_cve";

bool TaskInitReady = false;
bool IthoInit = false;

TaskHandle_t xTaskInitHandle = NULL;
TaskHandle_t xTaskSysControlHandle = NULL;
Ticker TaskConfigAndLogTimeout;

StaticTask_t xTaskInitBuffer;
StaticTask_t xTaskSysControlBuffer;

StackType_t xTaskInitStack[ STACK_SIZE ];
StackType_t xTaskSysControlStack[ STACK_SIZE ];

IthoRemote virtualRemotes;

uint32_t TaskInitHWmark = 0;
uint32_t TaskSysControlHWmark = 0;
bool i2cStartCommands = false;
bool joinSend = false;
unsigned long lastI2CinitRequest = 0;
int8_t ithoInitResult = 0;
bool ithoInitResultLogEntry = true;
bool clearQueue = false;
unsigned long lastVersionCheck;
unsigned long query2401tim = 0;

uint16_t ithoCurrentSpeed;

std::string ithoFanInfo;

void TaskInit( void * parameter ) {
  configASSERT( ( uint32_t ) parameter == 1UL );
  Ticker TaskTimeout;

  mutexI2Ctask = xSemaphoreCreateMutex();
  hardwareInit();
  startTaskSysControl();

  while (!TaskInitReady) {
    yield();
  }

  ESP_LOGD(TAG, "TaskInit Itho done");

  vTaskDelete( NULL );
}

void hardwareInit() {
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);
  pinMode(STATUSPIN, INPUT_PULLUP);
  pinMode(ITHOSTATUS, OUTPUT);
  digitalWrite(ITHOSTATUS, LOW);

  i2cInit();

  IthoInit = true;
  TaskInitReady = true;
}

void i2cInit() {
  i2c_master_init();
}

void startTaskSysControl() {
  xTaskSysControlHandle = xTaskCreateStaticPinnedToCore(
    TaskSysControl,
    "TaskSysControl",
    STACK_SIZE,
    ( void * ) 1,
    TASK_SYS_CONTROL_PRIO,
    xTaskSysControlStack,
    &xTaskSysControlBuffer,
    CONFIG_ARDUINO_RUNNING_CORE
  );
}

void TaskSysControl( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );
  Ticker TaskTimeout;

  init_vRemote();

  for (;;) {

    TaskTimeout.once_ms(35000, []() {
      ESP_LOGD(TAG, "Error: Task SysControl timed out!");
    });

    execSystemControlTasks();

    TaskSysControlHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void init_vRemote() {

  static uint8_t mac[6];
  esphome::get_mac_address_raw(mac);

  id0 = mac[3];
  id1 = mac[4];
  id2 = mac[5];

  ESP_LOGD(TAG, "Setup: Virtual remote ID: %d,%d,%d", id0, id1, id2);

  virtualRemotes.setMaxRemotes(systemConfig.itho_numvrem);
  loadVirtualRemotesConfig();
}

void execSystemControlTasks() {
  if (IthoInit && millis() > 250) {
    IthoInit = ithoInitCheck();
  }

  if (!i2cStartCommands && millis() > 15000 && (millis() - lastI2CinitRequest > 5000) ) {
    lastI2CinitRequest = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQueryDevicetype(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (itho_fwversion > 0) {
      ESP_LOGD(TAG, "I2C init: QueryDevicetype - fw:%d hw:%d", itho_fwversion, ithoDeviceID);

      ithoInitResult = 1;
      i2cStartCommands = true;

      digitalWrite(ITHOSTATUS, HIGH);

      if (systemConfig.syssht30 > 0) {
        if (ithoDeviceID == 0x1B) {

          if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            i2c_result_updateweb = false;

            index2410 = 0;
            if (systemConfig.syssht30 == 1) {
              value2410 = 0;
            }
            else if (systemConfig.syssht30 == 2) {
              value2410 = 1;
            }
            if (itho_fwversion == 25) {
              index2410 = 63;
            }
            else if (itho_fwversion == 26 || itho_fwversion == 27) {
              index2410 = 71;
            }
            if (index2410 > 0) {
              sendQuery2410(i2c_result_updateweb);
              setSetting2410(i2c_result_updateweb);
              ESP_LOGD(TAG, "I2C init: set hum sensor in itho firmware to: %s", value2410 ? "on" : "off");
            }
            xSemaphoreGive(mutexI2Ctask);
          }
        }
        if (systemConfig.syssht30 == 2) {
          systemConfig.syssht30 = 0;
        }
      }
      if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
        sendQueryStatusFormat(false);
        ESP_LOGD(TAG, "I2C init: QueryStatusFormat - items:%d", ithoStatus.size());
        xSemaphoreGive(mutexI2Ctask);
      }
      if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
        sendQueryStatus(false);
        ESP_LOGD(TAG, "I2C init: QueryStatus");
        xSemaphoreGive(mutexI2Ctask);
      }
    }
    else {
      ithoInitResult = -1;
      if (ithoInitResultLogEntry) {
        ithoInitResultLogEntry = false;
        ESP_LOGE(TAG, "I2C init: QueryDevicetype - failed");
      }
    }
  }
  
  if (!joinSend && ithoInitResult == 1) {
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      joinSend = true;
      sendRemoteCmd(0, IthoJoin, virtualRemotes);
      xSemaphoreGive(mutexI2Ctask);
      ESP_LOGD(TAG, "I2C init: Virtual remote join command send with id: %d, %d, %d", id0, id1, id2);
    }
  }
  
  if (clearQueue) {
    clearQueue = false;
    ithoQueue.clear_queue();
  }

  if (ithoQueue.ithoSpeedUpdated) {
    uint16_t speed = ithoQueue.get_itho_speed();
    char buf[32] {};
    sprintf(buf, "speed:%d", speed);
    if (!writeIthoVal( speed )) {
      ESP_LOGE(TAG, "ithoQueue_error");
    }
    ithoQueue.ithoSpeedUpdated = false;
  }

  if (!(itho_fwversion > 0) && millis() - lastVersionCheck > 60000) {
    lastVersionCheck = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQueryDevicetype(false);
      xSemaphoreGive(mutexI2Ctask);
    }
  }

  if (millis() - query2401tim >= systemConfig.itho_updatefreq * 1000UL && i2cStartCommands) {
    query2401tim = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQueryStatus(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQuery31DA(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQuery31D9(false);
      xSemaphoreGive(mutexI2Ctask);
    }

    if (!ithoInternalMeasurements.empty()) {
      for (const auto& internalMeasurement : ithoInternalMeasurements) {
        if (strcmp(internalMeasurement.name, "Speed status") == 0) { 
          if (!ithoQueue.ithoSpeedUpdated)   {
            ithoCurrentSpeed = static_cast<int>(internalMeasurement.value.floatval * 2.55);
          }
        }
      }
    }

    if (!ithoMeasurements.empty()) {
      for (const auto& ithoMeasurement : ithoMeasurements) {
        if (strcmp(ithoMeasurement.name, "FanInfo") == 0) {
          ithoFanInfo = ithoMeasurement.value.stringval;
        }
      }
    }
  }

  if (get2410) {
    get2410 = false;
    resultPtr2410 = sendQuery2410(i2c_result_updateweb);
    xSemaphoreGive(mutexI2Ctask);
  }
  if (set2410) {
    setSetting2410(i2c_result_updateweb);
    set2410 = false;
    xSemaphoreGive(mutexI2Ctask);
    getSettingsHack.once_ms(1, []() {
      getSetting(index2410, true, false, false);
    });
  }
}

bool ithoInitCheck() {

  if (digitalRead(STATUSPIN) == LOW) {
    return false;
  }
  sendI2CPWMinit();
  return false;
}

bool writeIthoVal(uint16_t value) {

  if (value > 254) value = 254;
  if (ithoCurrentSpeed != value) {

    ESP_LOGD(TAG, "writeIthoVal: %d", value);

    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
    }
    else {
      return false;
    }
    uint16_t valTemp = ithoCurrentSpeed;
    ithoCurrentSpeed = value;

    int timeout = 0;
    while (digitalRead(SCLPIN) == LOW && timeout < 1000) {
      yield();
      delay(1);
      timeout++;
    }
    if (timeout != 1000) {

      sendRemoteCmd(0, IthoMedium, virtualRemotes);

      uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};
      uint8_t b = (uint8_t) value;

      command[6] = b;
      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

      i2c_sendBytes(command, sizeof(command));

    }
    else {
      ESP_LOGE(TAG, "Warning: I2C timeout");
      ithoCurrentSpeed = valTemp;
      ithoQueue.add2queue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
    }

    xSemaphoreGive(mutexI2Ctask);
    return true;
  }
  return false;
}

bool loadVirtualRemotesConfig() {

  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 400));
  DeserializationError err = deserializeJson(doc, "{\"vremotes\":[{\"index\":0,\"id\":[0,0,0],\"name\":\"remote0\",\"remtype\":0,\"capabilities\":null}],\"remfunc\":0,\"version_of_program\":\"002\"}");

  doc.shrinkToFit();

  static uint8_t mac[6];
  esphome::get_mac_address_raw(mac);

  doc["vremotes"][0]["id"][0] = mac[3];
  doc["vremotes"][0]["id"][1] = mac[4];
  doc["vremotes"][0]["id"][2] = mac[5];
  doc["vremotes"][0]["remtype"] = RFTCVE;

  virtualRemotes.configLoaded = virtualRemotes.set(doc.as<JsonObject>(), "vremotes");
  return true;
}

bool IthoCVE::ithoI2CCommand(uint8_t remoteIndex, const std::string &command) {
  ESP_LOGD(TAG, "EXEC VREMOTE BUTTON COMMAND: %s remote: %d", command.c_str(), remoteIndex);

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
  }
  else {
    return false;
  }

  int timeout = 0;
  while (digitalRead(SCLPIN) == LOW && timeout < 1000) {
    yield();
    delay(1);
    timeout++;
  }
  if (timeout != 1000) {

    if (strcmp(command.c_str(), "away") == 0) {
      sendRemoteCmd(remoteIndex, IthoAway, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "low") == 0) {
      sendRemoteCmd(remoteIndex, IthoLow, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "medium") == 0) {
      sendRemoteCmd(remoteIndex, IthoMedium, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "high") == 0) {
      sendRemoteCmd(remoteIndex, IthoHigh, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "timer1") == 0) {
      sendRemoteCmd(remoteIndex, IthoTimer1, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "timer2") == 0) {
      sendRemoteCmd(remoteIndex, IthoTimer2, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "timer3") == 0) {
      sendRemoteCmd(remoteIndex, IthoTimer3, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "cook30") == 0) {
      sendRemoteCmd(remoteIndex, IthoCook30, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "cook60") == 0) {
      sendRemoteCmd(remoteIndex, IthoCook60, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "auto") == 0) {
      sendRemoteCmd(remoteIndex, IthoAuto, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "autonight") == 0) {
      sendRemoteCmd(remoteIndex, IthoAutoNight, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "join") == 0) {
      sendRemoteCmd(remoteIndex, IthoJoin, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "leave") == 0) {
      sendRemoteCmd(remoteIndex, IthoLeave, virtualRemotes);
    }
    else if (strcmp(command.c_str(), "type") == 0) {
      sendQueryDevicetype(false);
    }
    else if (strcmp(command.c_str(), "status") == 0) {
      sendQueryStatus(false);
    }
    else if (strcmp(command.c_str(), "statusformat") == 0) {
      sendQueryStatusFormat(false);
    }
    else if (strcmp(command.c_str(), "31DA") == 0) {
      sendQuery31DA(false);
    }
    else if (strcmp(command.c_str(), "31D9") == 0) {
      sendQuery31D9(false);
    }
    else if (strcmp(command.c_str(), "10D0") == 0) {
      filterReset();
    }
    else {

      xSemaphoreGive(mutexI2Ctask);

      return false;
    }
  }
  else {
    ESP_LOGE(TAG, "Warning: I2C timeout");
  }

  xSemaphoreGive(mutexI2Ctask);
  return true;
}

bool IthoCVE::ithoSetSpeed(uint16_t speed) {
  ESP_LOGD(TAG, "SET SPEED:%d", speed);
  if (speed < 255) {
    ithoQueue.add2queue(speed, 0, systemConfig.nonQ_cmd_clearsQ);
  }
  else {
    return false;
  }
  return true;
}

bool IthoCVE::ithoSetTimer(uint16_t timer) {
  ESP_LOGD(TAG, "SET TIMER:%dmin\n", timer);
  if (timer > 0 && timer < 65535) {
    ithoQueue.add2queue(systemConfig.itho_high, timer, systemConfig.nonQ_cmd_clearsQ);
  }
  else {
    return false;
  }
  return true;
}

bool IthoCVE::ithoSetSpeedTimer(uint16_t speed, uint16_t timer) {
  ESP_LOGD(TAG, "SET SPEED AND TIMER\n");
  if (speed < 255) {
    ithoCurrentSpeed = speed;
    ithoQueue.add2queue(speed, timer, systemConfig.nonQ_cmd_clearsQ);
  }
  else {
    return false;
  }
  return true;
}

void IthoCVE::loop() {}

void IthoCVE::setup() {

  ESP_LOGD(TAG, "Setup Itho Core start");

  xTaskInitHandle = xTaskCreateStaticPinnedToCore(
    TaskInit,
    "TaskInit",
    STACK_SIZE,
    ( void * ) 1,
    TASK_MAIN_PRIO,
    xTaskInitStack,
    &xTaskInitBuffer,
    CONFIG_ARDUINO_RUNNING_CORE);

  ESP_LOGD(TAG, "Setup Itho Core finished");
}

uint16_t IthoCVE::ithoGetSpeed() {return ithoCurrentSpeed;}
std::string IthoCVE::ithoGetFanInfo() {return ithoFanInfo;}

}  // namespace itho_cve
}  // namespace esphome
