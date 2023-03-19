#pragma once

#include "esphome/core/component.h"
#include "esphome/components/mi/mi.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
  namespace mi {

  class MiLight : public Component, public light::LightOutput {
   public:

    MiLight();
    ~MiLight();

    void set_bulb_id(uint16_t deviceId, uint8_t groupId, String remoteType);

    void setup() override;
    void dump_config() override;

    void set_mi_parent(Mi *parent) { this->parent_ = parent; }

    light::LightTraits get_traits() override;
    void setup_state(light::LightState *state) override;
    void write_state(light::LightState *state) override;

    void set_cold_white_temperature(float cold_white_temperature) { cold_white_temperature_ = cold_white_temperature; }
    void set_warm_white_temperature(float warm_white_temperature) { warm_white_temperature_ = warm_white_temperature; }

    float mi_color_temperature(float real_color_temperature) {
      float pct = (real_color_temperature - cold_white_temperature_) / color_temperature_range();
      return pct * MI_MIREDS_RANGE + MI_MIREDS_MIN;
    }
    float real_color_temperature(float mi_color_temperature) {
      float pct = (mi_color_temperature - MI_MIREDS_MIN) / MI_MIREDS_RANGE;
      return pct * color_temperature_range() + cold_white_temperature_;
    }

   private:
    const float MI_MIREDS_MIN = 153;
    const float MI_MIREDS_MAX = 370;
    const float MI_MIREDS_RANGE = MI_MIREDS_MAX - MI_MIREDS_MIN;
    float cold_white_temperature_;
    float warm_white_temperature_;
    float color_temperature_range() { return warm_white_temperature_ - cold_white_temperature_; }

    BulbId bulbId = {0, 0, REMOTE_TYPE_RGB_CCT};
    Mi *parent_;
    light::LightState *state_{nullptr};
  };

  }  // namespace mi
}  // namespace esphome
