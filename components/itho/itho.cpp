#include "itho.h"

#include "esphome/core/log.h"
#include "esphome.h"
#include "esphome/core/component.h"

namespace esphome
{
  namespace itho
  {

    static const char *const TAG = "itho";

    Itho::Itho()
    {
      systemConfig = new SystemConfig();
    }

    //// Detect rising edge of I2C SCL pin
    uint8_t scl_pin;
    bool lowSCL = false;
    void IRAM_ATTR gpio_intr()
    {
      lowSCL = true;
      detachInterrupt(digitalPinToInterrupt(scl_pin));
    }
    //// End detect rising edge of I2C SCL pin

    void Itho::execSystemControlTasks()
    {
      //// Only run once after a 200 msec of inactivity on the I2C bus. Itho queries the bus every 8 seconds
      if (lowSCL == true)
      {
        lowSCL = false;
        ESP_LOGD(TAG, "lowSCL triggered");
        this->lastSCLLowTime = millis();
      }
      if (millis() - lastSCLLowTime < 200 || digitalRead(systemConfig->getI2C_SCL_Pin()) == LOW)
      {
        loopSystemControlTasks = true;
        return;
      }
      //// End "only run"

      if (this->IthoInit && millis() > 250)
      {
        this->IthoInit = this->ithoInitCheck();
      }

      if (!this->i2cStartCommands && millis() > 15000UL && (millis() - this->lastI2CinitRequest > 5000))
      {
        this->ithoSystem->sendQueryDevicetype();

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
            }
            if (this->systemConfig->getSysSHT30() == 2)
            {
              this->systemConfig->setSysSHT30(0);
            }
          }
          this->ithoSystem->sendQueryStatusFormat();
          this->ithoSystem->sendQueryStatus();
        }
        else
        {
          this->ithoInitResult = -1;
        }
      }

      if (this->ithoQueue->getIthoSpeedUpdated())
      {
        uint16_t speed = this->ithoQueue->getIthoSpeed();

        // //ESP_LOGD(TAG, "Set FanInfo on auto...");
        this->ithoSystem->sendRemoteCmd(0, IthoMedium, this->virtualRemotes);

        vTaskDelay(1000 / portTICK_RATE_MS);

        uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};
        uint8_t b = (uint8_t)speed;

        command[6] = b;
        command[sizeof(command) - 1] = this->ithoSystem->checksum(command, sizeof(command) - 1);

        this->ithoSystem->i2c_sendBytes(command, sizeof(command));
        this->ithoQueue->setIthoSpeedUpdated(false);
      }

      if (!this->joinSend && this->ithoInitResult == 1)
      {
        this->joinSend = true;
        this->ithoSystem->sendRemoteCmd(0, IthoJoin, virtualRemotes);
        uint8_t id[3];
        this->ithoSystem->getID(id);
        ESP_LOGD(TAG, "I2C init: Virtual remote join command send with id: %d, %d, %d", id[0], id[1], id[2]);
      }

      if (!(this->ithoSystem->getIthoFwVersion() > 0) && millis() - lastVersionCheck > 60000)
      {
        lastVersionCheck = millis();
        this->ithoSystem->sendQueryDevicetype();
      }

      if (!loopSystemControlTasks && millis() - loopSystemControlTasksTime < 10000UL)
      {
        return;
      }
      loopSystemControlTasks = false;
      loopSystemControlTasksTime = millis();

      if (this->i2cStartCommands)
      {
        this->ithoSystem->sendQueryStatus();
        this->ithoSystem->sendQuery31DA();
        this->ithoSystem->sendQuery31D9();
      }

      if (this->ithoSystem->getUpdated2410())
      {
        this->ithoSystem->setUpdated2410(false);
        this->ithoSystem->sendQuery2410();
      }

      if (this->ithoSystem->getUpdate2410())
      {
        this->ithoSystem->setSetting2410();
        this->ithoSystem->setUpdate2410(false);
        this->ithoSystem->setSettingsHack();
      }
      attachInterrupt(digitalPinToInterrupt(systemConfig->getI2C_SCL_Pin()), gpio_intr, RISING);
      lowSCL = false;
    }

    bool Itho::ithoInitCheck()
    {
      if (digitalRead(STATUSPIN) == LOW)
      {
        return false;
      }
      this->ithoSystem->sendI2CPWMinit();
      return false;
    }

    bool Itho::loadVirtualRemotesConfig()
    {

      JsonDocument doc;
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
        return false;
      }
      return true;
    }

    bool Itho::setIthoSpeed(uint16_t value)
    {
      value += 13; //Correct diff between input and output values
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

    void Itho::loop()
    {
      this->execSystemControlTasks();
    }

    void Itho::setup()
    {

      ESP_LOGD(TAG, "Setup Itho Core start");

      pinMode(STATUSPIN, INPUT_PULLUP);
      pinMode(ITHOSTATUS, OUTPUT);
      digitalWrite(ITHOSTATUS, LOW);

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
      pinMode(scl_pin, INPUT);

      this->virtualRemotes.setMaxRemotes(1);
      this->loadVirtualRemotesConfig();
      this->IthoInit = true;

      ESP_LOGD(TAG, "Setup Itho Core finished");
    }

    void Itho::dump_config()
    {
    }

  } // namespace itho
} // namespace esphome
