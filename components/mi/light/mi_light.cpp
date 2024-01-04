#include "esphome.h"
#include "esphome/core/log.h"
#include "mi_light.h"
#include "../MiLightCommands.h"
#include "esphome/core/helpers.h"


namespace esphome {
  namespace mi {

    static const char *const TAG = "mi.light";

    MiLight::MiLight() {}

    void MiLight::set_bulb_id(uint16_t deviceId, uint8_t groupId, String remoteType) 
    {
      MiLight::bulbId = {
        deviceId, 
        groupId, 
        MiLightRemoteConfig::fromType(remoteType)->type
      };
    }

    void MiLight::setup() {}

    void MiLight::dump_config() {
      ESP_LOGCONFIG(TAG, "MiLight:");
      ESP_LOGCONFIG(TAG, " ObjectId: %s", state_->get_object_id().c_str());
      ESP_LOGCONFIG(TAG, " bulbId.deviceType: %s", MiLightRemoteTypeHelpers::remoteTypeToString(bulbId.deviceType).c_str());
      ESP_LOGCONFIG(TAG, " bulbId.deviceId: %04X", bulbId.deviceId);
      ESP_LOGCONFIG(TAG, " bulbId.groupId: %i", bulbId.groupId);
    }

    light::LightTraits MiLight::get_traits() {
      auto traits = light::LightTraits();

      switch (MiLight::bulbId.deviceType) {
        case REMOTE_TYPE_RGB_CCT:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
          traits.set_max_mireds(warm_white_temperature_);
          traits.set_min_mireds(cold_white_temperature_);
          break;
        case REMOTE_TYPE_RGB:
          traits.set_supported_color_modes({light::ColorMode::RGB});
          break;
        case REMOTE_TYPE_RGBW:
          traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
          break;
        case REMOTE_TYPE_CCT:
          traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
          traits.set_max_mireds(warm_white_temperature_);
          traits.set_min_mireds(cold_white_temperature_);
          break;
        case REMOTE_TYPE_FUT089:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
          traits.set_max_mireds(warm_white_temperature_);
          traits.set_min_mireds(cold_white_temperature_);
          break;
        case REMOTE_TYPE_FUT091:
          traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
          traits.set_max_mireds(warm_white_temperature_);
          traits.set_min_mireds(cold_white_temperature_);
          break;
        case REMOTE_TYPE_FUT020:
          traits.set_supported_color_modes({light::ColorMode::RGB});
          break;
        case REMOTE_TYPE_S2:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
          traits.set_max_mireds(warm_white_temperature_);
          traits.set_min_mireds(cold_white_temperature_);
          break;
        default:
          traits.set_supported_color_modes({light::ColorMode::ON_OFF});
          break;
      }
      return traits;
    }

    void MiLight::setup_state(light::LightState *state) { 
      state_ = state;

      parent_->add_child(state_->get_object_id_hash(), bulbId);

      state_->add_effects({new light::LambdaLightEffect(DISCO_MODE_NAMES[0], [=](bool initial_run) -> void {
        auto call = state_->make_call();
        call.set_effect(DISCO_MODE_NAMES[0]);
        call.perform();
        }, 0xffffffff)
      });
      
      if (MiLight::bulbId.deviceType == REMOTE_TYPE_RGB_CCT || 
          MiLight::bulbId.deviceType == REMOTE_TYPE_RGB || 
          MiLight::bulbId.deviceType == REMOTE_TYPE_RGBW || 
          MiLight::bulbId.deviceType == REMOTE_TYPE_FUT089 ||
          MiLight::bulbId.deviceType == REMOTE_TYPE_FUT020
          ) {
        
        // Add the 9 built-in effects with descriptive names...
        for (int i = 1; i < 10; i++) {
          state_->add_effects({new light::LambdaLightEffect(DISCO_MODE_NAMES[i], [=](bool initial_run) -> void {
            auto call = state_->make_call();
            call.set_effect(DISCO_MODE_NAMES[i]);
            call.perform();
            }, 0xffffffff)
          });
        }
      }
    }

    void MiLight::write_state(light::LightState *state) {
      state->current_values.set_color_temperature(
        mi_color_temperature(state->current_values.get_color_temperature())
      );
      parent_->write_state(bulbId, state);
    }
  }  // namespace mi
}  // namespace esphome
