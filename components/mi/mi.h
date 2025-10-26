#pragma once

#include <utility>
#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/hal.h"
#include "esphome/core/defines.h"
#include <esp_timer.h>

#include "PL1167_nRF24.h"
#include "NRF24MiLightRadio.h"
//#include "LT8900MiLightRadio.h"
#include "RadioUtils.h"
#include "MiLightRadioFactory.h"
#include "MiLightRadio.h"
#include "MiLightRadioConfig.h"
#include "BulbId.h"
#include "GroupStateField.h"
#include "RF24PowerLevel.h"
#include "MiLightCommands.h"
#include "RF24Channel.h"
#include "MiLightStatus.h"
#include "ParsedColor.h"
#include "MiLightRemoteType.h"
#include "LinkedList_espMH.h"
#include "Units.h"
#include "Size.h"
#include "IntParsing.h"
#include "JsonHelpers.h"
#include "GroupState.h"
#include "GroupStateStore.h"
#include "GroupStateCache.h"
#include "MiLightClient.h"
#include "RgbPacketFormatter.h"
#include "CctPacketFormatter.h"
#include "PacketQueue.h"
#include "V2PacketFormatter.h"
#include "RadioSwitchboard.h"
#include "V2RFEncoding.h"
#include "PacketSender.h"
#include "RgbwPacketFormatter.h"
#include "RgbCctPacketFormatter.h"
#include "FUT02xPacketFormatter.h"
#include "FUT089PacketFormatter.h"
#include "FUT091PacketFormatter.h"
#include "S2PacketFormatter.h"
#include "PacketFormatter.h"
#include "MiLightRemoteConfig.h"
#include "FUT020PacketFormatter.h"
#include "Settings.h"
#include "MiHelpers.h"

#include "ListLib.h"

namespace esphome {
  namespace mi {
    
    struct MiBridgeData {
      uint16_t device_id;
      uint8_t group_id;
      std::string remote_type;
      std::string command;
    };

    struct Request {
      char request[200];
    };

    struct MiOutput {
      uint32_t key;
      uint32_t deviceId;
      BulbId bulbId;
    };

    class Mi : public Component {
      public:

        Mi();
        ~Mi();

        void handleCommand(BulbId bulbId, std::string command);

        void setup() override;
        void loop() override;
        void dump_config() override;
        void write_state(BulbId bulbId, light::LightState *state);
        void write_state(BulbId bulbId, std::string command);
        void add_on_command_received_callback(std::function<void(MiBridgeData)> callback) {
          this->data_callback_.add(std::move(callback));
        }
        
        void add_child(uint32_t objectId, uint32_t deviceId, BulbId bulbId) {miOutputs.push_back({objectId, deviceId, bulbId});}

        void set_ce_pin(InternalGPIOPin *pin) {settings.cePin = pin->get_pin();}
        void set_csn_pin(InternalGPIOPin *pin) {settings.csnPin = pin->get_pin();}
        void set_reset_pin(InternalGPIOPin *pin) {settings.resetPin = pin->get_pin();}
        void set_radio_interface_type(std::string value) {settings.radioInterfaceType = Settings::typeFromString(value);}
        void set_packet_repeats(size_t value) {settings.packetRepeats = value;}
        void set_listen_repeats(uint8_t value) {settings.listenRepeats = value;}
        void set_state_flush_interval(size_t value) {settings.stateFlushInterval = value;}
        void set_packet_repeat_throttle_threshold(size_t value) {settings.packetRepeatThrottleThreshold = value;}
        void set_packet_repeat_throttle_sensitivity(size_t value) {settings.packetRepeatThrottleSensitivity = value;}
        void set_packet_repeat_minimum(size_t value) {settings.packetRepeatMinimum = value;}
        void set_enable_automatic_mode_switching(bool value) {settings.enableAutomaticModeSwitching = value;}
        void set_rf24_power_level(std::string value) {settings.rf24PowerLevel = RF24PowerLevelHelpers::valueFromName(value);}
        void del_rf24_channels() {settings.rf24Channels.clear();}
        void add_rf24_channel(std::string value) {settings.rf24Channels.push_back(RF24ChannelHelpers::valueFromName(value));}
        void set_rf24_listen_channel(std::string value) {settings.rf24ListenChannel = RF24ChannelHelpers::valueFromName(value);}
        void set_packet_repeats_per_loop(size_t value) {settings.packetRepeatsPerLoop = value;}
        void set_resend_last_command(bool value) {settings.resendLastCommand = value;}
      private:

        Settings settings;
        MiHelpers miHelpers;

        MiLightClient* milightClient = NULL;
        RadioSwitchboard* radios = nullptr;
        PacketSender* packetSender = nullptr;
        std::shared_ptr<MiLightRadioFactory> radioFactory;
        uint8_t currentRadioType = 0;

        // For tracking and managing group state
        GroupStateStore* stateStore = NULL;
        GroupState* groupState = NULL;
        
        unsigned long lastRequestTime;
        unsigned int repeatTimer = 0;
        
        int hue = 0;
        int saturation = 100;
        bool writingState = false;

        void writeState(BulbId bulbId, JsonObject requestJson);
        void onPacketSentHandler(uint8_t* packet, const MiLightRemoteConfig& config);
        void onPacketReceivedHandler(uint8_t* packet, const MiLightRemoteConfig& config);
        void updateState(BulbId bulbId, JsonObject requestJson, bool local);
        void handleListen();

        void applySettings();

        List<uint32_t> bulbCompactIds;
        List<Request> requests;
        std::vector<MiOutput> miOutputs;
        int i = 0;

        void updateOutput(light::LightState *state, JsonObject result);
        
        CallbackManager<void(MiBridgeData)> data_callback_;
    };
    
    class MiBridgeReceivedCodeTrigger : public Trigger<MiBridgeData> {
      public:
        explicit MiBridgeReceivedCodeTrigger(Mi *parent) {
          parent->add_on_command_received_callback([this](MiBridgeData data) { this->trigger(data); });
        }
    };

  }  // namespace mi
}  // namespace esphome
