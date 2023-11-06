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
    
    Mi::Mi() {
      settings = Settings();
    }

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

      this->updateState(bulbId, result, true);
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
      
      if (ESPHOME_LOG_LEVEL_DEBUG) {
        char responseBody[200];
        char* responseBuffer = responseBody;

        responseBuffer += sprintf_P(
          responseBuffer,
          PSTR("\n%s packet received (%d bytes):\n"),
          config.name.c_str(),
          config.packetFormatter->getPacketLength()
        );
        config.packetFormatter->format(packet, responseBuffer);

        ESP_LOGD(TAG, "Received packet: %s", responseBody);
      }

      this->updateState(bulbId, result, false);
    }

    /**
     * Update internal groupstate
     */
    void Mi::updateState(BulbId bulbId, JsonObject requestJson, bool local) {
      
      MiBridgeData data;
      data.device_id = bulbId.deviceId;
      data.group_id = bulbId.groupId;
      data.remote_type = MiLightRemoteTypeHelpers::remoteTypeToString(bulbId.deviceType);
      serializeJson(requestJson, data.command);
      this->data_callback_.call(data);
      
      for (MiOutput miOutput : Mi::miOutputs) {
        //also listen to groupId 0
        if (
            bulbId.deviceId == miOutput.bulbId.deviceId &&
             (
              bulbId.groupId == miOutput.bulbId.groupId ||
              bulbId.groupId == 0
             )
           )
        {
          // update state to reflect changes from this packet
          groupState = stateStore->get(bulbId);

          // pass in previous scratch state as well
          const GroupState stateUpdates(groupState, requestJson);

          if (groupState != NULL) {
            groupState->patch(stateUpdates);

            // Copy state before setting it to avoid group 0 re-initialization clobbering it
            stateStore->set(bulbId, stateUpdates);
            
            light::LightState* state;
            if (bulbId.groupId == 0) {
              for (MiOutput miOutput : Mi::miOutputs) {
                if (bulbId.deviceId == miOutput.bulbId.deviceId) {
                  state = App.get_light_by_key(miOutput.key);
                  Mi::updateOutput(state, requestJson);
                }
              }
            } else {
              if (local == false) {
                state = App.get_light_by_key(miOutput.key);
                Mi::updateOutput(state, requestJson);
              }
            }
            
            char buff[200];
            serializeJson(requestJson, buff);
            ESP_LOGD(TAG, "Received Milight request: %s", buff);
          }
          break;
        }
      }
    }

    /**
     * Update ESPhome light state
     */
    void Mi::updateOutput(light::LightState *state, JsonObject result) {

      MiLight* output = (MiLight*)(state->get_output());
      if (result.containsKey("state")) {
        state->current_values.set_state(result["state"] == "ON");
        state->remote_values.set_state(result["state"] == "ON");
      }
      if (result.containsKey("color_temp")) {
        float color_temp = output->real_color_temperature((float)result["color_temp"]);
        state->current_values.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
        state->current_values.set_color_temperature(color_temp);
        state->remote_values.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
        state->remote_values.set_color_temperature(color_temp);
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
        Mi::hue = (int)result["hue"];
      }
      if (result.containsKey("saturation")) {
        colorMode = true;
        Mi::saturation = (int)result["saturation"];
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
      output->update_state(state);
    }

    /**
     * Send milight commands
     */
    void Mi::handleCommand(BulbId bulbId, String command) {
      
      StaticJsonDocument<200> buffer;
      deserializeJson(buffer, command);
      JsonVariant variant = buffer.as<JsonVariant>();

      ESP_LOGD("mi", "device_id: %i", bulbId.deviceId);
      ESP_LOGD("mi", "group_id: %i", bulbId.groupId);
      ESP_LOGD("mi", "mi_command: %s", command.c_str());

      milightClient->prepare(bulbId.deviceType, bulbId.deviceId, bulbId.groupId);
      milightClient->handleCommand(variant);
    }

    /**
     * Listen for packets on one radio config.  Cycles through all configs as its
     * called.
     */
    void  Mi::handleListen() {
      
      // Do not handle listens while there are packets enqueued to be sent
      // Doing so causes the radio module to need to be reinitialized inbetween
      // repeats, which slows things down.
      if (packetSender->isSending() || writingState == true) {
        return;
      }

      milightClient->prepare(Mi::miOutputs[i].bulbId.deviceType, 0, 0);
      std::shared_ptr<MiLightRadio> radio = radios->switchRadio(Mi::miOutputs[i].bulbId.deviceType);
      
      if (radios->available()) {
        uint8_t readPacket[MILIGHT_MAX_PACKET_LENGTH];
        size_t packetLen = radios->read(readPacket);

        const MiLightRemoteConfig* remoteConfig = MiLightRemoteConfig::fromType(Mi::miOutputs[i].bulbId.deviceType);

        if (remoteConfig == NULL) {
          // This can happen under normal circumstances, so not an error condition
          ESP_LOGD(TAG, "WARNING: Couldn't find remote for received packet");
          return;
        }

        // update state to reflect this packet
        onPacketReceivedHandler(readPacket, *remoteConfig);
      }

      i++;
      if (i > miOutputs.size()-1) {i=0;}
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
        settings
      );
    }

    void Mi::setup() {

      Mi::applySettings();

      repeatTimer = random(2000, 3000);

      ESP_LOGD(TAG, "Setup complete"); 
    }


     /**
     * write incoming state from ESPhome to MiLight.
     */
    void Mi::write_state(BulbId bulbId, light::LightState *state) {

      for (MiOutput miOutput : Mi::miOutputs) {
        if (bulbId == miOutput.bulbId) {
           light::LightState* state = App.get_light_by_key(miOutput.key);
           break;
        }
      }
      
      StaticJsonDocument<200> root;
      JsonObject requestJson = root.to<JsonObject>();
      
      std::string effect;
      bool effectExist = false;
      if (state->supports_effects()) {
        effect = state->get_effect_name();
       
        if (effect.size() > 0 && effect != "None") {
         effectExist = true;
        }
      }
      
      auto values = state->current_values;
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

        JsonObject color = root.createNestedObject("color");

        if (values.get_color_mode() & light::ColorCapability::RGB) {
          color["r"] = uint8_t(values.get_color_brightness() * values.get_red() * 255);
          color["g"] = uint8_t(values.get_color_brightness() * values.get_green() * 255);
          color["b"] = uint8_t(values.get_color_brightness() * values.get_blue() * 255);
        }

        if (values.get_color_mode() & light::ColorCapability::WHITE) {
          color["w"] = uint8_t(values.get_white() * 255);
          root["white_value"] = uint8_t(values.get_white() * 255);  // legacy API
        }

         if (values.get_color_mode() & light::ColorCapability::COLD_WARM_WHITE) {
          color["c"] = uint8_t(values.get_cold_white() * 255);
          color["w"] = uint8_t(values.get_warm_white() * 255);
        }
        
        if (!(values.get_color_mode() & light::ColorCapability::RGB)) {
          root.remove("color");
        }
        
        //Remove unwanted color and white commands when set RGBW light to Off
        if (values.get_color_mode() == light::ColorMode::RGB_WHITE) {
          if (values.get_state() == 0.0f) {
            root.remove("color");
            root.remove("white_value");
            root.remove("brightness");
          }
        }

        if (values.get_color_mode() & light::ColorCapability::COLOR_TEMPERATURE) {
          root["color_temp"] = uint32_t(values.get_color_temperature());
        }
      }
      
      this->writeState(bulbId, requestJson);
    }

    /**
     * write incoming json state from ESPhome to MiLight.
     */
    void Mi::write_state(BulbId bulbId, std::string command) {

      StaticJsonDocument<400> buffer;
      deserializeJson(buffer, command.c_str());
      JsonObject requestJson = buffer.as<JsonObject>();
      
      this->writeState(bulbId, requestJson);
      this->updateState(bulbId, requestJson, false);
    }

    /**
     * write incoming state from ESP home to MiLight.
     */
    void Mi::writeState(BulbId bulbId, JsonObject requestJson) {
      
      writingState = true;
      
      if (millis() - lastRequestTime < 2000) {
        //ESP_LOGD(TAG, "Milight setRepeatsOverride to 10");
        milightClient->setRepeatsOverride(10);
      }

      //dont write anything the first 5 seconds after boot to prevent wrong device assignment after power loss
      if (millis() > 5000) {
        milightClient->prepare(bulbId.deviceType, bulbId.deviceId, bulbId.groupId);
        milightClient->update(requestJson);
      }
      
      milightClient->clearRepeatsOverride();
      
      if (settings.resendLastCommand == true) {
        Request request = Request();
        serializeJson(requestJson, request.request);
        ESP_LOGD(TAG, "Send Milight request: %s", request.request);

        int pos = bulbCompactIds.IndexOf(bulbId.getCompactId());
        if (pos > -1) {
          requests.Replace(pos, request);
        } else {
          bulbCompactIds.Add(bulbId.getCompactId());
          requests.Add(request);
        }
      }
      
      lastRequestTime = millis();
      writingState = false;
    }


    void Mi::loop() {

      Mi::handleListen();

      stateStore->limitedFlush();
      packetSender->loop();
      
       while (millis() - lastRequestTime > repeatTimer && bulbCompactIds.Count() > 0) {

        uint32_t bulbCompactId = bulbCompactIds.First();
        bulbCompactIds.RemoveFirst();
        Request request = requests.First();
        requests.RemoveFirst();
        
        //uint32_t bulbCompactId = (deviceId << 16) | (deviceType << 8) | groupId;
        uint16_t deviceId = (bulbCompactId >> 16);
        MiLightRemoteType deviceType = MiLightRemoteType((bulbCompactId >> 8) & 0xFF);
        uint8_t groupId = bulbCompactId & 0xFF;

        StaticJsonDocument<400> buffer;
        deserializeJson(buffer, request.request);
        JsonObject obj = buffer.as<JsonObject>();

        if (bulbCompactIds.Count() == 0) {
          requests.Clear();
        }
        
        //dont write anything the first 5 seconds after boot to prevent wrong device assignment after power loss
        if (millis() > 5000) {
          milightClient->prepare(deviceType, deviceId, groupId);
          milightClient->update(obj);
        }
      }
    }
  }  // namespace mi
}  // namespace esphome
