#include "itho.h"

#include "esphome/core/log.h"
#include "esphome.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/component.h"

namespace esphome
{
  namespace itho
  {

    static const char *const TAG = "itho";

    Itho::Itho() {}

    void Itho::execSystemControlTasks()
    {
      if (this->IthoInit && millis() > 250)
      {
        this->IthoInit = this->ithoInitCheck();
      }
      
      if (!this->i2cStartCommands && millis() > 15000 && (millis() - this->lastI2CinitRequest > 5000))
      {

        this->lastI2CinitRequest = millis();
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
            vTaskDelay(500 / portTICK_RATE_MS);
            this->ithoSystem->sendQueryStatus();
            vTaskDelay(500 / portTICK_RATE_MS);
        }
        else
        {
          this->ithoInitResult = -1;
        }
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

      if (millis() - this->query2401time >= 10000UL && this->i2cStartCommands)
      {
        this->query2401time = millis();
        this->ithoSystem->sendQueryStatus();
        vTaskDelay(500 / portTICK_RATE_MS);
        this->ithoSystem->sendQuery31DA();
        vTaskDelay(500 / portTICK_RATE_MS);
        this->ithoSystem->sendQuery31D9();
        vTaskDelay(500 / portTICK_RATE_MS);
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

      if (value > 254)
        value = 254;
      if (this->ithoSystem->getIthoSpeed() != value)
      {

        ESP_LOGD(TAG, "setIthoSpeed: %d", value);
          this->ithoSystem->sendRemoteCmd(0, IthoMedium, this->virtualRemotes);

          uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};
          uint8_t b = (uint8_t)value;

          command[6] = b;
          command[sizeof(command) - 1] = this->ithoSystem->checksum(command, sizeof(command) - 1);

          this->write_bytes_raw_callback(command, sizeof(command));
          return true;
      }
      return false;
    }

    void Itho::write_bytes_raw_callback(const uint8_t *buffer, uint32_t len) {
      this->write_bytes_raw((const char*)buffer, len);
    };

    void Itho::slave_receive_callback() {

      uint8_t i2cbuf[512]{};
      size_t len = this->slave_receive(i2cbuf);
      this->ithoSystem->setI2Cbuffer(i2cbuf, len);
    }

    void Itho::loop() {
      if (millis() > 3000) {
        execSystemControlTasks();
      }
    }

    void Itho::setup()
    {

      ESP_LOGD(TAG, "Setup Itho Core start");

      pinMode(WIFILED, OUTPUT);
      digitalWrite(WIFILED, HIGH);
      pinMode(STATUSPIN, INPUT_PULLUP);
      pinMode(ITHOSTATUS, OUTPUT);
      digitalWrite(ITHOSTATUS, LOW);

      systemConfig = new SystemConfig();
      systemConfig->setSysSHT30(syssht30);

      static uint8_t mac[6];
      esphome::get_mac_address_raw(mac);

      uint8_t id[3];
      id[0] = mac[3];
      id[1] = mac[4];
      id[2] = mac[5];
      ESP_LOGD(TAG, "Setup: Virtual remote ID: %d,%d,%d", id[0], id[1], id[2]);
      
      using std::placeholders::_1;
      using std::placeholders::_2;
      ithoSystem = new IthoSystem(
        id, 
        systemConfig, 
        std::bind(&Itho::write_bytes_raw_callback, this, _1, _2),
        std::bind(&Itho::slave_receive_callback, this)
      );
      
      this->virtualRemotes.setMaxRemotes(1);
      this->loadVirtualRemotesConfig();
      this->IthoInit = true;

      ESP_LOGD(TAG, "Setup Itho Core finished");
    }

    void Itho::dump_config()
    {
      LOG_I2C_DEVICE(this);
    }

  } // namespace itho
} // namespace esphome
