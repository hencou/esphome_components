#include "IthoSystem.h"

#include "esphome/core/log.h"
#include "esphome.h"

namespace esphome
{
  namespace itho
  {
    static const char *const TAG = "itho.system";
    
    IthoSystem::IthoSystem(
        uint8_t *id_,
        SystemConfig *systemConfig
        )
        : systemConfig(systemConfig) 
    {
      memcpy(id, id_, sizeof(id_[0]) * 3);
      ithoI2C = new IthoI2C(systemConfig);
      shtSensor = new SHTSensor(ithoI2C, systemConfig->getSysSHT30_Address());
    }

    const uint8_t *IthoSystem::getRemoteCmd(const RemoteTypes type, const IthoCommand command)
    {

      const struct ithoRemoteCmdMap *ithoRemoteCmdMapPtr = ithoRemoteCmdMapping;
      const struct ithoRemoteCmdMap *ithoRemoteCmdMapEndPtr = ithoRemoteCmdMapping + sizeof(ithoRemoteCmdMapping) / sizeof(ithoRemoteCmdMapping[0]);
      while (ithoRemoteCmdMapPtr < ithoRemoteCmdMapEndPtr)
      {
        if (ithoRemoteCmdMapPtr->type == type)
        {
          return *(ithoRemoteCmdMapPtr->cammandMapping + command);
        }
        ithoRemoteCmdMapPtr++;
      }
      return nullptr;
    }

    const char *IthoSystem::getIthoType(const uint8_t deviceID)
    {
      static char ithoDeviceType[32] = "Unkown device type";

      const struct ithoDeviceType *ithoDevicesptr = ithoDevices;
      const struct ithoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
      while (ithoDevicesptr < ithoDevicesendPtr)
      {
        if (ithoDevicesptr->ID == deviceID)
        {
          strcpy(ithoDeviceType, ithoDevicesptr->name);
        }
        ithoDevicesptr++;
      }
      return ithoDeviceType;
    }

    int IthoSystem::getSettingsLength(const uint8_t deviceID, const uint8_t version)
    {

      const struct ithoDeviceType *ithoDevicesptr = ithoDevices;
      const struct ithoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
      while (ithoDevicesptr < ithoDevicesendPtr)
      {
        if (ithoDevicesptr->ID == deviceID)
        {
          if (ithoDevicesptr->settingsMapping == nullptr)
          {
            return -2; // Settings not available for this device
          }
          if (version > (ithoDevicesptr->versionsMapLen - 1))
          {
            return -3; // Settings not available for this version
          }

          for (int i = 0; i < 999; i++)
          {
            if (static_cast<int>(*(*(ithoDevicesptr->settingsMapping + version) + i) == 999))
            {
              // end of array
              if (ithoSettingsArray == nullptr)
                ithoSettingsArray = new ithoSettings[i];
              return i;
            }
          }
        }
        ithoDevicesptr++;
      }
      return -1;
    }

    void IthoSystem::getSetting(const uint8_t i, const bool updateState, const bool loop)
    {
      getSetting(i, updateState, loop, ithoDeviceptr, ithoDeviceID, itho_fwversion);
    }

    void IthoSystem::getSetting(const uint8_t i, const bool updateState, const bool loop, const struct ithoDeviceType *settingsPtr, const uint8_t deviceID, const uint8_t version)
    {

      int settingsLen = getSettingsLength(deviceID, version);
      if (settingsLen < 0)
      {
        return;
      }

      index2410 = i;
      result2410[0] = 0;
      updated2410 = true;
    }

    int IthoSystem::getStatusLabelLength(const uint8_t deviceID, const uint8_t version)
    {

      const struct ithoDeviceType *ithoDevicesptr = ithoDevices;
      const struct ithoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
      while (ithoDevicesptr < ithoDevicesendPtr)
      {
        if (ithoDevicesptr->ID == deviceID)
        {
          if (ithoDevicesptr->statusLabelMapping == nullptr)
          {
            return -2; // Labels not available for this device
          }
          if (version > (ithoDevicesptr->statusMapLen - 1))
          {
            return -3; // Labels not available for this version
          }

          for (int i = 0; i < 255; i++)
          {
            if (static_cast<int>(*(*(ithoDevicesptr->statusLabelMapping + version) + i) == 255))
            {
              // end of array
              return i;
            }
          }
        }
        ithoDevicesptr++;
      }
      return -1;
    }

    const char *IthoSystem::getStatusLabel(const uint8_t i, const struct ithoDeviceType *statusPtr, const uint8_t version)
    {

      if (statusPtr == nullptr)
      {
        return ithoLabelErrors[0].labelFull;
      }
      else if (ithoStatusLabelLength == -2)
      {
        return ithoLabelErrors[1].labelFull;
      }
      else if (ithoStatusLabelLength == -3)
      {
        return ithoLabelErrors[2].labelFull;
      }
      else if (!(i < ithoStatusLabelLength))
      {
        return ithoLabelErrors[3].labelFull;
      }
      else
      {
        return (statusPtr->settingsStatusLabels[static_cast<int>(*(*(statusPtr->statusLabelMapping + version) + i))].labelFull);
      }
    }

    void IthoSystem::updateSetting(const uint8_t i, const int32_t value, bool webupdate)
    {

      index2410 = i;
      value2410 = value;
      update2410 = true;
    }

    void IthoSystem::getDevicePtr(const uint8_t deviceID)
    {

      const struct ithoDeviceType *ithoDevicesptr = ithoDevices;
      const struct ithoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
      while (ithoDevicesptr < ithoDevicesendPtr)
      {
        if (ithoDevicesptr->ID == deviceID)
        {
          ithoDeviceptr = ithoDevicesptr;
        }
        ithoDevicesptr++;
      }
    }

    uint8_t IthoSystem::checksum(const uint8_t *buf, size_t buflen)
    {
      uint8_t sum = 0;
      while (buflen--)
      {
        sum += *buf++;
      }
      return -sum;
    }

    void IthoSystem::sendI2CPWMinit()
    {

      uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0xB6};

      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);
      this->i2c_sendBytes(command, sizeof(command));
    }

    uint8_t cmdCounter = 0;

    void IthoSystem::sendRemoteCmd(const uint8_t remoteIndex, const IthoCommand command, IthoRemote &remotes)
    {

      // command structure:
      //  [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]<  opcode  ><len2><   len2 length command      >[chk2][cntr][chk ]
      //  0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
      if (remoteIndex > remotes.getMaxRemotes())
        return;

      const RemoteTypes remoteType = remotes.getRemoteType(remoteIndex);
      if (remoteType == RemoteTypes::UNSETTYPE)
        return;

      // Get the corresponding command / remote type combination
      const uint8_t *remote_command = getRemoteCmd(remoteType, command);
      if (remote_command == nullptr)
        return;

      uint8_t i2c_command[64] = {};
      uint8_t i2c_command_len = 0;

      /*
         First build the i2c header / wrapper for the remote command
      */
      //                 [I2C addr][I2C command][len][timestamp ][fmt ][remote ID][cntr]
      uint8_t i2c_header[] = {0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF};

      unsigned long curtime = millis();
      i2c_header[6] = (curtime >> 24) & 0xFF;
      i2c_header[7] = (curtime >> 16) & 0xFF;
      i2c_header[8] = (curtime >> 8) & 0xFF;
      i2c_header[9] = curtime & 0xFF;

      const int *id = remotes.getRemoteIDbyIndex(remoteIndex);
      i2c_header[11] = *id;
      i2c_header[12] = *(id + 1);
      i2c_header[13] = *(id + 2);

      i2c_header[14] = cmdCounter;
      cmdCounter++;

      for (; i2c_command_len < sizeof(i2c_header) / sizeof(i2c_header[0]); i2c_command_len++)
      {
        i2c_command[i2c_command_len] = i2c_header[i2c_command_len];
      }

      // determine command length
      const int command_len = remote_command[2];

      // copy to i2c_command
      for (int i = 0; i < 2 + command_len + 1; i++)
      {
        i2c_command[i2c_command_len] = remote_command[i];
        i2c_command_len++;
      }

      // if join or leave, add remote ID fields
      if (command == IthoJoin || command == IthoLeave)
      {
        // set command ID's
        if (command_len > 0x05)
        {
          // add 1st ID
          i2c_command[21] = *id;
          i2c_command[22] = *(id + 1);
          i2c_command[23] = *(id + 2);
        }
        if (command_len > 0x0B)
        {
          // add 2nd ID
          i2c_command[27] = *id;
          i2c_command[28] = *(id + 1);
          i2c_command[29] = *(id + 2);
        }
        if (command_len > 0x12)
        {
          // add 3rd ID
          i2c_command[33] = *id;
          i2c_command[34] = *(id + 1);
          i2c_command[35] = *(id + 2);
        }
        if (command_len > 0x17)
        {
          // add 4th ID
          i2c_command[39] = *id;
          i2c_command[40] = *(id + 1);
          i2c_command[41] = *(id + 2);
        }
        if (command_len > 0x1D)
        {
          // add 5th ID
          i2c_command[45] = *id;
          i2c_command[46] = *(id + 1);
          i2c_command[47] = *(id + 2);
        }
      }

      // if timer1-3 command use timer config of systemConfig.itho_timer1-3
      // example timer command: {0x22, 0xF3, 0x06, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00} 0xFF is timer value on pos 20 of i2c_command[]
      if (command == IthoTimer1)
      {
        i2c_command[20] = systemConfig->getIthoTimer1();
      }
      else if (command == IthoTimer2)
      {
        i2c_command[20] = systemConfig->getIthoTimer2();
      }
      else if (command == IthoTimer3)
      {
        i2c_command[20] = systemConfig->getIthoTimer3();
      }

      /*
         built the footer of the i2c wrapper
         chk2 = checksum of [fmt]+[remote ID]+[cntr]+[remote command]
         chk = checksum of the whole command

      */
      //                         [chk2][cntr][chk ]
      // uint8_t i2c_footer[] = { 0x00, 0x00, 0xFF };

      // calculate chk2 val
      uint8_t i2c_command_tmp[64] = {};
      for (int i = 10; i < i2c_command_len; i++)
      {
        i2c_command_tmp[(i - 10)] = i2c_command[i];
      }

      i2c_command[i2c_command_len] = checksum(i2c_command_tmp, i2c_command_len - 11);
      i2c_command_len++;

      //[cntr]
      i2c_command[i2c_command_len] = 0x00;
      i2c_command_len++;

      // set i2c_command length value
      i2c_command[5] = i2c_command_len - 6;

      // calculate chk val
      i2c_command[i2c_command_len] = checksum(i2c_command, i2c_command_len - 1);
      i2c_command_len++;

      this->i2c_sendBytes(i2c_command, sizeof(i2c_command));
    }

    void IthoSystem::sendQueryDevicetype()
    {

      uint8_t command[] = {0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
      {
        // ESP_LOGD(TAG, "I2C sendRemoteCmd response: %s", i2cbuf2string(i2cbuf, len).c_str());
        ithoDeviceID = i2cbuf[9];
        itho_fwversion = i2cbuf[11];
        getDevicePtr(ithoDeviceID);
        ithoSettingsLength = getSettingsLength(ithoDeviceID, itho_fwversion);
      }
    }

    void IthoSystem::sendQueryStatusFormat()
    {

      uint8_t command[] = {0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6};
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
      {
        // ESP_LOGD(TAG, "I2C sendQueryStatusFormat response: %s", i2cbuf2string(i2cbuf, len).c_str());
        if (!ithoStatus.empty())
        {
          ithoStatus.clear();
        }
        if (!(itho_fwversion > 0))
          return;
        ithoStatusLabelLength = getStatusLabelLength(ithoDeviceID, itho_fwversion);
        const uint8_t endPos = i2cbuf[5];
        for (uint8_t i = 0; i < endPos; i++)
        {
          ithoStatus.push_back(ithoDeviceStatus());
          ithoStatus.back().divider = 0;
          if ((i2cbuf[6 + i] & 0x07) == 0)
          { // integer value
            if ((i2cbuf[6 + i] & 0x80) == 0)
            { // unsigned value
              ithoStatus.back().type = ithoDeviceStatus::is_uint;
            }
            else
            {
              ithoStatus.back().type = ithoDeviceStatus::is_int;
            }
          }
          else
          {
            ithoStatus.back().type = ithoDeviceStatus::is_float;
            ithoStatus.back().divider = quick_pow10((i2cbuf[6 + i] & 0x07));
          }
          if (((i2cbuf[6 + i] >> 3) & 0x07) == 0)
          {
            ithoStatus.back().length = 1;
          }
          else
          {
            ithoStatus.back().length = (i2cbuf[6 + i] >> 3) & 0x07;
          }

          // special cases
          if (i2cbuf[6 + i] == 0x0C || i2cbuf[6 + i] == 0x6C)
          {
            ithoStatus.back().type = ithoDeviceStatus::is_byte;
            ithoStatus.back().length = 1;
          }
          if (i2cbuf[6 + i] == 0x0F)
          {
            ithoStatus.back().type = ithoDeviceStatus::is_float;
            ithoStatus.back().length = 1;
            ithoStatus.back().divider = 2;
          }
          if (i2cbuf[6 + i] == 0x5B)
          {
            ithoStatus.back().type = ithoDeviceStatus::is_uint;
            ithoStatus.back().length = 2;
          }
        }
      }
      // ESP_LOGD(TAG, "I2C ithoStatus - items:%d", ithoStatus.size());
    }

    void IthoSystem::sendQueryStatus()
    {

      uint8_t command[] = {0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5};
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
      {
        // ESP_LOGD(TAG, "I2C sendQueryStatus response: %s", i2cbuf2string(i2cbuf,len).c_str());
        int statusPos = 6; // first byte with status info
        int labelPos = 0;
        if (!ithoStatus.empty())
        {
          for (auto &ithoStat : ithoStatus)
          {
            ithoStat.name = getStatusLabel(labelPos, ithoDeviceptr, itho_fwversion);
            auto tempVal = 0;
            for (int i = ithoStat.length; i > 0; i--)
            {
              tempVal |= i2cbuf[statusPos + (ithoStat.length - i)] << ((i - 1) * 8);
            }

            if (ithoStat.type == ithoDeviceStatus::is_byte)
            {
              ithoStat.value.byteval = (byte)tempVal;
              ESP_LOGD(TAG, "I2C ithoStat.name: %s, byteval: %i", ithoStat.name, ithoStat.value.byteval);
            }
            if (ithoStat.type == ithoDeviceStatus::is_uint)
            {
              ithoStat.value.uintval = tempVal;
              ESP_LOGD(TAG, "I2C ithoStat.name: %s, intvalue: %i", ithoStat.name, ithoStat.value.intval);
            }
            if (ithoStat.type == ithoDeviceStatus::is_int)
            {
              if (ithoStat.length == 4)
              {
                tempVal = (int32_t)tempVal;
              }
              if (ithoStat.length == 2)
              {
                tempVal = (int16_t)tempVal;
              }
              if (ithoStat.length == 1)
              {
                tempVal = (int8_t)tempVal;
              }
              ithoStat.value.intval = tempVal;
            }
            if (ithoStat.type == ithoDeviceStatus::is_float)
            {
              ithoStat.value.floatval = static_cast<double>(tempVal) / ithoStat.divider;
              ESP_LOGD(TAG, "I2C ithoStat.name: %s, floatval: %02f", ithoStat.name, ithoStat.value.floatval);
            }

            statusPos += ithoStat.length;

            if (strcmp("Highest CO2 concentration (ppm)", ithoStat.name) == 0 || strcmp("highest-co2-concentration_ppm", ithoStat.name) == 0)
            {
              if (ithoStat.value.intval == 0x8200)
              {
                ithoStat.type = ithoDeviceStatus::is_string;
                ithoStat.value.stringval = fanSensorErrors.begin()->second;
              }
            }
            if (strcmp("Highest RH concentration (%)", ithoStat.name) == 0 || strcmp("highest-rh-concentration_perc", ithoStat.name) == 0)
            {
              if (ithoStat.value.intval == 0xEF)
              {
                ithoStat.type = ithoDeviceStatus::is_string;
                ithoStat.value.stringval = fanSensorErrors.begin()->second;
              }
            }
            if (strcmp("RelativeHumidity", ithoStat.name) == 0 || strcmp("relativehumidity", ithoStat.name) == 0)
            {
              // ESP_LOGD(TAG, "I2C RelativeHumidity response: %02f", ithoStat.value.floatval);
              if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0)
              {
                if (systemConfig->getSysSHT30() == 0)
                {
                  ithoHumidity = ithoStat.value.floatval;
                }
              }
              else
              {
                ithoStat.type = ithoDeviceStatus::is_string;
                ithoStat.value.stringval = fanSensorErrors.begin()->second;
              }
            }
            if (strcmp("Temperature", ithoStat.name) == 0 || strcmp("temperature", ithoStat.name) == 0)
            {
              // ESP_LOGD(TAG, "I2C Temperature response: %02f", ithoStat.value.floatval);
              if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0)
              {
                if (systemConfig->getSysSHT30() == 0)
                {
                  ithoTemperature = ithoStat.value.floatval;
                }
              }
              else
              {
                ithoStat.type = ithoDeviceStatus::is_string;
                ithoStat.value.stringval = fanSensorErrors.begin()->second;
              }
            }
            if (strcmp("Fan setpoint (rpm)", ithoStat.name) == 0)
            {
              ithoFanSetpoint = ithoStat.value.intval;
              ESP_LOGD(TAG, "ithoFanSetpoint: %02f", ithoFanSetpoint);
            }
            if (strcmp("Fan speed (rpm)", ithoStat.name) == 0)
            {
              ithoFanSpeed = ithoStat.value.intval;
              ESP_LOGD(TAG, "ithoFanSpeed: %02f", ithoFanSpeed);
            }
            labelPos++;
          }
        }
      }

      if (systemConfig->getSysSHT30() == 1) {
        ESP_LOGD(TAG, "Internal Itho sensor disabled, get temperature from SHT sensor...");
        shtSensor->readSample();
        ithoTemperature = shtSensor->getTemperature();
        ithoHumidity = shtSensor->getHumidity();
      }
      ESP_LOGD(TAG, "ithoTemperature: %02f", ithoTemperature);
      ESP_LOGD(TAG, "ithoHumidity: %02f", ithoHumidity);
    }

    void IthoSystem::sendQuery31DA()
    {

      uint8_t command[] = {0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF};
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
      {
        // ESP_LOGD(TAG, "I2C sendQuery31DA response: %s", i2cbuf2string(i2cbuf, len).c_str());
        auto dataLength = i2cbuf[5];

        auto dataStart = 6;
        if (!ithoMeasurements.empty())
        {
          ithoMeasurements.clear();
        }
        const int labelLen = 19;
        static const char *labels31DA[labelLen]{};
        for (int i = 0; i < labelLen; i++)
        {
          labels31DA[i] = itho31DALabels[i].labelFull;
        }
        if (dataLength > 0)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[0];
          if (i2cbuf[0 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors.find(i2cbuf[0 + dataStart]);
            if (it != fanSensorErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
            ithoMeasurements.back().value.intval = i2cbuf[0 + dataStart];
          }

          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[1];
          ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
          ithoMeasurements.back().value.intval = i2cbuf[1 + dataStart];
        }
        if (dataLength > 1)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[2];
          if (i2cbuf[2 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[2 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[2 + dataStart] << 8;
            tempVal |= i2cbuf[3 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
            ithoMeasurements.back().value.intval = tempVal;
          }
        }
        if (dataLength > 3)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[3];
          if (i2cbuf[4 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors.find(i2cbuf[4 + dataStart]);
            if (it != fanSensorErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
            ithoMeasurements.back().value.intval = i2cbuf[4 + dataStart];
          }
        }
        if (dataLength > 4)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[4];
          if (i2cbuf[5 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors.find(i2cbuf[5 + dataStart]);
            if (it != fanSensorErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = i2cbuf[5 + dataStart] / 2;
          }
        }
        if (dataLength > 5)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[5];
          if (i2cbuf[6 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[6 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[6 + dataStart] << 8;
            tempVal |= i2cbuf[7 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = tempVal / 100.0;
          }
        }
        if (dataLength > 7)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[6];
          if (i2cbuf[8 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[8 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[8 + dataStart] << 8;
            tempVal |= i2cbuf[9 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = tempVal / 100.0;
          }
        }
        if (dataLength > 9)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[7];
          if (i2cbuf[10 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[10 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[10 + dataStart] << 8;
            tempVal |= i2cbuf[11 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = tempVal / 100.0;
          }
        }
        if (dataLength > 11)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[8];
          if (i2cbuf[12 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[12 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[12 + dataStart] << 8;
            tempVal |= i2cbuf[13 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = tempVal / 100.0;
          }
        }
        if (dataLength > 13)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[9];
          ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
          int32_t tempVal = i2cbuf[14 + dataStart] << 8;
          tempVal |= i2cbuf[15 + dataStart];
          ithoMeasurements.back().value.intval = tempVal;
        }
        if (dataLength > 15)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[10];
          if (i2cbuf[16 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors.find(i2cbuf[16 + dataStart]);
            if (it != fanSensorErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = i2cbuf[16 + dataStart] / 2;
          }
        }
        if (dataLength > 16)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[11];
          ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
          auto it = fanInfo.find(i2cbuf[17 + dataStart] & 0x1F);
          if (it != fanInfo.end())
            ithoMeasurements.back().value.stringval = it->second;
          else
            ithoMeasurements.back().value.stringval = fanInfo.rbegin()->second;
        }
        if (dataLength > 17)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[12];
          if (i2cbuf[18 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors.find(i2cbuf[18 + dataStart]);
            if (it != fanSensorErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = i2cbuf[18 + dataStart] / 2;
          }
        }
        if (dataLength > 18)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[13];
          if (i2cbuf[19 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors.find(i2cbuf[19 + dataStart]);
            if (it != fanSensorErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = i2cbuf[19 + dataStart] / 2;
          }
        }
        if (dataLength > 19)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[14];
          int32_t tempVal = i2cbuf[20 + dataStart] << 8;
          tempVal |= i2cbuf[21 + dataStart];
          ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
          ithoMeasurements.back().value.intval = tempVal;
        }
        if (dataLength > 21)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[15];
          if (i2cbuf[22 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanHeatErrors.find(i2cbuf[22 + dataStart]);
            if (it != fanHeatErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = i2cbuf[22 + dataStart] / 2;
          }
        }
        if (dataLength > 22)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[16];
          if (i2cbuf[23 + dataStart] > 200)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanHeatErrors.find(i2cbuf[23 + dataStart]);
            if (it != fanHeatErrors.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
          }
          else
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = i2cbuf[23 + dataStart] / 2;
          }
        }
        if (dataLength > 23)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[17];
          if (i2cbuf[24 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[24 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[24 + dataStart] << 8;
            tempVal |= i2cbuf[25 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = tempVal / 100.0;
          }
        }
        if (dataLength > 25)
        {
          ithoMeasurements.push_back(ithoDeviceMeasurements());
          ithoMeasurements.back().name = labels31DA[18];

          if (i2cbuf[26 + dataStart] >= 0x7F)
          {
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
            auto it = fanSensorErrors2.find(i2cbuf[26 + dataStart]);
            if (it != fanSensorErrors2.end())
              ithoMeasurements.back().value.stringval = it->second;
            else
              ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
          }
          else
          {
            int32_t tempVal = i2cbuf[26 + dataStart] << 8;
            tempVal |= i2cbuf[27 + dataStart];
            ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
            ithoMeasurements.back().value.floatval = tempVal / 100.0;
          }
        }
      }

      // ESP_LOGD(TAG, "I2C ithoMeasurements: %i", ithoMeasurements.size());
      if (!ithoMeasurements.empty())
      {
        for (const auto &ithoMeasurement : ithoMeasurements)
        {
          if (ithoMeasurement.type == ithoDeviceMeasurements::is_string)
          {
            ESP_LOGD(TAG, "I2C ithoMeasurement.name: %s, stringval: %s", ithoMeasurement.name, ithoMeasurement.value.stringval);
          }
          if (ithoMeasurement.type == ithoDeviceMeasurements::is_int)
          {
            ESP_LOGD(TAG, "I2C ithoMeasurement.name: %s, intval: %i", ithoMeasurement.name, ithoMeasurement.value.intval);
          }
          if (ithoMeasurement.type == ithoDeviceMeasurements::is_float)
          {
            ESP_LOGD(TAG, "I2C ithoMeasurement.name: %s, floatval: %02f", ithoMeasurement.name, ithoMeasurement.value.floatval);
          }
          if (strcmp(ithoMeasurement.name, "FanInfo") == 0)
          {
            ithoFanInfo = ithoMeasurement.value.stringval;
            // ESP_LOGD(TAG, "Read ithoFanInfo: %s", ithoMeasurement.value.stringval);
          }
        }
      }
    }

    void IthoSystem::sendQuery31D9()
    {

      uint8_t command[] = {0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0};
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
      {
        // ESP_LOGD(TAG, "I2C sendQuery31D9 response: %s", i2cbuf2string(i2cbuf, len).c_str());
        auto dataStart = 6;

        if (!ithoInternalMeasurements.empty())
        {
          ithoInternalMeasurements.clear();
        }
        const int labelLen = 4;
        static const char *labels31D9[labelLen]{};
        for (int i = 0; i < labelLen; i++)
        {
          labels31D9[i] = itho31D9Labels[i].labelFull;
        }

        double tempVal = i2cbuf[1 + dataStart] / 2.0;
        ithoDeviceMeasurements sTemp = {labels31D9[0], ithoDeviceMeasurements::is_float, {.floatval = tempVal}};
        ithoInternalMeasurements.push_back(sTemp);

        int status = 0;
        if (i2cbuf[0 + dataStart] == 0x80)
        {
          status = 1; // fault
        }
        else
        {
          status = 0; // no fault
        }
        ithoInternalMeasurements.push_back({labels31D9[1], ithoDeviceMeasurements::is_int, {.intval = status}});
        if (i2cbuf[0 + dataStart] == 0x40)
        {
          status = 1; // frost cycle active
        }
        else
        {
          status = 0; // frost cycle not active
        }
        ithoInternalMeasurements.push_back({labels31D9[2], ithoDeviceMeasurements::is_int, {.intval = status}});
        if (i2cbuf[0 + dataStart] == 0x20)
        {
          status = 1; // filter dirty
        }
        else
        {
          status = 0; // filter clean
        }
        ithoInternalMeasurements.push_back({labels31D9[3], ithoDeviceMeasurements::is_int, {.intval = status}});
        //    if (i2cbuf[0 + dataStart] == 0x10) {
        //      //unknown
        //    }
        //    if (i2cbuf[0 + dataStart] == 0x08) {
        //      //unknown
        //    }
        //    if (i2cbuf[0 + dataStart] == 0x04) {
        //      //unknown
        //    }
        //    if (i2cbuf[0 + dataStart] == 0x02) {
        //      //unknown
        //    }
        //    if (i2cbuf[0 + dataStart] == 0x01) {
        //      //unknown
        //    }
      }

      // ESP_LOGD(TAG, "I2C ithoInternalMeasurements: %i", ithoInternalMeasurements.size());
      if (!ithoInternalMeasurements.empty())
      {
        for (const auto &internalMeasurement : ithoInternalMeasurements)
        {
          if (internalMeasurement.type == ithoDeviceMeasurements::is_string)
          {
            ESP_LOGD(TAG, "I2C internalMeasurement.name: %s, stringval: %s", internalMeasurement.name, internalMeasurement.value.stringval);
          }
          if (internalMeasurement.type == ithoDeviceMeasurements::is_int)
          {
            ESP_LOGD(TAG, "I2C internalMeasurement.name: %s, intval: %i", internalMeasurement.name, internalMeasurement.value.intval);
          }
          if (internalMeasurement.type == ithoDeviceMeasurements::is_float)
          {
            ESP_LOGD(TAG, "I2C iinternalMeasurement.name: %s, floatval: %02f", internalMeasurement.name, internalMeasurement.value.floatval);
          }
          if (strcmp(internalMeasurement.name, "Speed status") == 0)
          {
            ithoSpeed = static_cast<int>(internalMeasurement.value.floatval * 2.55);
            // ESP_LOGD(TAG, "Read ithoSpeed: %02f", internalMeasurement.value.floatval);
          }
        }
      }
    }

    void IthoSystem::sendQuery2410()
    {

      result2410[0] = 0;
      result2410[1] = 0;
      result2410[2] = 0;

      uint8_t command[] = {0x82, 0x80, 0x24, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF};

      command[23] = index2410;
      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
      {
        //ESP_LOGD(TAG, "I2C sendQuery2410 response: %s", i2cbuf2string(i2cbuf, len).c_str());
        uint8_t tempBuf[] = {i2cbuf[9], i2cbuf[8], i2cbuf[7], i2cbuf[6]};
        std::memcpy(&result2410[0], tempBuf, 4);

        uint8_t tempBuf2[] = {i2cbuf[13], i2cbuf[12], i2cbuf[11], i2cbuf[10]};
        std::memcpy(&result2410[1], tempBuf2, 4);

        uint8_t tempBuf3[] = {i2cbuf[17], i2cbuf[16], i2cbuf[15], i2cbuf[14]};
        std::memcpy(&result2410[2], tempBuf3, 4);

        ithoSettingsArray[index2410].value = result2410[0];

        if (((i2cbuf[22] >> 3) & 0x07) == 0)
        {
          ithoSettingsArray[index2410].length = 1;
        }
        else
        {
          ithoSettingsArray[index2410].length = (i2cbuf[22] >> 3) & 0x07;
        }

        if ((i2cbuf[22] & 0x07) == 0)
        { // integer value
          if ((i2cbuf[22] & 0x80) == 0)
          { // unsigned value
            if (ithoSettingsArray[index2410].length == 1)
            {
              ithoSettingsArray[index2410].type = ithoSettings::is_uint8;
            }
            else if (ithoSettingsArray[index2410].length == 2)
            {
              ithoSettingsArray[index2410].type = ithoSettings::is_uint16;
            }
            else
            {
              ithoSettingsArray[index2410].type = ithoSettings::is_uint32;
            }
          }
          else
          {
            if (ithoSettingsArray[index2410].length == 1)
            {
              ithoSettingsArray[index2410].type = ithoSettings::is_int8;
            }
            else if (ithoSettingsArray[index2410].length == 2)
            {
              ithoSettingsArray[index2410].type = ithoSettings::is_int16;
            }
            else
            {
              ithoSettingsArray[index2410].type = ithoSettings::is_int32;
            }
          }
        }
        else
        { // float

          if ((i2cbuf[22] & 0x04) != 0)
          {
            ithoSettingsArray[index2410].type = ithoSettings::is_float1000;
          }
          else if ((i2cbuf[22] & 0x02) != 0)
          {
            ithoSettingsArray[index2410].type = ithoSettings::is_float100;
          }
          else if ((i2cbuf[22] & 0x01) != 0)
          {
            ithoSettingsArray[index2410].type = ithoSettings::is_float10;
          }
          else
          {
            ithoSettingsArray[index2410].type = ithoSettings::is_unknown;
          }
        }
        // special cases
        if (i2cbuf[22] == 0x01)
        {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint8;
        }
      }
    }

    void IthoSystem::setSetting2410()
    {

      if (index2410 == 7 && value2410 == 1 && (ithoDeviceID == 0x14 || ithoDeviceID == 0x1B || ithoDeviceID == 0x1D))
      {
        return;
      }

      if (ithoSettingsArray == nullptr)
        return;

      uint8_t command[] = {0x82, 0x80, 0x24, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF};

      command[23] = index2410;

      if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_int8)
      {
        command[9] = value2410 & 0xFF;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_int16)
      {
        command[9] = value2410 & 0xFF;
        command[8] = (value2410 >> 8) & 0xFF;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint32 || ithoSettingsArray[index2410].type == ithoSettings::is_int32 || ithoSettingsArray[index2410].type == ithoSettings::is_float2 || ithoSettingsArray[index2410].type == ithoSettings::is_float10 || ithoSettingsArray[index2410].type == ithoSettings::is_float100 || ithoSettingsArray[index2410].type == ithoSettings::is_float1000)
      {
        command[9] = value2410 & 0xFF;
        command[8] = (value2410 >> 8) & 0xFF;
        command[7] = (value2410 >> 16) & 0xFF;
        command[6] = (value2410 >> 24) & 0xFF;
      }

      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);
      this->i2c_sendBytes(command, sizeof(command));

      uint8_t i2cbuf[512]{};
      uint8_t len = ithoI2C->i2c_slave_receive(i2cbuf);
      //ESP_LOGD(TAG, "I2C setSetting2410 response: %s", i2cbuf2string(i2cbuf, len).c_str());
    }

    void IthoSystem::filterReset()
    {

      //[I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr][cmd opcode][len ][  command ][  counter ][chk]
      uint8_t command[] = {0x82, 0x62, 0xC1, 0x01, 0x01, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0xD0, 0x02, 0x63, 0xFF, 0x00, 0x00, 0xFF};

      unsigned long curtime = millis();

      command[6] = (curtime >> 24) & 0xFF;
      command[7] = (curtime >> 16) & 0xFF;
      command[8] = (curtime >> 8) & 0xFF;
      command[9] = curtime & 0xFF;

      command[11] = id[0];
      command[12] = id[1];
      command[13] = id[2];

      command[14] = cmdCounter;
      cmdCounter++;

      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

      this->i2c_sendBytes(command, sizeof(command));
    }

    int IthoSystem::quick_pow10(int n)
    {
      static int pow10[10] = {
          1, 10, 100, 1000, 10000,
          100000, 1000000, 10000000, 100000000, 1000000000};
      if (n > (sizeof(pow10) / sizeof(pow10[0]) - 1))
        return 1;
      return pow10[n];
    }

    char IthoSystem::toHex(uint8_t c)
    {
      return c < 10 ? c + '0' : c + 'A' - 10;
    }

    std::string IthoSystem::i2cbuf2string(const uint8_t *data, size_t len)
    {
      std::string s;
      s.reserve(len * 3 + 2);
      for (size_t i = 0; i < len; ++i)
      {
        if (i)
          s += ' ';
        s += toHex(data[i] >> 4);
        s += toHex(data[i] & 0xF);
      }
      return s;
    }

    void IthoSystem::getID(uint8_t *id_)
    {
      memcpy(id_, id, sizeof(id[0]) * 3);
    }

    void IthoSystem::setSettingsHack()
    {
      getSettingsHack.once_ms(
          1, +[](IthoSystem *ithoSystem)
             { ithoSystem->getSetting(ithoSystem->getIndex2410(), true, false); },
          this);
    }

    bool IthoSystem::i2c_sendBytes(const uint8_t *buf, size_t len)
    {

      if (len)
      {
        esp_err_t rc = ithoI2C->i2c_master_send((const char *)buf, len);
        if (rc)
        {
          return false;
        }
        return true;
      }
      return false;
    }

  }
}
