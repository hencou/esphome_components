#include "remeha.h"
#ifdef USE_CLIMATE
#include "climate/remeha_climate.h"
#endif
#include "esphome/core/log.h"

namespace esphome {
namespace remeha {

void Remeha::setup() {
  this->boot_time_ms_ = millis();
  this->boot_phase_ = 0;

  // Register CAN frame callback
  this->canbus_->add_callback([this](uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                                     const std::vector<uint8_t> &data) {
    this->handle_frame_(can_id, use_extended_id, remote_transmission_request, data);
  });

  ESP_LOGI(TAG, "Remeha component initialized, boot delay %u ms, user level %u",
           this->boot_delay_ms_, this->user_level_);
}

void Remeha::loop() {
  uint32_t now = millis();

  // --- Boot sequence (phased) ---
  if (this->boot_phase_ < 4) {
    uint32_t elapsed = now - this->boot_time_ms_;
    if (this->boot_phase_ == 0 && elapsed >= this->boot_delay_ms_) {
      // Phase 1: NMT reset
      uint8_t nmt_reset[2] = {0x81, 0x00};
      this->send_can_(0x000, nmt_reset, 2);
      ESP_LOGI(TAG, "NMT reset sent");
      this->boot_phase_ = 1;
      this->boot_time_ms_ = now;
    } else if (this->boot_phase_ == 1 && elapsed >= 500) {
      // Phase 2: NMT start
      uint8_t nmt_start[2] = {0x01, 0x00};
      this->send_can_(0x000, nmt_start, 2);
      ESP_LOGI(TAG, "NMT start sent");
      this->boot_phase_ = 2;
      this->boot_time_ms_ = now;
    } else if (this->boot_phase_ == 2 && elapsed >= 500) {
      // Phase 3: Read 0x4004 to enable custom SDO gateway
      uint8_t gw_read[8] = {0x40, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
      this->send_can_(0x601, gw_read, 8);
      ESP_LOGI(TAG, "Gateway read (0x4004) sent");
      this->boot_phase_ = 3;
      this->boot_time_ms_ = now;
    } else if (this->boot_phase_ == 3 && elapsed >= 2000) {
      // Phase 4: Fallback write if read didn't work
      if (!this->gateway_enabled_) {
        ESP_LOGW(TAG, "0x4004 read did not return ready, trying write...");
        uint8_t gw_write[8] = {0x2F, 0x04, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00};
        this->send_can_(0x601, gw_write, 8);
      }
      this->boot_phase_ = 4;
    }
    return;
  }

  // --- Heartbeat every second ---
  if (now - this->last_heartbeat_ms_ >= 1000) {
    uint8_t hb[8] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->send_can_(0x281, hb, 8);
    this->last_heartbeat_ms_ = now;
  }

  // --- Every 10 seconds: timeouts + gateway check + auth + SDO reads ---
  if (now - this->last_poll_ms_ >= 10000) {
    this->last_poll_ms_ = now;

    // Auth timeout
    if (this->auth_step_ > 0 && (now - this->auth_start_ms_) > 10000) {
      ESP_LOGW(TAG, "Auth timeout at step %d, resetting", this->auth_step_);
      this->auth_step_ = 0;
      this->authenticated_ = false;
    }

    // Write timeout
    if (this->write_pending_ && (now - this->write_start_ms_) > 5000) {
      ESP_LOGW(TAG, "SDO write timeout");
      this->write_pending_ = false;
#ifdef USE_TEXT_SENSOR
      if (this->write_status_ != nullptr)
        this->write_status_->publish_state("Timeout");
#endif
    }

    // Segmented read timeout
    if (this->seg_read_active_ && (now - this->seg_read_start_ms_) > 5000) {
      ESP_LOGW(TAG, "Segmented read timeout at segment %d", this->seg_read_segment_);
      this->seg_read_active_ = false;
      this->seg_read_segment_ = 0;
    }

    // Don't send reads while write or segmented read is pending
    if (this->write_pending_ || this->seg_read_active_)
      return;

    // Re-check gateway if not yet enabled
    if (!this->gateway_enabled_) {
      uint8_t gw[8] = {0x40, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
      this->send_can_(0x601, gw, 8);
      return;
    }

    // If not authenticated, start auth
    if (!this->authenticated_ && this->auth_step_ == 0) {
      this->start_auth_();
      return;
    }

    // SDO reads (round-robin)
    if (this->authenticated_ && this->auth_step_ == 0) {
      this->poll_next_sdo_();
    }
  }
}

void Remeha::dump_config() {
  ESP_LOGCONFIG(TAG, "Remeha:");
  ESP_LOGCONFIG(TAG, "  Boot delay: %u ms", this->boot_delay_ms_);
  ESP_LOGCONFIG(TAG, "  User level: %u", this->user_level_);
  ESP_LOGCONFIG(TAG, "  SDO poll entries: %u", this->sdo_poll_list_.size());
}

void Remeha::send_can_(uint32_t can_id, const uint8_t *data, size_t len) {
  if (this->canbus_ != nullptr) {
    this->canbus_->send_data(can_id, false, false, std::vector<uint8_t>(data, data + len));
  }
}

void Remeha::add_sdo_poll(uint16_t index, uint8_t subindex) {
  // Avoid duplicates
  for (const auto &entry : this->sdo_poll_list_) {
    if (entry.index == index && entry.subindex == subindex)
      return;
  }
  this->sdo_poll_list_.push_back({index, subindex});
}

void Remeha::start_auth_() {
  ESP_LOGI(TAG, "Attempting authentication (level %u)...", this->user_level_);
  // Read serial number from 0x2001 sub 0x0A
  uint8_t rd[8] = {0x40, 0x01, 0x20, 0x0A, 0x00, 0x00, 0x00, 0x00};
  this->send_can_(0x241, rd, 8);
  this->auth_step_ = 1;
  this->auth_start_ms_ = millis();
}

void Remeha::poll_next_sdo_() {
  if (this->sdo_poll_list_.empty())
    return;

  int step = this->sdo_read_step_ % this->sdo_poll_list_.size();
  uint16_t idx = this->sdo_poll_list_[step].index;
  uint8_t sub = this->sdo_poll_list_[step].subindex;
  uint8_t data[8] = {0x40, (uint8_t)(idx & 0xFF), (uint8_t)(idx >> 8), sub,
                     0x00, 0x00, 0x00, 0x00};
  this->send_can_(0x241, data, 8);
  this->sdo_read_step_ = (step + 1) % this->sdo_poll_list_.size();
}

bool Remeha::write_sdo(uint16_t index, uint8_t subindex, uint32_t value, uint8_t size) {
  if (!this->authenticated_) {
    ESP_LOGW(TAG, "Cannot write: not authenticated");
#ifdef USE_TEXT_SENSOR
    if (this->write_status_ != nullptr)
      this->write_status_->publish_state("Not authenticated");
#endif
    return false;
  }
  if (this->write_pending_) {
    ESP_LOGW(TAG, "Write busy, try again later");
#ifdef USE_TEXT_SENSOR
    if (this->write_status_ != nullptr)
      this->write_status_->publish_state("Busy");
#endif
    return false;
  }

  uint8_t cmd;
  switch (size) {
    case 1: cmd = 0x2F; break;
    case 2: cmd = 0x2B; break;
    default: cmd = 0x23; break;
  }

  uint8_t data[8] = {cmd, (uint8_t)(index & 0xFF), (uint8_t)(index >> 8), subindex,
                     (uint8_t)(value & 0xFF), (uint8_t)((value >> 8) & 0xFF),
                     (uint8_t)((value >> 16) & 0xFF), (uint8_t)((value >> 24) & 0xFF)};
  this->send_can_(0x241, data, 8);
  this->write_pending_ = true;
  this->write_start_ms_ = millis();

  ESP_LOGI(TAG, "WRITE SDO 0x%04X sub %d = %u (cmd=0x%02X)", index, subindex, value, cmd);

#ifdef USE_TEXT_SENSOR
  if (this->write_status_ != nullptr)
    this->write_status_->publish_state("Sending...");
#endif
  return true;
}

void Remeha::tea_encrypt_(uint32_t v[2], const uint32_t k[4]) {
  uint32_t v0 = v[0], v1 = v[1];
  uint32_t sum = 0;
  const uint32_t delta = 0x9E3779B9u;
  for (int i = 0; i < 32; i++) {
    sum += delta;
    v0 += ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
    v1 += ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
  }
  v[0] = v0;
  v[1] = v1;
}

// --- CAN frame dispatcher ---
void Remeha::handle_frame_(uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                           const std::vector<uint8_t> &data) {
  switch (can_id) {
    case 0x581: this->handle_0x581_(data); break;
    case 0x1C1: this->handle_0x1c1_(data); break;
    case 0x282: this->handle_pdo_0x282_(data); break;
    case 0x381: this->handle_pdo_0x381_(data); break;
    case 0x382: this->handle_pdo_0x382_(data); break;
    case 0x481: this->handle_pdo_0x481_(data); break;
    case 0x482: this->handle_pdo_0x482_(data); break;
    default: break;
  }
}

// --- Standard SDO response (0x581): gateway ready detection ---
void Remeha::handle_0x581_(const std::vector<uint8_t> &x) {
  if (x.size() < 4) return;
  uint8_t cmd = x[0];
  uint16_t index = ((uint16_t)x[2] << 8) | x[1];
  uint8_t sub = x[3];

  if (index == 0x4004 && sub == 0x00) {
    if ((cmd == 0x4F || cmd == 0x4B || cmd == 0x43) && x.size() > 4 && x[4] > 0) {
      if (!this->gateway_enabled_) {
        ESP_LOGI(TAG, "0x4004=0x%02X: custom SDO gateway READY", x[4]);
        this->gateway_enabled_ = true;
        if (!this->authenticated_ && this->auth_step_ == 0) {
          this->start_auth_();
        }
      }
    } else if (cmd == 0x60) {
      if (!this->gateway_enabled_) {
        ESP_LOGI(TAG, "Write to 0x4004 confirmed, gateway enabled");
        this->gateway_enabled_ = true;
        if (!this->authenticated_ && this->auth_step_ == 0) {
          this->start_auth_();
        }
      }
    } else if (cmd == 0x80) {
      ESP_LOGW(TAG, "SDO abort on 0x4004: %02X%02X%02X%02X",
               x.size() > 7 ? x[7] : 0, x.size() > 6 ? x[6] : 0,
               x.size() > 5 ? x[5] : 0, x.size() > 4 ? x[4] : 0);
    }
  }
}

// --- Custom SDO response (0x1C1): auth + data parsing ---
void Remeha::handle_0x1c1_(const std::vector<uint8_t> &x) {
  if (x.size() < 4) return;

  // --- Segmented SDO read handler (for 0x501D trending string) ---
  if (this->seg_read_active_) {
    uint8_t seg_cmd = x[0];
    if ((seg_cmd & 0xE0) == 0x00) {
      int seg = this->seg_read_segment_;
      bool is_last = (seg_cmd & 0x01) != 0;
      // Buffer segment data (7 bytes per segment in x[1]-x[7])
      for (int i = 1; i < (int)x.size() && i <= 7; i++) {
        if (this->seg_read_buffer_pos_ < 96) {
          this->seg_read_buffer_[this->seg_read_buffer_pos_++] = x[i];
        }
      }
      if (is_last) {
        ESP_LOGD(TAG, "Segmented read complete (%d bytes)", this->seg_read_buffer_pos_);
        this->process_trending_data_();
        this->seg_read_active_ = false;
        this->seg_read_segment_ = 0;
        this->seg_read_buffer_pos_ = 0;
      } else {
        this->seg_read_segment_ = seg + 1;
        uint8_t toggle = ((seg + 1) & 1) ? 0x70 : 0x60;
        uint8_t req[8] = {toggle, 0, 0, 0, 0, 0, 0, 0};
        this->send_can_(0x241, req, 8);
      }
      return;
    }
    if (x[0] == 0x80) {
      ESP_LOGW(TAG, "Segmented read aborted by boiler");
      this->seg_read_active_ = false;
      this->seg_read_segment_ = 0;
      this->seg_read_buffer_pos_ = 0;
    }
  }

  uint8_t cmd = x[0];
  uint16_t index = ((uint16_t)x[2] << 8) | x[1];
  uint8_t sub = x[3];

  // ---------- ABORT handling ----------
  if (cmd == 0x80) {
    uint32_t abort_code = 0;
    if (x.size() >= 8) {
      abort_code = ((uint32_t)x[7] << 24) | ((uint32_t)x[6] << 16) |
                   ((uint32_t)x[5] << 8) | x[4];
    }

    if (this->write_pending_) {
      const char *reason = "unknown error";
      if (abort_code == 0x06010000) reason = "access denied";
      else if (abort_code == 0x06010002) reason = "write-only object";
      else if (abort_code == 0x06020000) reason = "object does not exist";
      else if (abort_code == 0x06040043) reason = "parameter incompatibility";
      else if (abort_code == 0x06090030) reason = "value out of range";
      else if (abort_code == 0x06090031) reason = "value too high";
      else if (abort_code == 0x06090032) reason = "value too low";
      ESP_LOGW(TAG, "WRITE FAILED 0x%04X sub %d: %s (0x%08X)", index, sub, reason, abort_code);
      this->write_pending_ = false;
#ifdef USE_TEXT_SENSOR
      if (this->write_status_ != nullptr) {
        char status[64];
        snprintf(status, sizeof(status), "FAILED: %s", reason);
        this->write_status_->publish_state(status);
      }
#endif
      return;
    }

    if (abort_code == 0x06010000) {
      ESP_LOGW(TAG, "Access denied for 0x%04X sub %d, re-auth needed", index, sub);
      if (this->auth_step_ == 0)
        this->authenticated_ = false;
    } else if (abort_code == 0x06040043) {
      ESP_LOGW(TAG, "Auth rejected (param incompatibility), will retry");
      this->auth_step_ = 0;
      this->authenticated_ = false;
    } else {
      ESP_LOGD(TAG, "SDO ABORT 0x%04X sub %d code 0x%08X", index, sub, abort_code);
    }
    return;
  }

  // ---------- WRITE ACK handling (auth steps) ----------
  if (cmd == 0x60) {
    if (index == 0x4003 && sub == 0x03 && this->auth_step_ == 3) {
      ESP_LOGI(TAG, "Auth step3: sub3 ack, writing sub1=0x%08X", this->auth_sub1_);
      uint32_t s1 = this->auth_sub1_;
      uint8_t data[8] = {0x23, 0x03, 0x40, 0x01,
                         (uint8_t)(s1 & 0xFF), (uint8_t)((s1 >> 8) & 0xFF),
                         (uint8_t)((s1 >> 16) & 0xFF), (uint8_t)((s1 >> 24) & 0xFF)};
      this->send_can_(0x241, data, 8);
      this->auth_step_ = 4;
    } else if (index == 0x4003 && sub == 0x01 && this->auth_step_ == 4) {
      ESP_LOGI(TAG, "Auth step4: sub1 ack, writing sub2=0x%08X", this->auth_sub2_);
      uint32_t s2 = this->auth_sub2_;
      uint8_t data[8] = {0x23, 0x03, 0x40, 0x02,
                         (uint8_t)(s2 & 0xFF), (uint8_t)((s2 >> 8) & 0xFF),
                         (uint8_t)((s2 >> 16) & 0xFF), (uint8_t)((s2 >> 24) & 0xFF)};
      this->send_can_(0x241, data, 8);
      this->auth_step_ = 5;
    } else if (index == 0x4003 && sub == 0x02 && this->auth_step_ == 5) {
      ESP_LOGI(TAG, "Auth step5: sub2 ack, reading access level (0x4002)");
      uint8_t data[8] = {0x40, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
      this->send_can_(0x241, data, 8);
      this->auth_step_ = 6;
    } else {
      // Parameter write ACK
      if (this->write_pending_) {
        ESP_LOGI(TAG, "WRITE OK: 0x%04X sub %d", index, sub);
        this->write_pending_ = false;
#ifdef USE_TEXT_SENSOR
        if (this->write_status_ != nullptr)
          this->write_status_->publish_state("OK");
#endif
        // Read back the written parameter to confirm new value
        uint8_t rd[8] = {0x40, (uint8_t)(index & 0xFF), (uint8_t)(index >> 8), sub,
                         0x00, 0x00, 0x00, 0x00};
        this->send_can_(0x241, rd, 8);
      } else {
        ESP_LOGD(TAG, "Write ACK 0x%04X sub %d", index, sub);
      }
    }
    return;
  }

  // ---------- READ RESPONSE handling ----------
  if ((cmd & 0xE0) == 0x40) {
    uint32_t value = 0;
    for (int i = 0; i < 4 && (4 + i) < (int)x.size(); i++)
      value |= ((uint32_t)x[4 + i]) << (8 * i);

    // --- Auth state machine ---
    if (index == 0x2001 && sub == 0x0A && this->auth_step_ == 1) {
      this->auth_serial_ = value;
      ESP_LOGI(TAG, "Auth step1: serial=0x%08X, reading token...", value);
      uint8_t data[8] = {0x40, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
      this->send_can_(0x241, data, 8);
      this->auth_step_ = 2;
      return;
    }

    if (index == 0x4001 && sub == 0x00 && this->auth_step_ == 2) {
      uint32_t v[2] = {this->user_level_, this->user_level_};
      uint32_t k[4] = {0x15EFA43Fu, this->auth_serial_, value, this->user_level_};
      tea_encrypt_(v, k);
      this->auth_sub1_ = v[0];
      this->auth_sub2_ = v[1];
      ESP_LOGI(TAG, "Auth step2: token=0x%08X => sub1=0x%08X sub2=0x%08X", value, v[0], v[1]);
      uint8_t wd[8] = {0x2F, 0x03, 0x40, 0x03, (uint8_t)this->user_level_, 0x00, 0x00, 0x00};
      this->send_can_(0x241, wd, 8);
      this->auth_step_ = 3;
      return;
    }

    if (index == 0x4002 && sub == 0x00 && this->auth_step_ == 6) {
      if (value >= 1) {
        ESP_LOGI(TAG, "*** AUTHENTICATED *** (access level = %u)", value);
        this->authenticated_ = true;
        this->auth_step_ = 0;
        this->sdo_read_step_ = 0;
      } else {
        ESP_LOGW(TAG, "Auth FAILED: access level = %u", value);
        this->authenticated_ = false;
        this->auth_step_ = 0;
      }
      return;
    }

    // --- Data parsing for SDO reads ---
#ifdef USE_SENSOR
    if (index == 0x500F && sub == 0x00 && this->locking_mode_ != nullptr) {
      this->locking_mode_->publish_state(value);
    } else if (index == 0x5011 && sub == 0x00 && this->blocking_mode_ != nullptr) {
      this->blocking_mode_->publish_state(value);
    } else if (index == 0x1003 && sub == 0x01 && this->error_history_ != nullptr) {
      this->error_history_->publish_state(value);
    } else if (index == 0x2004 && sub == 0x01 && this->diagnostics_ != nullptr) {
      this->diagnostics_->publish_state(value);
    } else if (index == 0x502C && sub == 0x00 && this->appliance_type_ != nullptr) {
      this->appliance_type_->publish_state(value);
    } else if (index == 0x5037 && sub == 0x00 && this->appliance_variant_ != nullptr) {
      this->appliance_variant_->publish_state(value);
    } else
#endif
#ifdef USE_NUMBER
    if (index == 0x3451 && sub == 0x01 && this->cp510_setpoint_ != nullptr) {
      float temp = (value & 0xFFFF) * 0.1f;
      this->cp510_setpoint_->publish_state(temp);
      ESP_LOGD(TAG, "CP510 current=%.1f C (raw=%u)", temp, value & 0xFFFF);
#ifdef USE_CLIMATE
      if (this->climate_ != nullptr)
        this->climate_->update_target_temperature(temp);
#endif
    } else
#endif
    if (index == 0x3458 && sub == 0x01) {
      uint8_t program = value & 0xFF;
      ESP_LOGD(TAG, "Time program=%d", program);
#ifdef USE_SELECT
      if (this->time_program_ != nullptr) {
        const auto &options = this->time_program_->traits.get_options();
        int idx = program - 1;  // SDO values are 1-indexed
        if (idx >= 0 && idx < (int)options.size()) {
          this->time_program_->publish_state(options[idx]);
        }
      }
#endif
    } else
#ifdef USE_SELECT
    if (index == 0x341F && sub == 0x01 && this->zone_mode_ != nullptr) {
      uint8_t mode = value & 0xFF;
      const auto &options = this->zone_mode_->traits.get_options();
      if (mode < options.size()) {
        this->zone_mode_->publish_state(options[mode]);
      } else {
        ESP_LOGW(TAG, "Unknown zone mode: %d", mode);
      }
#ifdef USE_CLIMATE
      if (this->climate_ != nullptr)
        this->climate_->update_zone_mode(mode);
#endif
    } else
#endif
    // Trending string (0x501D) - segmented transfer
    if (index == 0x501D && sub == 0x00) {
      bool expedited = (cmd & 0x02) != 0;
      if (!expedited) {
        this->seg_read_active_ = true;
        this->seg_read_segment_ = 0;
        this->seg_read_start_ms_ = millis();
        this->seg_read_buffer_pos_ = 0;
        uint8_t req[8] = {0x60, 0, 0, 0, 0, 0, 0, 0};
        this->send_can_(0x241, req, 8);
        ESP_LOGD(TAG, "Segmented read of 0x501D started");
        return;
      }
    } else {
      ESP_LOGD(TAG, "SDO READ 0x%04X sub %d = 0x%08X (%u)", index, sub, value, value);
    }
  }
}

// --- Process buffered trending data (0x501D, 93 bytes) ---
void Remeha::process_trending_data_() {
  int len = this->seg_read_buffer_pos_;
  const uint8_t *d = this->seg_read_buffer_;

#ifdef USE_SENSOR
  // Water pressure: byte 22, single byte × 0.01
  if (len > 22 && this->water_pressure_ != nullptr) {
    float wp = d[22] * 0.01f;
    this->water_pressure_->publish_state(wp);
    ESP_LOGD(TAG, "Water pressure=%.2f bar (raw=%d)", wp, d[22]);
  }

  // Room temperature: byte 71 and 72, double byte × 0.01
  if (len > 72 && this->room_temperature_ != nullptr) {
    float room_temp = (uint16_t)(d[71] | (d[72] << 8)) * 0.1f;  // max 65535 × 0.1
    this->room_temperature_->publish_state(room_temp);
    ESP_LOGD(TAG, "Room temperature=%.1f C", room_temp);
  }

  // Room setpoint: read from CP510 via SDO poll, not from trending data
  // (bytes 27-28 in trending data is NOT room setpoint)
#endif

#ifdef USE_CLIMATE
  // Update climate entity with room temperature from byte 23 and 24
  if (this->climate_ != nullptr && len > 72) {
    float room_temp2 = (uint16_t)(d[71] | (d[72] << 8)) * 0.1f;  // max 65535 × 0.1
    if (room_temp2 > 0.0f && room_temp2 < 50.0f)
      this->climate_->update_current_temperature(room_temp2);
  }
#endif
}

// --- PDO handlers ---
void Remeha::handle_pdo_0x282_(const std::vector<uint8_t> &x) {
  if (x.size() < 4) return;
#ifdef USE_SENSOR
  if (this->relative_power_ != nullptr)
    this->relative_power_->publish_state(x[0]);
  float flow_temp = (((uint16_t)x[2] << 8) + x[3]) / 100.0f;
  if (this->flow_temperature_ != nullptr)
    this->flow_temperature_->publish_state(flow_temp);
#endif
}

void Remeha::handle_pdo_0x381_(const std::vector<uint8_t> &x) {
  if (x.size() < 6) return;
#ifdef USE_SENSOR
  float out_temp = int16_t(((uint16_t)x[1] << 8) + x[0]) / 100.0f;
  if (this->outside_temperature_ != nullptr)
    this->outside_temperature_->publish_state(out_temp);
  float out_temp_3m = int16_t(((uint16_t)x[3] << 8) + x[2]) / 100.0f;
  if (this->outside_temperature_3m_avg_ != nullptr)
    this->outside_temperature_3m_avg_->publish_state(out_temp_3m);
  float out_temp_2h = int16_t(((uint16_t)x[5] << 8) + x[4]) / 100.0f;
  if (this->outside_temperature_2h_avg_ != nullptr)
    this->outside_temperature_2h_avg_->publish_state(out_temp_2h);
#endif
}

void Remeha::handle_pdo_0x382_(const std::vector<uint8_t> &x) {
  if (x.size() < 4) return;
#ifdef USE_SENSOR
  float setpoint = (((uint16_t)x[2] << 8) + x[3]) / 100.0f;
  if (this->setpoint_ != nullptr)
    this->setpoint_->publish_state(setpoint);
#endif
}

void Remeha::handle_pdo_0x481_(const std::vector<uint8_t> &x) {
  if (x.size() < 2) return;
  uint8_t status = x[0];
  uint8_t substatus = x[1];

#ifdef USE_SENSOR
  if (this->status_code_ != nullptr)
    this->status_code_->publish_state(status);
  if (this->substatus_code_ != nullptr)
    this->substatus_code_->publish_state(substatus);
#endif

#ifdef USE_TEXT_SENSOR
  if (this->status_text_ != nullptr)
    this->status_text_->publish_state(get_status_text_(status));
  if (this->substatus_text_ != nullptr)
    this->substatus_text_->publish_state(get_substatus_text_(substatus));
#endif

#ifdef USE_CLIMATE
  if (this->climate_ != nullptr)
    this->climate_->update_action(status);
#endif
}

void Remeha::handle_pdo_0x482_(const std::vector<uint8_t> &x) {
  if (x.size() < 4) return;
#ifdef USE_SENSOR
  if (x[0] == 0x01 && x[1] == 0x03 && this->relative_power2_ != nullptr) {
    this->relative_power2_->publish_state(float(x[3]));
  }
#endif
}

const char *Remeha::get_status_text_(uint8_t status) {
  switch (status) {
    case 0:   return "Standby";
    case 1:   return "Heat request";
    case 2:   return "Burner ignition";
    case 3:   return "Heating mode";
    case 4:   return "DHW mode";
    case 5:   return "Burner off";
    case 6:   return "Pump active";
    case 8:   return "Burner shut down";
    case 9:   return "Temporary fault";
    case 10:  return "Permanent fault";
    case 11:  return "Test heat min";
    case 12:  return "Test heat max";
    case 13:  return "Test DHW max";
    case 15:  return "Manual heat";
    case 16:  return "Frost protection";
    case 17:  return "Bleed air";
    case 19:  return "Reset";
    case 20:  return "Automatic filling";
    case 22:  return "Forced calibration";
    case 200: return "Service mode";
    default:  return "Unknown";
  }
}

const char *Remeha::get_substatus_text_(uint8_t substatus) {
  switch (substatus) {
    case 0:  return "Standby";
    case 1:  return "Waiting ignition";
    case 4:  return "Waiting for temperature";
    case 13: return "Pre-ventilation";
    case 15: return "Ignition signal sent";
    case 17: return "Burner pre-ignition";
    case 18: return "Burner ignition";
    case 19: return "Flame check";
    case 20: return "Fan operation at ign";
    case 30: return "Operation at setpoint";
    case 31: return "Operation reduced setpoint";
    case 32: return "Operation required setpoint";
    case 33: return "Level1 ramp";
    case 34: return "Level2 ramp";
    case 35: return "Level3 ramp";
    case 36: return "Flame protection";
    case 37: return "Stabilization time";
    case 38: return "Start min output";
    case 39: return "Heating mode interrupted by DHW";
    case 41: return "Post-ventilation";
    case 44: return "Fan off";
    case 45: return "Power reduction";
    case 46: return "Automatic filling: empty";
    case 47: return "Automatic filling: low";
    case 60: return "Pump post-ventilation";
    case 95: return "Standby due water pressure";
    default: return "Unknown";
  }
}

}  // namespace remeha
}  // namespace esphome
