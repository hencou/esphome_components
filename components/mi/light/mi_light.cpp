#include "esphome.h"
#include "esphome/core/log.h"
#include "mi_light.h"
#include "../MiLightCommands.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace mi {

static const char *const TAG = "mi.light";

// ---- EFFECT SYSTEM (STATLESS, ESPHOME 2025.11.0 COMPATIBLE) ----
constexpr int MAX_DISCO_EFFECTS = 10;
//global storage for effects
static light::LightState *g_state = nullptr;
static const char *g_names[MAX_DISCO_EFFECTS] = {nullptr};

// template function (template → functiepointer per effect)
template<int IDX>
static void disco_fn(bool initial_run) {
  if (!g_state) return;
  if (!g_names[IDX]) return;

  auto call = g_state->make_call();
  call.set_effect(g_names[IDX]);
  call.perform();
}

// runtime index → select compile-time function
static void register_effect(int i) {
  const char *name = g_names[i];

  switch (i) {
    case 0:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<0>, 0xffffffff) });
      break;
    case 1:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<1>, 0xffffffff) });
      break;
    case 2:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<2>, 0xffffffff) });
      break;
    case 3:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<3>, 0xffffffff) });
      break;
    case 4:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<4>, 0xffffffff) });
      break;
    case 5:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<5>, 0xffffffff) });
      break;
    case 6:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<6>, 0xffffffff) });
      break;
    case 7:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<7>, 0xffffffff) });
      break;
    case 8:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<8>, 0xffffffff) });
      break;
    case 9:
      g_state->add_effects({ new light::LambdaLightEffect(name, disco_fn<9>, 0xffffffff) });
      break;
    default:
      ESP_LOGE(TAG, "Effect index out of range: %d", i);
      break;
  }
}


MiLight::MiLight() {}

void MiLight::set_bulb_id(uint16_t deviceId, uint8_t groupId, std::string remoteType) {
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

#ifdef USE_DEVICES
  parent_->add_child(state_->get_object_id_hash(), state_->get_device_id(), bulbId);
#else
  parent_->add_child(state_->get_object_id_hash(), 0, bulbId);
#endif

  // ---- STATELESS EFFECT SYSTEM ACTIVATION ----
  g_state = state_;

  // effect 0
  g_names[0] = DISCO_MODE_NAMES[0];
  register_effect(0);

  // effects (1..9)
  if (MiLight::bulbId.deviceType == REMOTE_TYPE_RGB_CCT ||
      MiLight::bulbId.deviceType == REMOTE_TYPE_RGB ||
      MiLight::bulbId.deviceType == REMOTE_TYPE_RGBW ||
      MiLight::bulbId.deviceType == REMOTE_TYPE_FUT089 ||
      MiLight::bulbId.deviceType == REMOTE_TYPE_FUT020) {

    for (int i = 1; i < 10; i++) {
      g_names[i] = DISCO_MODE_NAMES[i];
      register_effect(i);
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
