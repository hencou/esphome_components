#include "itho.h"

#include "esphome/core/log.h"
#include "esphome.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/component.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome
{
  namespace itho
  {

    static const char *const TAG = "itho";

    Itho::Itho()
    {
      systemConfig = new SystemConfig();
    }

    void Itho::TaskSysControl(void *pvParameters)
    {
      Ticker TaskTimeout;
      Itho *l_pThis = (Itho *)pvParameters;

      vTaskDelay(3000 / portTICK_RATE_MS);

      for (;;)
      {
        TaskTimeout.once_ms(35000, []()
                            { ESP_LOGD(TAG, "Error: Task SysControl timed out!"); });

        l_pThis->execSystemControlTasks();
        vTaskDelay(25 / portTICK_PERIOD_MS);
      }
      vTaskDelete(NULL);
    }

    ///detect rising edge of I2C SCL pin
    uint8_t scl_pin;
    bool lowSCL = false;
    void IRAM_ATTR gpio_intr()
    {
      lowSCL = true;
      detachInterrupt(digitalPinToInterrupt(scl_pin));
    }
    ///end detect rising edge of I2C SCL pin

    void Itho::execSystemControlTasks()
    {
      //// Only run once after a 2 seconds of inactivity on the I2C bus. Itho queries the bus every 8 seconds with enabled sensor, or everty 2 minutes with disabled sensor
      if (lowSCL == true) {
        lowSCL = false;
        this->lastSCLLowTime = millis();
      }
      if (millis() - lastSCLLowTime < 2000 || digitalRead(systemConfig->getI2C_SCL_Pin()) == LOW) {
        loopSystemControlTasks = true;
        return;
      }
      //// end "only run"
      
      if (this->IthoInit && millis() > 250)
      {
        this->IthoInit = this->ithoInitCheck();
      }

      if (!this->i2cStartCommands && millis() > 15000UL && (millis() - this->lastI2CinitRequest > 5000))
      {

        this->lastI2CinitRequest = millis();
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->sendQueryDevicetype();
          xSemaphoreGive(this->mutexI2Ctask);
        }

        if (this->ithoSystem->getIthoFwVersion() > 0)
        {
          ESP_LOGD(TAG, "I2C init: QueryDevicetype - fw:%d hw:%d", this->ithoSystem->getIthoFwVersion(), this->ithoSystem->getIthoDeviceID());

          this->ithoInitResult = 1;
          this->i2cStartCommands = true;

          digitalWrite(ITHOSTATUS, HIGH);

          if (this->systemConfig->getSysSHT30() > 0)
          {
            if (this->ithoSystem->getIthoDeviceID() == 27)
            {
              if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)1000 / portTICK_PERIOD_MS) == pdTRUE)
              {

                uint8_t index2410 = 0;
                int32_t value2410 = 0;
                if (this->systemConfig->getSysSHT30() == 1)
                {
                  value2410 = 0;
                }
                else if (this->systemConfig->getSysSHT30() == 2)
                {
                  value2410 = 1;
                }
                if (this->ithoSystem->getIthoFwVersion() == 25)
                {
                  index2410 = 63;
                }
                else if (this->ithoSystem->getIthoFwVersion() == 26 || this->ithoSystem->getIthoFwVersion() == 27)
                {
                  index2410 = 71;
                }
                if (index2410 > 0)
                {
                  this->ithoSystem->setIndex2410(index2410);
                  this->ithoSystem->setValue2410(value2410);
                  this->ithoSystem->sendQuery2410();
                  this->ithoSystem->setSetting2410();
                  ESP_LOGD(TAG, "I2C init: set hum sensor in itho firmware to: %s", value2410 ? "on" : "off");
                }
                xSemaphoreGive(this->mutexI2Ctask);
              }
            }
            if (this->systemConfig->getSysSHT30() == 2)
            {
              this->systemConfig->setSysSHT30(0);
            }
          }
          if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)1000 / portTICK_PERIOD_MS) == pdTRUE)
          {
            this->ithoSystem->sendQueryStatusFormat();
            xSemaphoreGive(this->mutexI2Ctask);
          }
          if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)1000 / portTICK_PERIOD_MS) == pdTRUE)
          {
            this->ithoSystem->sendQueryStatus();
            xSemaphoreGive(this->mutexI2Ctask);
          }
        }
        else
        {
          this->ithoInitResult = -1;
        }
      }

      if (this->ithoQueue->getIthoSpeedUpdated())
      {
        uint16_t speed = this->ithoQueue->getIthoSpeed();

        // ESP_LOGD(TAG, "Set FanInfo on auto...");
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->sendRemoteCmd(0, IthoMedium, this->virtualRemotes);
          xSemaphoreGive(this->mutexI2Ctask);
        }

        vTaskDelay(1000 / portTICK_RATE_MS);

        // ESP_LOGD(TAG, "Set speed to: %d", speed);
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};
          uint8_t b = (uint8_t)speed;

          command[6] = b;
          command[sizeof(command) - 1] = this->ithoSystem->checksum(command, sizeof(command) - 1);

          this->ithoSystem->i2c_sendBytes(command, sizeof(command));
          xSemaphoreGive(this->mutexI2Ctask);
        }
        this->ithoQueue->setIthoSpeedUpdated(false);
      }

      if (!this->joinSend && this->ithoInitResult == 1)
      {
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->joinSend = true;
          this->ithoSystem->sendRemoteCmd(0, IthoJoin, virtualRemotes);
          uint8_t id[3];
          this->ithoSystem->getID(id);
          ESP_LOGD(TAG, "I2C init: Virtual remote join command send with id: %d, %d, %d", id[0], id[1], id[2]);
          xSemaphoreGive(this->mutexI2Ctask);
        }
      }

      if (!(this->ithoSystem->getIthoFwVersion() > 0) && millis() - lastVersionCheck > 60000)
      {
        lastVersionCheck = millis();
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->sendQueryDevicetype();
          xSemaphoreGive(this->mutexI2Ctask);
        }
      }

      if (!loopSystemControlTasks || millis() - loopSystemControlTasksTime < 10000UL) {
        return;
      }
      loopSystemControlTasks = false;
      loopSystemControlTasksTime = millis();

      if (this->i2cStartCommands)
      {
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->sendQueryStatus();
          xSemaphoreGive(this->mutexI2Ctask);
        }
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->sendQuery31DA();
          xSemaphoreGive(this->mutexI2Ctask);
        }
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->sendQuery31D9();
          xSemaphoreGive(this->mutexI2Ctask);
        }
      }

      if (this->ithoSystem->getUpdated2410())
      {
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->setUpdated2410(false);
          this->ithoSystem->sendQuery2410();
          xSemaphoreGive(this->mutexI2Ctask);
        }
      }

      if (this->ithoSystem->getUpdate2410())
      {
        if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {
          this->ithoSystem->setSetting2410();
          this->ithoSystem->setUpdate2410(false);
          xSemaphoreGive(this->mutexI2Ctask);
          this->ithoSystem->setSettingsHack();
        }
      }
      attachInterrupt(digitalPinToInterrupt(systemConfig->getI2C_SCL_Pin()), gpio_intr, RISING);
    }

    bool Itho::ithoInitCheck()
    {
      if (digitalRead(STATUSPIN) == LOW)
      {
        return false;
      }
      if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
      {
        this->ithoSystem->sendI2CPWMinit();
        xSemaphoreGive(this->mutexI2Ctask);
      }
      return false;
    }

    bool Itho::loadVirtualRemotesConfig()
    {

      DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 400));
      DeserializationError err = deserializeJson(doc, "{\"vremotes\":[{\"index\":0,\"id\":[0,0,0],\"name\":\"remote0\",\"remtype\":0,\"capabilities\":null}],\"remfunc\":0,\"version_of_program\":\"002\"}");

      doc.shrinkToFit();

      static uint8_t mac[6];
      esphome::get_mac_address_raw(mac);

      doc["vremotes"][0]["id"][0] = mac[3];
      doc["vremotes"][0]["id"][1] = mac[4];
      doc["vremotes"][0]["id"][2] = mac[5];
      doc["vremotes"][0]["remtype"] = RFTCVE;

      this->virtualRemotes.setConfigLoaded(this->virtualRemotes.set(doc.as<JsonObject>(), "vremotes"));
      return true;
    }

    bool Itho::ithoI2CCommand(uint8_t remoteIndex, const std::string &command)
    {
      ESP_LOGD(TAG, "EXEC VREMOTE BUTTON COMMAND: %s remote: %d", command.c_str(), remoteIndex);

      if (xSemaphoreTake(this->mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
      {
        if (strcmp(command.c_str(), "away") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoAway, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "low") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoLow, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "medium") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoMedium, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "high") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoHigh, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "timer1") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoTimer1, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "timer2") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoTimer2, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "timer3") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoTimer3, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "cook30") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoCook30, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "cook60") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoCook60, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "auto") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoAuto, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "autonight") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoAutoNight, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "join") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoJoin, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "leave") == 0)
        {
          this->ithoSystem->sendRemoteCmd(remoteIndex, IthoLeave, this->virtualRemotes);
        }
        else if (strcmp(command.c_str(), "type") == 0)
        {
          this->ithoSystem->sendQueryDevicetype();
        }
        else if (strcmp(command.c_str(), "status") == 0)
        {
          this->ithoSystem->sendQueryStatus();
        }
        else if (strcmp(command.c_str(), "statusformat") == 0)
        {
          this->ithoSystem->sendQueryStatusFormat();
        }
        else if (strcmp(command.c_str(), "31DA") == 0)
        {
          this->ithoSystem->sendQuery31DA();
        }
        else if (strcmp(command.c_str(), "31D9") == 0)
        {
          this->ithoSystem->sendQuery31D9();
        }
        else if (strcmp(command.c_str(), "10D0") == 0)
        {
          this->ithoSystem->filterReset();
        }
        else
        {
          xSemaphoreGive(this->mutexI2Ctask);
          return false;
        }
        xSemaphoreGive(this->mutexI2Ctask);
        return true;
      }
      else
      {
        return false;
      }
    }

    bool Itho::setIthoSpeed(uint16_t value)
    {

      if (value > 254)
        value = 254;
      if (this->ithoSystem->getIthoSpeed() != value)
      {

        ESP_LOGD(TAG, "setIthoSpeed: %d", value);
        this->ithoQueue->add2queue(value, 0, 0);
        return true;
      }
      return false;
    }

    void Itho::loop() {}

    void Itho::setup()
    {

      ESP_LOGD(TAG, "Setup Itho Core start");

      pinMode(WIFILED, OUTPUT);
      digitalWrite(WIFILED, HIGH);
      pinMode(STATUSPIN, INPUT_PULLUP);
      pinMode(ITHOSTATUS, OUTPUT);
      digitalWrite(ITHOSTATUS, LOW);

      mutexI2Ctask = xSemaphoreCreateMutex();

      static uint8_t mac[6];
      esphome::get_mac_address_raw(mac);

      uint8_t id[3];
      id[0] = mac[3];
      id[1] = mac[4];
      id[2] = mac[5];
      ESP_LOGD(TAG, "Setup: Virtual remote ID: %d,%d,%d", id[0], id[1], id[2]);

      ithoSystem = new IthoSystem(id, systemConfig);
      ithoQueue = new IthoQueue();

      scl_pin = systemConfig->getI2C_SCL_Pin();

      this->virtualRemotes.setMaxRemotes(1);
      this->loadVirtualRemotesConfig();
      this->IthoInit = true;

      xTaskCreatePinnedToCore(
          this->TaskSysControl,
          "TaskSysControl",
          STACK_SIZE,
          this,
          TASK_MAIN_PRIO,
          &xTaskSysControlHandle,
          CONFIG_ARDUINO_RUNNING_CORE);

      ESP_LOGD(TAG, "Setup Itho Core finished");
    }

    void Itho::dump_config()
    {
    }

  } // namespace itho
} // namespace esphome
