#include "FS.h"
#include "mi.h"
#include "esphome.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"

namespace esphome {
  namespace mi {

    static const char *const TAG = "mi";

    void Mi::dump_config() { }

    /**
     * Milight RF packet handler.
     *
     * Called when a packet is sent locally
     */
    void  Mi::onPacketSentHandler(uint8_t* packet, const MiLightRemoteConfig& config) {
      StaticJsonDocument<200> buffer;
      JsonObject result = buffer.to<JsonObject>();
      
      BulbId bulbId = config.packetFormatter->parsePacket(packet, result);
      if (&bulbId == &DEFAULT_BULB_ID) {
        ESP_LOGD(TAG, "Skipping packet handler because packet was not decoded");
        return;
      }

      ESP_LOGD(TAG, "onPacketSentHandler - update the status of group");

      // update state to reflect changes from this packet
      groupState = stateStore->get(bulbId);

      // pass in previous scratch state as well
      const GroupState stateUpdates(groupState, result);

      if (groupState != NULL) {
        groupState->patch(stateUpdates);

        // Copy state before setting it to avoid group 0 re-initialization clobbering it
        stateStore->set(bulbId, stateUpdates);
      }
    }

    /**
     * Milight RF packet handler.
     *
     * Called  when an intercepted packet is read.
     */
    void  Mi::onPacketReceivedHandler(uint8_t* packet, const MiLightRemoteConfig& config) {
      
      StaticJsonDocument<200> buffer;
      JsonObject result = buffer.to<JsonObject>();
      
      BulbId bulbId = config.packetFormatter->parsePacket(packet, result);
      
      if (&bulbId == &DEFAULT_BULB_ID) {
        ESP_LOGD(TAG, "Skipping packet handler because packet was not decoded");
        return;
      }
        
      ESP_LOGD(TAG, "bulbId.deviceType: %s", MiLightRemoteTypeHelpers::remoteTypeToString(bulbId.deviceType).c_str());
      ESP_LOGD(TAG, "bulbId.deviceId: %4X", bulbId.deviceId);
      ESP_LOGD(TAG, "bulbId.groupId: %i", bulbId.groupId);
      
      for (MiOutput miOutput : Mi::miOutputs) {
        if (bulbId == miOutput.bulbId) {

          light::LightState* state = App.get_light_by_key(miOutput.key);
          // update state to reflect changes from this packet
          groupState = stateStore->get(bulbId);

          // pass in previous scratch state as well
          const GroupState stateUpdates(groupState, result);

          if (groupState != NULL) {
            groupState->patch(stateUpdates);

            // Copy state before setting it to avoid group 0 re-initialization clobbering it
            stateStore->set(bulbId, stateUpdates);
            
            if (result.containsKey("state")) {
              state->current_values.set_state(result["state"] == "ON");
              state->remote_values.set_state(result["state"] == "ON");
            }
            if (result.containsKey("color_temp")) {
              state->current_values.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
              state->current_values.set_color_temperature((float)result["color_temp"]);
              state->remote_values.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
              state->remote_values.set_color_temperature((float)result["color_temp"]);
            }
            if (result.containsKey("brightness")) {
              state->current_values.set_brightness((float)result["brightness"]/255.00);
              state->remote_values.set_brightness((float)result["brightness"]/255.00);
            }
            if (result.containsKey("command")) {
              if(result["command"] == "night_mode"){
                //TODO
              }
            }
            
            bool colorMode = false;
            if (result.containsKey("hue")) {
              colorMode = true;
              hue = (int)result["hue"];
            }
            if (result.containsKey("saturation")) {
              colorMode = true;
              saturation = (int)result["saturation"];
            }
            
            if (colorMode) {
              float rgb[3];  
              
              /// Convert hue (0-360) & saturation/value percentage (0-1) to RGB floats (0-1)
              //hsv_to_rgb(int hue, float saturation, float value, float &red, float &green, float &blue);   
              hsv_to_rgb(hue, saturation/100.0, 1.0, rgb[0], rgb[1], rgb[2]);
              
              state->current_values.set_color_mode(light::ColorMode::RGB);
              state->current_values.set_red(rgb[0]);
              state->current_values.set_green(rgb[1]);
              state->current_values.set_blue(rgb[2]);
              state->remote_values.set_color_mode(light::ColorMode::RGB);
              state->remote_values.set_red(rgb[0]);
              state->remote_values.set_green(rgb[1]);
              state->remote_values.set_blue(rgb[2]);
              
              colorMode = false;
            }    
              
            state->publish_state();
            state->get_output()->update_state(state);
          }
          break;
        }
      }
    }

    /**
     * Listen for packets on one radio config.  Cycles through all configs as its
     * called.
     */
    void  Mi::handleListen() {
      
      // Do not handle listens while there are packets enqueued to be sent
      // Doing so causes the radio module to need to be reinitialized inbetween
      // repeats, which slows things down.
      if (packetSender->isSending() || writeState == true) {
        return;
      }

      std::shared_ptr<MiLightRadio> radio = radios->switchRadio(Mi::miOutputs[i].bulbId.deviceType);
      i++;
      if (i > miOutputs.size()-1) {i=0;}

      if (radios->available()) {
        uint8_t readPacket[MILIGHT_MAX_PACKET_LENGTH];
        size_t packetLen = radios->read(readPacket);

        const MiLightRemoteConfig* remoteConfig = MiLightRemoteConfig::fromReceivedPacket(
          radio->config(),
          readPacket,
          packetLen
        );

        if (remoteConfig == NULL) {
          // This can happen under normal circumstances, so not an error condition
          ESP_LOGD(TAG, "WARNING: Couldn't find remote for received packet");
          return;
        }

        // update state to reflect this packet
        onPacketReceivedHandler(readPacket, *remoteConfig);
      }
    }

    /**
     * Apply what's in the Settings object.
     */
    void  Mi::applySettings() {
      if (milightClient) {
        delete milightClient;
      }
      if (stateStore) {
        delete stateStore;
      }
      if (packetSender) {
        delete packetSender;
      }
      if (radios) {
        delete radios;
      }

      transitions.setDefaultPeriod(settings.defaultTransitionPeriod);
      radioFactory = MiLightRadioFactory::fromSettings(settings);

      if (radioFactory == NULL) {
        ESP_LOGD(TAG, "ERROR: unable to construct radio factory");
      }

      stateStore = new GroupStateStore(100, settings.stateFlushInterval);

      radios = new RadioSwitchboard(radioFactory, stateStore, settings);
      using std::placeholders::_1;
      using std::placeholders::_2;
      packetSender = new PacketSender(*radios, settings, std::bind(&Mi::onPacketSentHandler, this, _1, _2));

      milightClient = new MiLightClient(
        *radios,
        *packetSender,
        stateStore,
        settings,
        transitions
      );
    }

    void Mi::setup() {

      SPIFFS.begin();
      Settings::load(settings);
      Mi::applySettings();

      transitions.addListener(
        [&](const BulbId& bulbId, GroupStateField field, uint16_t value) {
          StaticJsonDocument<100> buffer;

          const char* fieldName = GroupStateFieldHelpers::getFieldName(field);
          buffer[fieldName] = value;

          milightClient->prepare(bulbId.deviceType, bulbId.deviceId, bulbId.groupId);
          milightClient->update(buffer.as<JsonObject>());
        }
      );

      repeatTimer = random(2000, 3000);

      ESP_LOGD(TAG, "Setup complete"); 
    }

    void Mi::write_state(BulbId bulbId, light::LightState *state) {
      
      writeState = true;

      for (MiOutput miOutput : Mi::miOutputs) {
        if (bulbId == miOutput.bulbId) {
           light::LightState* state = App.get_light_by_key(miOutput.key);
           break;
        }
      }
      
      StaticJsonDocument<200> root;
      JsonObject request = root.to<JsonObject>();
      
      std::string effect;
      bool effectExist = false;
      if (state->supports_effects()) {
        effect = state->get_effect_name();
       
        if (effect.size() > 0 && effect != "None") {
         effectExist = true;
        }
      }
      
      auto values = state->remote_values;
      auto traits = state->get_traits();
        
      if (effectExist) {
        
        char *effectChr = new char[effect.size()+1];
        std::copy(effect.begin(), effect.end(), effectChr);
        effectChr[effect.size()] = '\0';
         
        root["effect"] = effectChr;
      } else {
       
        switch (values.get_color_mode()) {
          case light::ColorMode::UNKNOWN:  // don't need to set color mode if we don't know it
            break;
          case light::ColorMode::ON_OFF:
            root["color_mode"] = "onoff";
            break;
          case light::ColorMode::BRIGHTNESS:
            root["color_mode"] = "brightness";
            break;
          case light::ColorMode::WHITE:  // not supported by HA in MQTT
            root["color_mode"] = "white";
            break;
          case light::ColorMode::COLOR_TEMPERATURE:
            root["color_mode"] = "color_temp";
            break;
          case light::ColorMode::COLD_WARM_WHITE:  // not supported by HA
            root["color_mode"] = "cwww";
            break;
          case light::ColorMode::RGB:
            root["color_mode"] = "rgb";
            break;
          case light::ColorMode::RGB_WHITE:
            root["color_mode"] = "rgbw";
            break;
          case light::ColorMode::RGB_COLOR_TEMPERATURE:  // not supported by HA
            root["color_mode"] = "rgbct";
            break;
          case light::ColorMode::RGB_COLD_WARM_WHITE:
            root["color_mode"] = "rgbww";
            break;
        }
       
        if (values.get_color_mode() & light::ColorCapability::ON_OFF) {
          root["state"] = (values.get_state() != 0.0f) ? "ON" : "OFF";
        }
        if (values.get_color_mode() & light::ColorCapability::BRIGHTNESS) {
          root["brightness"] = uint8_t(values.get_brightness() * 255);
        }
       
        if (values.get_color_mode() & light::ColorCapability::RGB) {
          JsonObject color = root.createNestedObject("color");
          color["r"] = uint8_t(values.get_color_brightness() * values.get_red() * 255);
          color["g"] = uint8_t(values.get_color_brightness() * values.get_green() * 255);
          color["b"] = uint8_t(values.get_color_brightness() * values.get_blue() * 255);
        }
        if (values.get_color_mode() & light::ColorCapability::WHITE) {
          JsonObject color = root.createNestedObject("color");
          color["w"] = uint8_t(values.get_white() * 255);
          root["white_value"] = uint8_t(values.get_white() * 255);  // legacy API
        }
        if (values.get_color_mode() & light::ColorCapability::COLOR_TEMPERATURE) {
          root["color_temp"] = uint32_t(values.get_color_temperature());
        }
        if (values.get_color_mode() & light::ColorCapability::COLD_WARM_WHITE) {
          JsonObject color = root.createNestedObject("color");
          color["c"] = uint8_t(values.get_cold_white() * 255);
          color["w"] = uint8_t(values.get_warm_white() * 255);
        }
      }
      
      if (millis() - lastCommandTime < 2000) {
        ESP_LOGD(TAG, "Milight setRepeatsOverride to 10");
        milightClient->setRepeatsOverride(10);
      }

      //dont write anything the first 5 seconds after boot to prevent wrong device assignment after power loss
      if (millis() > 5000) {
        milightClient->prepare(bulbId.deviceType, bulbId.deviceId, bulbId.groupId);
        milightClient->update(request);
      }
      
      milightClient->clearRepeatsOverride();
      
      Command command = Command();
      serializeJson(request, command.command);
      ESP_LOGD(TAG, "Milight request: %s", command.command);

      int pos = bulbIds.IndexOf(bulbId);
      if (pos > -1) {
        commands.Replace(pos, command);
      } else {
        bulbIds.Add(bulbId);
        commands.Add(command);
      }
      
      lastCommandTime = millis();
      writeState = false;
    }


    void Mi::loop() {

      Mi::handleListen();

      stateStore->limitedFlush();
      packetSender->loop();

      transitions.loop();
      
      while (millis() - lastCommandTime > repeatTimer && bulbIds.Count() > 0) {
    
        BulbId bulbId = bulbIds.First();
        bulbIds.RemoveFirst();
        Command command = commands.First();
        commands.RemoveFirst();

        StaticJsonDocument<400> buffer;
        deserializeJson(buffer, command.command);
        JsonObject obj = buffer.as<JsonObject>();

        if (bulbIds.Count() == 0) {
          commands.Clear();
        }
        
        //dont write anything the first 5 seconds after boot to prevent wrong device assignment after power loss
        if (millis() > 5000) {
          milightClient->prepare(bulbId.deviceType, bulbId.deviceId, bulbId.groupId);
          milightClient->update(obj);
        }
      }
    }
  }  // namespace mi
}  // namespace esphome
