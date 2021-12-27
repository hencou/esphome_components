#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"

#include "milight/Radio/PL1167_nRF24.h"
#include "milight/Radio/NRF24MiLightRadio.h"
#include "milight/Radio/LT8900MiLightRadio.h"
#include "milight/Radio/RadioUtils.h"
#include "milight/Radio/MiLightRadioFactory.h"
#include "milight/Radio/MiLightRadio.h"
#include "milight/Radio/MiLightRadioConfig.h"
#include "milight/Types/BulbId.h"
#include "milight/Types/GroupStateField.h"
#include "milight/Types/RF24PowerLevel.h"
#include "milight/Types/MiLightCommands.h"
#include "milight/Types/RF24Channel.h"
#include "milight/Types/MiLightStatus.h"
#include "milight/Types/ParsedColor.h"
#include "milight/Types/MiLightRemoteType.h"
#include "milight/DataStructures/LinkedList_espMH.h"
#include "milight/Helpers/Units.h"
#include "milight/Helpers/Size.h"
#include "milight/Helpers/IntParsing.h"
#include "milight/Helpers/JsonHelpers.h"
#include "milight/Transitions/Transition.h"
#include "milight/Transitions/TransitionController.h"
#include "milight/Transitions/ColorTransition.h"
#include "milight/Transitions/ChangeFieldOnFinishTransition.h"
#include "milight/Transitions/FieldTransition.h"
#include "milight/MiLightState/GroupState.h"
#include "milight/MiLightState/GroupStateStore.h"
#include "milight/MiLightState/GroupStateCache.h"
#include "milight/MiLight/MiLightClient.h"
#include "milight/MiLight/RgbPacketFormatter.h"
#include "milight/MiLight/CctPacketFormatter.h"
#include "milight/MiLight/PacketQueue.h"
#include "milight/MiLight/V2PacketFormatter.h"
#include "milight/MiLight/RadioSwitchboard.h"
#include "milight/MiLight/V2RFEncoding.h"
#include "milight/MiLight/PacketSender.h"
#include "milight/MiLight/RgbwPacketFormatter.h"
#include "milight/MiLight/RgbCctPacketFormatter.h"
#include "milight/MiLight/FUT02xPacketFormatter.h"
#include "milight/MiLight/FUT089PacketFormatter.h"
#include "milight/MiLight/FUT091PacketFormatter.h"
#include "milight/MiLight/PacketFormatter.h"
#include "milight/MiLight/MiLightRemoteConfig.h"
#include "milight/MiLight/FUT020PacketFormatter.h"
#include "milight/Settings/Settings.h"
#include "milight/Settings/AboutHelper.h"
#include "milight/Settings/StringStream.h"

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

        void handleCommand(BulbId bulbId, String command);

        void setup() override;
        void loop() override;
        void dump_config() override;
        void write_state(BulbId bulbId, light::LightState *state);
        
        void add_child(uint32_t objectId, BulbId bulbId) {miOutputs.push_back({objectId, bulbId});}

        void set_ce_pin(InternalGPIOPin *ce_pin) {ce_pin_ = ce_pin;}
        void set_csn_pin(InternalGPIOPin *csn_pin) {csn_pin_ = csn_pin;}
        void set_reset_pin(InternalGPIOPin *reset_pin) {reset_pin_ = reset_pin;}
      private:

        Settings settings;
        InternalGPIOPin *ce_pin_;
        InternalGPIOPin *csn_pin_;
        InternalGPIOPin *reset_pin_;

        MiLightClient* milightClient = NULL;
        RadioSwitchboard* radios = nullptr;
        PacketSender* packetSender = nullptr;
        std::shared_ptr<MiLightRadioFactory> radioFactory;
        uint8_t currentRadioType = 0;

        // For tracking and managing group state
        GroupStateStore* stateStore = NULL;
        GroupState* groupState = NULL;

        TransitionController transitions;
        
        unsigned long lastRequestTime;
        unsigned int repeatTimer = 0;
        
        int hue = 0;
        int saturation = 100;
        bool writeState = false;

        void onPacketSentHandler(uint8_t* packet, const MiLightRemoteConfig& config);
        void onPacketReceivedHandler(uint8_t* packet, const MiLightRemoteConfig& config);
        void handleListen();

        void applySettings();

        List<BulbId> bulbIds;
        List<Request> requests;
        std::vector<MiOutput> miOutputs;
        int i = 0;

        void updateOutput(light::LightState *state, JsonObject result);
    };

  }  // namespace mi
}  // namespace esphome
