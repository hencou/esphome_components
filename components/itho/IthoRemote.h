#pragma once

#include "esphome/components/json/json_util.h"

namespace esphome
{
  namespace itho
  {

#define MAX_NUMBER_OF_REMOTES 12
#define REMOTE_CONFIG_VERSION "002"

    enum RemoteFunctions : uint8_t
    {
      UNSETFUNC,
      RECEIVE,
      SEND
    };

    enum RemoteTypes : uint16_t
    {
      UNSETTYPE = 0x0,
      NORMAL = 0x1,
      MONITOR = 0x2,
      RFTCVE = 0x22F1,
      RFTAUTO = 0x22F3,
      DEMANDFLOW = 0x22F8,
      RFTRV = 0x12A0,
      RFTCO2 = 0x1298
    };

    class IthoRemote
    {
    public:
      IthoRemote();
      ~IthoRemote();

      int getRemoteCount();

      bool toggleLearnLeaveMode();
      bool remoteLearnLeaveStatus() {return llMode;}

      void updatellModeTimer();
      uint8_t getllModeTime() {return llModeTime;}

      void setllModeTime(const int timeVal) {llModeTime = timeVal;}

      int setMaxRemotes(unsigned int number) { return (maxRemotes < MAX_NUMBER_OF_REMOTES + 1) ? (maxRemotes = number) : (maxRemotes = MAX_NUMBER_OF_REMOTES); }
      int getMaxRemotes() { return maxRemotes; }
      int registerNewRemote(const int *id);
      int removeRemote(const int *id);
      int removeRemote(const uint8_t index, const char *type);
      void addCapabilities(const uint8_t remoteIndex, const char *name, int32_t value);
      void updateRemoteName(const uint8_t index, const char *remoteName);
      void updateRemoteType(const uint8_t index, const uint16_t type);
      int remoteIndex(const int32_t id);
      int remoteIndex(const int *id);
      const int *getRemoteIDbyIndex(const int index);
      const char *getRemoteNamebyIndex(const int index);
      int getRemoteIndexbyName(const char *name);
      RemoteTypes getRemoteType(const int index) { return remotes[index].remtype; };
      bool checkID(const int *id);
      bool set(JsonObjectConst, const char *root);
      void get(JsonObject obj, const char *root) const;
      void getCapabilities(JsonObject obj) const;
      void setConfigLoaded(bool value) {configLoaded = value;}
    
    private:

      int activeRemote{-1};
      const char *lastRemoteName;
      bool configLoaded;
      char config_struct_version[4];

      struct Remote
      {
        mutable uint8_t ID[3]{0, 0, 0};
        char name[32];
        mutable RemoteTypes remtype{RemoteTypes::UNSETTYPE};
        JsonDocument capabilities;
        void set(JsonObjectConst);
        void get(JsonObject, const char *root, int index) const;
      };

      mutable RemoteFunctions remfunc{RemoteFunctions::UNSETFUNC};
      Remote remotes[MAX_NUMBER_OF_REMOTES];
      int remoteCount{0};
      int maxRemotes{10};
      mutable bool llMode = false;
      volatile uint8_t llModeTime{0};

    };

  }
}
