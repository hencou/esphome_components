#include "esphome.h"
#include "esphome/core/log.h"
#include "mi_light.h"
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
      ESP_LOGCONFIG(TAG, " bulbId.deviceId: %04X", bulbId.deviceId);
      ESP_LOGCONFIG(TAG, " bulbId.groupId: %i", bulbId.groupId);
      ESP_LOGCONFIG(TAG, " bulbId.remoteType: %i", MiLightRemoteTypeHelpers::remoteTypeToString(bulbId.deviceType).c_str());
    }

    light::LightTraits MiLight::get_traits() {
      auto traits = light::LightTraits();

      switch (MiLight::bulbId.deviceType) {
        case REMOTE_TYPE_RGB_CCT:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
          break;
        case REMOTE_TYPE_RGB:
          traits.set_supported_color_modes({light::ColorMode::RGB});
          break;
        case REMOTE_TYPE_RGBW:
          traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
          break;
        case REMOTE_TYPE_CCT:
          traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
          break;
        case REMOTE_TYPE_FUT089:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
          break;
        case REMOTE_TYPE_FUT091:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
          break;
        case REMOTE_TYPE_FUT020:
          traits.set_supported_color_modes({light::ColorMode::RGB});
          break;
        default:
          traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::ON_OFF});
          break;
      }
      traits.set_max_mireds(370);
      traits.set_min_mireds(153);
      return traits;
    }

    void MiLight::setup_state(light::LightState *state) { 
      state_ = state;

      parent_->add_child(state_->get_object_id_hash(), bulbId);

      state_->set_default_transition_length(0);
      state_->add_effects({new light::LambdaLightEffect("night_mode", [=]() -> void {
        auto call = state_->make_call();
        call.set_effect("night_mode");
        call.perform();
        }, 360000)
      });
      
      if (MiLight::bulbId.deviceType != REMOTE_TYPE_CCT) {
        state_->add_effects({new light::LambdaLightEffect("1", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("1");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("2", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("2");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("3", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("3");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("4", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("4");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("5", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("5");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("6", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("6");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("7", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("7");
          call.perform();
          }, 360000)
        });
        state_->add_effects({new light::LambdaLightEffect("8", [=]() -> void {
          auto call = state_->make_call();
          call.set_effect("8");
          call.perform();
          }, 360000)
        });
      }
    }

    void MiLight::write_state(light::LightState *state) {
      parent_->write_state(bulbId, state);
    }
  }  // namespace mi
}  // namespace esphome
