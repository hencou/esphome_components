#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"

#include "lib/Radio/PL1167_nRF24.h"
#include "lib/Radio/NRF24MiLightRadio.h"
#include "lib/Radio/LT8900MiLightRadio.h"
#include "lib/Radio/RadioUtils.h"
#include "lib/Radio/MiLightRadioFactory.h"
#include "lib/Radio/MiLightRadio.h"
#include "lib/Radio/MiLightRadioConfig.h"
#include "lib/Types/BulbId.h"
#include "lib/Types/GroupStateField.h"
#include "lib/Types/RF24PowerLevel.h"
#include "lib/Types/MiLightCommands.h"
#include "lib/Types/RF24Channel.h"
#include "lib/Types/MiLightStatus.h"
#include "lib/Types/ParsedColor.h"
#include "lib/Types/MiLightRemoteType.h"
#include "lib/DataStructures/LinkedList_espMH.h"
#include "lib/Helpers/Units.h"
#include "lib/Helpers/Size.h"
#include "lib/Helpers/IntParsing.h"
#include "lib/Helpers/JsonHelpers.h"
#include "lib/MiLightState/GroupState.h"
#include "lib/MiLightState/GroupStateStore.h"
#include "lib/MiLightState/GroupStateCache.h"
#include "lib/MiLight/MiLightClient.h"
#include "lib/MiLight/RgbPacketFormatter.h"
#include "lib/MiLight/CctPacketFormatter.h"
#include "lib/MiLight/PacketQueue.h"
#include "lib/MiLight/V2PacketFormatter.h"
#include "lib/MiLight/RadioSwitchboard.h"
#include "lib/MiLight/V2RFEncoding.h"
#include "lib/MiLight/PacketSender.h"
#include "lib/MiLight/RgbwPacketFormatter.h"
#include "lib/MiLight/RgbCctPacketFormatter.h"
#include "lib/MiLight/FUT02xPacketFormatter.h"
#include "lib/MiLight/FUT089PacketFormatter.h"
#include "lib/MiLight/FUT091PacketFormatter.h"
#include "lib/MiLight/PacketFormatter.h"
#include "lib/MiLight/MiLightRemoteConfig.h"
#include "lib/MiLight/FUT020PacketFormatter.h"
#include "lib/Settings/Settings.h"

#include "ListLib.h"

namespace esphome {
  namespace mi {
 
    struct Request {
      char request[200];
    };

    struct MiOutput {
      uint32_t key;
      BulbId bulbId;
    };
    
    class Mi : public Component {
      public:

        Mi();
        ~Mi();

        void handleCommand(BulbId bulbId, String command);

        void setup() override;
        void loop() override;
        void dump_config() override;
        void write_state(BulbId bulbId, light::LightState *state);
        
        void add_child(uint32_t objectId, BulbId bulbId) {miOutputs.push_back({objectId, bulbId});}

        void set_ce_pin(InternalGPIOPin *pin) {settings.cePin = pin->get_pin();}
        void set_csn_pin(InternalGPIOPin *pin) {settings.csnPin = pin->get_pin();}
        void set_reset_pin(InternalGPIOPin *pin) {settings.resetPin = pin->get_pin();}
        void set_radio_interface_type(String value) {settings.radioInterfaceType = Settings::typeFromString(value);}
        void set_packet_repeats(size_t value) {settings.packetRepeats = value;}
        void set_listen_repeats(uint8_t value) {settings.listenRepeats = value;}
        void set_state_flush_interval(size_t value) {settings.stateFlushInterval = value;}
        void set_packet_repeat_throttle_threshold(size_t value) {settings.packetRepeatThrottleThreshold = value;}
        void set_packet_repeat_throttle_sensitivity(size_t value) {settings.packetRepeatThrottleSensitivity = value;}
        void set_packet_repeat_minimum(size_t value) {settings.packetRepeatMinimum = value;}
        void set_enable_automatic_mode_switching(bool value) {settings.enableAutomaticModeSwitching = value;}
        void set_rf24_power_level(String value) {settings.rf24PowerLevel = RF24PowerLevelHelpers::valueFromName(value);}
        void del_rf24_channels() {settings.rf24Channels.clear();}
        void add_rf24_channel(String value) {settings.rf24Channels.push_back(RF24ChannelHelpers::valueFromName(value));}
        void set_rf24_listen_channel(String value) {settings.rf24ListenChannel = RF24ChannelHelpers::valueFromName(value);}
        void set_packet_repeats_per_loop(size_t value) {settings.packetRepeatsPerLoop = value;}
      private:

        Settings settings;

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
        bool writeState = false;

        void onPacketSentHandler(uint8_t* packet, const MiLightRemoteConfig& config);
        void onPacketReceivedHandler(uint8_t* packet, const MiLightRemoteConfig& config);
        void handleListen();

        void applySettings();

        List<uint32_t> bulbCompactIds;
        List<Request> requests;
        std::vector<MiOutput> miOutputs;
        int i = 0;

        void updateOutput(light::LightState *state, JsonObject result);
    };

  }  // namespace mi
}  // namespace esphome
