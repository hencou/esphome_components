#include "remeha.h"
#include <algorithm>
#ifdef USE_CLIMATE
#include "climate/remeha_climate.h"
#endif
#ifdef USE_SELECT
#include "select/remeha_select.h"
#endif
#ifdef USE_NUMBER
#include "number/remeha_number.h"
#endif
#include "esphome/core/log.h"
#ifdef USE_ESP32
#include <driver/twai.h>
#endif

namespace esphome {
namespace remeha {

// A full round of SDO reads is sent back to back, the next request following the
// previous response. Between rounds single reads keep the channel from going
// idle, which the boiler answers for as long as the authorisation lives.
static const uint32_t SDO_CYCLE_INTERVAL_MS = 60000;
static const uint32_t SDO_READ_GAP_MS = 20;
static const uint32_t SDO_READ_TIMEOUT_MS = 2000;
static const uint32_t SDO_KEEPALIVE_MS = 10000;
static const uint32_t ERROR_LOG_INTERVAL_MS = 900000;
// The boiler keeps up to 32 entries, but only the entries we expose are read.
static const uint8_t ERROR_LOG_MAX_ENTRIES = ERROR_LOG_SLOTS;
static const uint32_t BUS_CHECK_INTERVAL_MS = 1000;

// A transmitter that is error passive and gets no acknowledgement does not
// increase its error counter any further, so the controller never reaches
// bus-off: it keeps retrying the same frame and the TX queue stays full.
static const uint32_t TX_STALL_TIMEOUT_MS = 15000;

void Remeha::setup() {
  this->boot_time_ms_ = millis();
  this->boot_phase_ = 0;

  // Register CAN frame callback
  this->canbus_->add_callback([this](uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                                     const std::vector<uint8_t> &data) {
    this->handle_frame_(can_id, use_extended_id, remote_transmission_request, data);
  });

  ESP_LOGI(TAG, "Remeha component initialized, boot delay %u ms, user level %u",
           (unsigned) this->boot_delay_ms_, this->user_level_);
}

void Remeha::loop() {
  uint32_t now = millis();

  if (now - this->last_bus_check_ms_ >= BUS_CHECK_INTERVAL_MS) {
    this->last_bus_check_ms_ = now;
    this->service_bus_recovery_();
  }

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

  // --- Every 10 seconds: timeouts + gateway check + auth ---
  if (now - this->last_poll_ms_ >= 10000) {
    this->last_poll_ms_ = now;

    // Auth timeout
    if (this->auth_step_ > 0 && (now - this->auth_start_ms_) > 10000) {
      ESP_LOGW(TAG, "Auth timeout at step %d, resetting", this->auth_step_);
      this->auth_step_ = 0;
      this->authenticated_ = false;
      this->effective_level_ = 0;
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
    if (this->auth_required_() && !this->authenticated_ && this->auth_step_ == 0) {
      this->start_auth_();
      return;
    }
  }

  this->service_sdo_polling_(now);
}

void Remeha::dump_config() {
  ESP_LOGCONFIG(TAG, "Remeha:");
  ESP_LOGCONFIG(TAG, "  Boot delay: %u ms", (unsigned) this->boot_delay_ms_);
  ESP_LOGCONFIG(TAG, "  User level: %u", this->user_level_);
  ESP_LOGCONFIG(TAG, "  Auth key: %s", this->auth_key_ != 0 ? "set" : "not set");
  ESP_LOGCONFIG(TAG, "  Minimum level for writes: %u", this->min_write_level_);
  ESP_LOGCONFIG(TAG, "  SDO poll entries: %u", (unsigned) this->sdo_poll_list_.size());
  ESP_LOGCONFIG(TAG, "  SDO channel: %u (0x%03X/0x%03X)", this->sdo_channel_, (unsigned) this->sdo_tx_id_,
                (unsigned) this->sdo_rx_id_);
}

// The TWAI controller stops transmitting once it goes bus-off and stays there
// until recovery is requested and the driver is started again.
void Remeha::service_bus_recovery_() {
#ifdef USE_ESP32
  twai_status_info_t status;
  if (twai_get_status_info(&status) != ESP_OK)
    return;

  if (status.state == TWAI_STATE_BUS_OFF) {
    ESP_LOGW(TAG, "CAN bus-off detected, starting recovery");
    twai_initiate_recovery();
    this->bus_recovering_ = true;
  } else if (status.state == TWAI_STATE_STOPPED && this->bus_recovering_) {
    if (twai_start() != ESP_OK) {
      ESP_LOGW(TAG, "CAN restart after recovery failed");
      return;
    }
    ESP_LOGI(TAG, "CAN bus recovered, restarting boot sequence");
    this->bus_recovering_ = false;
    this->restart_bus_session_();
  } else if (status.state == TWAI_STATE_RUNNING) {
    this->bus_recovering_ = false;

    bool stalled = status.msgs_to_tx > 0 && status.tx_error_counter >= 128;
    if (!stalled) {
      this->tx_stall_since_ms_ = 0;
      return;
    }

    uint32_t now = millis();
    if (this->tx_stall_since_ms_ == 0) {
      this->tx_stall_since_ms_ = now;
      return;
    }
    if (now - this->tx_stall_since_ms_ < TX_STALL_TIMEOUT_MS)
      return;

    ESP_LOGW(TAG, "CAN transmit stalled (TEC %u, %u queued), restarting controller",
             (unsigned) status.tx_error_counter, (unsigned) status.msgs_to_tx);
    this->tx_stall_since_ms_ = 0;
    // Stopping and starting aborts the pending frame and clears the error
    // counters, which is what a reboot of the module does today.
    if (twai_stop() != ESP_OK || twai_start() != ESP_OK) {
      ESP_LOGW(TAG, "CAN controller restart failed");
      return;
    }
    this->restart_bus_session_();
  }
#endif
}

void Remeha::restart_bus_session_() {
  this->gateway_enabled_ = false;
  this->authenticated_ = false;
  this->effective_level_ = 0;
  this->auth_step_ = 0;
  this->sdo_pending_ = false;
  this->sdo_cycle_active_ = false;
  this->sdo_cycle_start_ms_ = 0;
  this->write_pending_ = false;
  this->seg_read_active_ = false;
  this->seg_read_segment_ = 0;
  this->seg_read_buffer_pos_ = 0;
  this->boot_phase_ = 0;
  this->boot_time_ms_ = millis();
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
  this->send_can_(this->sdo_tx_id_, rd, 8);
  this->auth_step_ = 1;
  this->auth_start_ms_ = millis();
}

void Remeha::set_sdo_channel_(uint8_t channel) {
  this->sdo_channel_ = channel;
  this->sdo_tx_id_ = 0x141 + 0x100 * (uint32_t) channel;
  this->sdo_rx_id_ = 0x0C1 + 0x100 * (uint32_t) channel;
}

void Remeha::send_sdo_read_object_(uint16_t index, uint8_t subindex) {
  uint8_t data[8] = {0x40, (uint8_t)(index & 0xFF), (uint8_t)(index >> 8), subindex,
                     0x00, 0x00, 0x00, 0x00};
  this->send_can_(this->sdo_tx_id_, data, 8);
  this->sdo_pending_ = true;
  this->sdo_pending_index_ = index;
  this->sdo_pending_sub_ = subindex;
  this->sdo_sent_ms_ = millis();
}

void Remeha::send_sdo_read_(size_t entry) {
  this->send_sdo_read_object_(this->sdo_poll_list_[entry].index, this->sdo_poll_list_[entry].subindex);
}

// Walks the poll list one entry at a time, advancing as soon as the previous
// response arrives, so a full round takes under a second instead of one read
// per tick.
void Remeha::service_sdo_polling_(uint32_t now) {
  if (this->sdo_poll_list_.empty() || !this->gateway_enabled_ || this->auth_step_ != 0)
    return;
  if (this->auth_required_() && !this->authenticated_)
    return;

  if (this->sdo_pending_) {
    if ((now - this->sdo_sent_ms_) < SDO_READ_TIMEOUT_MS)
      return;
    ESP_LOGD(TAG, "No SDO response for 0x%04X sub %d, skipping", this->sdo_pending_index_,
             this->sdo_pending_sub_);
    this->sdo_pending_ = false;
    if (this->error_log_active_) {
      ESP_LOGD(TAG, "Error history read timed out, aborting this round");
      this->error_log_active_ = false;
      this->error_log_last_ms_ = now;
    }
  }

  if (this->write_pending_ || this->seg_read_active_)
    return;

  this->service_error_log_(now);
  if (this->error_log_active_)
    return;

  if (this->sdo_cycle_active_) {
    if (this->sdo_read_step_ >= this->sdo_poll_list_.size()) {
      ESP_LOGD(TAG, "SDO poll cycle complete (%u entries)", (unsigned) this->sdo_poll_list_.size());
      this->sdo_cycle_active_ = false;
      return;
    }
    if ((now - this->sdo_sent_ms_) < SDO_READ_GAP_MS)
      return;
    this->send_sdo_read_(this->sdo_read_step_);
    this->sdo_read_step_++;
    return;
  }

  if (this->sdo_cycle_start_ms_ == 0 || (now - this->sdo_cycle_start_ms_) >= SDO_CYCLE_INTERVAL_MS) {
    this->sdo_cycle_active_ = true;
    this->sdo_cycle_start_ms_ = now;
    this->sdo_read_step_ = 0;
    return;
  }

  if ((now - this->sdo_sent_ms_) >= SDO_KEEPALIVE_MS) {
    this->sdo_keepalive_step_ = this->sdo_keepalive_step_ % this->sdo_poll_list_.size();
    this->send_sdo_read_(this->sdo_keepalive_step_);
    this->sdo_keepalive_step_++;
  }
}

// Reads the error history arrays object by object: 0x1003:00 holds the number
// of entries, 0x1003:n a two byte {code, category} struct and 0x2004:n the
// matching customer code.
void Remeha::service_error_log_(uint32_t now) {
  if (!this->error_log_enabled_)
    return;

  if (!this->error_log_active_) {
    if (this->error_log_last_ms_ != 0 && (now - this->error_log_last_ms_) < ERROR_LOG_INTERVAL_MS)
      return;
    if (this->sdo_cycle_active_)
      return;
    this->error_log_active_ = true;
    this->error_log_customer_phase_ = false;
    this->error_log_step_ = 0;
    this->error_log_count_ = 0;
    this->error_log_entries_.clear();
    this->error_log_customer_codes_.clear();
    this->send_sdo_read_object_(0x1003, 0x00);
    return;
  }

  if (this->sdo_pending_ || (now - this->sdo_sent_ms_) < SDO_READ_GAP_MS)
    return;

  uint16_t index = this->error_log_customer_phase_ ? 0x2004 : 0x1003;
  this->send_sdo_read_object_(index, this->error_log_step_);
}

bool Remeha::handle_error_log_response_(uint16_t index, uint8_t sub, uint32_t value) {
  if (index == 0x1003 && sub == 0x00) {
    this->error_log_count_ = (uint8_t) std::min<uint32_t>(value & 0xFF, ERROR_LOG_MAX_ENTRIES);
    if (this->error_log_count_ == 0) {
      this->publish_error_log_();
      return true;
    }
    this->error_log_step_ = 1;
    return true;
  }

  if (index == 0x1003 && sub >= 1 && sub <= this->error_log_count_) {
    this->error_log_entries_.push_back((uint16_t)(value & 0xFFFF));
    if (sub >= this->error_log_count_) {
      this->error_log_customer_phase_ = true;
      this->error_log_step_ = 1;
    } else {
      this->error_log_step_ = sub + 1;
    }
    return true;
  }

  if (index == 0x2004 && sub >= 1 && sub <= this->error_log_count_) {
    this->error_log_customer_codes_.push_back(value);
    if (sub >= this->error_log_count_) {
      this->publish_error_log_();
    } else {
      this->error_log_step_ = sub + 1;
    }
    return true;
  }

  return false;
}

void Remeha::publish_error_log_() {
  this->error_log_active_ = false;
  this->error_log_customer_phase_ = false;
  this->error_log_last_ms_ = millis();

  ESP_LOGD(TAG, "Error history: %u entries", this->error_log_count_);
#ifdef USE_TEXT_SENSOR
  for (int slot = 0; slot < ERROR_LOG_SLOTS; slot++) {
    if (this->error_slots_[slot] == nullptr)
      continue;
    this->error_slots_[slot]->publish_state(this->format_error_entry_((size_t) slot, false));
  }
  if (this->last_error_ != nullptr)
    this->last_error_->publish_state(this->format_error_entry_(0, true));
#endif
}

// Entry 0 is the most recent error. A verbose entry spells out the customer
// code, a compact one keeps it between parentheses so it fits a table cell.
std::string Remeha::format_error_entry_(size_t i, bool verbose) {
  if (i >= this->error_log_entries_.size())
    return verbose ? "No errors" : "-";

  uint8_t code = this->error_log_entries_[i] & 0xFF;
  uint8_t category = (this->error_log_entries_[i] >> 8) & 0xFF;
  char buf[48];
  if (i < this->error_log_customer_codes_.size()) {
    snprintf(buf, sizeof(buf), verbose ? "E:%02u.%02u (customer code %u)" : "E:%02u.%02u (%u)", category, code,
             (unsigned) this->error_log_customer_codes_[i]);
  } else {
    snprintf(buf, sizeof(buf), "E:%02u.%02u", category, code);
  }
  return buf;
}

void Remeha::report_write_rejected(const char *reason) {
  ESP_LOGW(TAG, "Write rejected: %s", reason);
#ifdef USE_TEXT_SENSOR
  if (this->write_status_ != nullptr) {
    char status[64];
    snprintf(status, sizeof(status), "REJECTED: %s", reason);
    this->write_status_->publish_state(status);
  }
#endif
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
  if (this->effective_level_ < this->min_write_level_) {
    ESP_LOGW(TAG, "Cannot write: access level %u is below the required level %u",
             this->effective_level_, this->min_write_level_);
#ifdef USE_TEXT_SENSOR
    if (this->write_status_ != nullptr)
      this->write_status_->publish_state("Access level too low");
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

  // The boiler aborts a value that does not fit the object, but a truncated
  // frame would silently write a different value, so check it here first.
  int32_t as_signed = (int32_t) value;
  if (size == 1 && (value > 0xFF && (as_signed < -128 || as_signed > 127))) {
    this->report_write_rejected("value does not fit in 1 byte");
    return false;
  }
  if (size == 2 && (value > 0xFFFF && (as_signed < -32768 || as_signed > 32767))) {
    this->report_write_rejected("value does not fit in 2 bytes");
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
  this->send_can_(this->sdo_tx_id_, data, 8);
  this->write_pending_ = true;
  this->write_start_ms_ = millis();

  ESP_LOGI(TAG, "WRITE SDO 0x%04X sub %d = %u (cmd=0x%02X)", index, subindex, (unsigned) value, cmd);

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
  if (can_id == this->sdo_rx_id_) {
    this->handle_0x1c1_(data);
    return;
  }

  switch (can_id) {
    case 0x581: this->handle_0x581_(data); break;
    case 0x282: this->handle_pdo_0x282_(data); break;
    case 0x381: this->handle_pdo_0x381_(data); break;
    case 0x382: this->handle_pdo_0x382_(data); break;
    case 0x481: this->handle_pdo_0x481_(data); break;
    case 0x482: this->handle_pdo_0x482_(data); break;
    default: break;
  }
}

// --- Standard SDO response (0x581): channel assignment ---
void Remeha::handle_0x581_(const std::vector<uint8_t> &x) {
  if (x.size() < 4) return;
  uint8_t cmd = x[0];
  uint16_t index = ((uint16_t)x[2] << 8) | x[1];
  uint8_t sub = x[3];

  if (index == 0x4004 && sub == 0x00) {
    if ((cmd == 0x4F || cmd == 0x4B || cmd == 0x43) && x.size() > 4 && x[4] > 0) {
      if (!this->gateway_enabled_ || x[4] != this->sdo_channel_) {
        this->set_sdo_channel_(x[4]);
        ESP_LOGI(TAG, "SDO channel %u assigned, using 0x%03X/0x%03X", this->sdo_channel_,
                 (unsigned) this->sdo_tx_id_, (unsigned) this->sdo_rx_id_);
        this->gateway_enabled_ = true;
        this->authenticated_ = false;
        this->effective_level_ = 0;
        this->auth_step_ = 0;
        if (this->auth_required_()) {
          this->start_auth_();
        }
      }
    } else if (cmd == 0x60) {
      if (!this->gateway_enabled_) {
        this->set_sdo_channel_(1);
        ESP_LOGI(TAG, "Write to 0x4004 confirmed, using channel 1 (0x%03X/0x%03X)",
                 (unsigned) this->sdo_tx_id_, (unsigned) this->sdo_rx_id_);
        this->gateway_enabled_ = true;
        if (this->auth_required_() && !this->authenticated_ && this->auth_step_ == 0) {
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

// --- Custom SDO channel response: auth + data parsing ---
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
        if (this->seg_read_buffer_pos_ < 100) {
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
        this->send_can_(this->sdo_tx_id_, req, 8);
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

  if (this->sdo_pending_ && index == this->sdo_pending_index_ && sub == this->sdo_pending_sub_) {
    this->sdo_pending_ = false;
  }

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
      ESP_LOGW(TAG, "WRITE FAILED 0x%04X sub %d: %s (0x%08X)", index, sub, reason,
               (unsigned) abort_code);
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
      if (this->auth_step_ == 0) {
        this->authenticated_ = false;
        this->effective_level_ = 0;
      }
    } else if (abort_code == 0x06040043) {
      ESP_LOGW(TAG, "Auth rejected (param incompatibility), will retry");
      this->auth_step_ = 0;
      this->authenticated_ = false;
      this->effective_level_ = 0;
    } else {
      ESP_LOGD(TAG, "SDO ABORT 0x%04X sub %d code 0x%08X", index, sub, (unsigned) abort_code);
    }
    return;
  }

  // ---------- WRITE ACK handling (auth steps) ----------
  if (cmd == 0x60) {
    if (index == 0x4003 && sub == 0x03 && this->auth_step_ == 3) {
      ESP_LOGI(TAG, "Auth step3: sub3 ack, writing sub1");
      uint32_t s1 = this->auth_sub1_;
      uint8_t data[8] = {0x23, 0x03, 0x40, 0x01,
                         (uint8_t)(s1 & 0xFF), (uint8_t)((s1 >> 8) & 0xFF),
                         (uint8_t)((s1 >> 16) & 0xFF), (uint8_t)((s1 >> 24) & 0xFF)};
      this->send_can_(this->sdo_tx_id_, data, 8);
      this->auth_step_ = 4;
    } else if (index == 0x4003 && sub == 0x01 && this->auth_step_ == 4) {
      ESP_LOGI(TAG, "Auth step4: sub1 ack, writing sub2");
      uint32_t s2 = this->auth_sub2_;
      uint8_t data[8] = {0x23, 0x03, 0x40, 0x02,
                         (uint8_t)(s2 & 0xFF), (uint8_t)((s2 >> 8) & 0xFF),
                         (uint8_t)((s2 >> 16) & 0xFF), (uint8_t)((s2 >> 24) & 0xFF)};
      this->send_can_(this->sdo_tx_id_, data, 8);
      this->auth_step_ = 5;
    } else if (index == 0x4003 && sub == 0x02 && this->auth_step_ == 5) {
      ESP_LOGI(TAG, "Auth step5: sub2 ack, reading access level (0x4002)");
      uint8_t data[8] = {0x40, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
      this->send_can_(this->sdo_tx_id_, data, 8);
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
        this->send_can_(this->sdo_tx_id_, rd, 8);
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
      ESP_LOGI(TAG, "Auth step1: serial received, reading token...");
      ESP_LOGV(TAG, "Auth step1: serial=0x%08X", (unsigned) value);
      uint8_t data[8] = {0x40, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};
      this->send_can_(this->sdo_tx_id_, data, 8);
      this->auth_step_ = 2;
      return;
    }

    if (index == 0x4001 && sub == 0x00 && this->auth_step_ == 2) {
      uint32_t v[2] = {this->user_level_, this->user_level_};
      uint32_t k[4] = {this->auth_key_, this->auth_serial_, value, this->user_level_};
      tea_encrypt_(v, k);
      this->auth_sub1_ = v[0];
      this->auth_sub2_ = v[1];
      ESP_LOGI(TAG, "Auth step2: token received, writing response...");
      ESP_LOGV(TAG, "Auth step2: token=0x%08X => sub1=0x%08X sub2=0x%08X", (unsigned) value,
               (unsigned) v[0], (unsigned) v[1]);
      uint8_t wd[8] = {0x2F, 0x03, 0x40, 0x03, (uint8_t)this->user_level_, 0x00, 0x00, 0x00};
      this->send_can_(this->sdo_tx_id_, wd, 8);
      this->auth_step_ = 3;
      return;
    }

    if (index == 0x4002 && sub == 0x00 && this->auth_step_ == 6) {
      if (value == this->user_level_) {
        ESP_LOGI(TAG, "*** AUTHENTICATED *** (access level = %u)", (unsigned) value);
        this->authenticated_ = true;
        this->effective_level_ = (uint8_t) value;
        this->auth_step_ = 0;
        this->sdo_read_step_ = 0;
      } else {
        ESP_LOGW(TAG, "Auth FAILED: requested level %u, effective level %u", this->user_level_,
                 (unsigned) value);
        this->authenticated_ = false;
        this->effective_level_ = 0;
        this->auth_step_ = 0;
      }
      return;
    }

    if (this->error_log_active_ && this->handle_error_log_response_(index, sub, value))
      return;

    // --- Data parsing for SDO reads ---
#ifdef USE_SENSOR
    if (index == 0x500F && sub == 0x00 && this->locking_mode_ != nullptr) {
      this->locking_mode_->publish_state(value);
    } else if (index == 0x5011 && sub == 0x00 && this->blocking_mode_ != nullptr) {
      this->blocking_mode_->publish_state(value);
    } else if (index == 0x502C && sub == 0x00 && this->appliance_type_ != nullptr) {
      this->appliance_type_->publish_state(value);
    } else if (index == 0x5037 && sub == 0x00 && this->appliance_variant_ != nullptr) {
      this->appliance_variant_->publish_state(value);
    } else
#endif
#ifdef USE_NUMBER
    if (this->room_setpoint_ != nullptr && this->room_setpoint_->matches(index, sub)) {
      this->room_setpoint_->publish_from_sdo(value);
    } else if (this->dhw_comfort_setpoint_ != nullptr && this->dhw_comfort_setpoint_->matches(index, sub)) {
      this->dhw_comfort_setpoint_->publish_from_sdo(value);
    } else if (this->dhw_reduced_setpoint_ != nullptr && this->dhw_reduced_setpoint_->matches(index, sub)) {
      this->dhw_reduced_setpoint_->publish_from_sdo(value);
    } else if (this->night_setpoint_ != nullptr && this->night_setpoint_->matches(index, sub)) {
      this->night_setpoint_->publish_from_sdo(value);
    } else if (this->holiday_setpoint_ != nullptr && this->holiday_setpoint_->matches(index, sub)) {
      this->holiday_setpoint_->publish_from_sdo(value);
    } else if (this->summer_winter_threshold_ != nullptr && this->summer_winter_threshold_->matches(index, sub)) {
      this->summer_winter_threshold_->publish_from_sdo(value);
    } else if (this->heating_curve_slope_ != nullptr && this->heating_curve_slope_->matches(index, sub)) {
      this->heating_curve_slope_->publish_from_sdo(value);
    } else if (this->room_sensor_calibration_ != nullptr && this->room_sensor_calibration_->matches(index, sub)) {
      this->room_sensor_calibration_->publish_from_sdo(value);
    } else if (this->anti_legionella_setpoint_ != nullptr && this->anti_legionella_setpoint_->matches(index, sub)) {
      this->anti_legionella_setpoint_->publish_from_sdo(value);
    } else
#endif
#ifdef USE_SELECT
    if (this->ch_enabled_ != nullptr && this->ch_enabled_->matches(index, sub)) {
      uint8_t val = value & 0xFF;
      this->ch_enabled_->publish_from_sdo(val);
      ESP_LOGD(TAG, "CH enabled=%d", val);
    } else if (this->dhw_enabled_ != nullptr && this->dhw_enabled_->matches(index, sub)) {
      uint8_t val = value & 0xFF;
      this->dhw_enabled_->publish_from_sdo(val);
      ESP_LOGD(TAG, "DHW enabled=%d", val);
    } else if (this->anti_legionella_mode_ != nullptr && this->anti_legionella_mode_->matches(index, sub)) {
      uint8_t val = value & 0xFF;
      this->anti_legionella_mode_->publish_from_sdo(val);
      ESP_LOGD(TAG, "Anti-legionella mode=%d", val);
    } else if (this->fireplace_mode_ != nullptr && this->fireplace_mode_->matches(index, sub)) {
      uint8_t val = value & 0xFF;
      this->fireplace_mode_->publish_from_sdo(val);
      ESP_LOGD(TAG, "Fireplace mode=%d", val);
    } else
#endif
    if (index == 0x3458) {
      uint8_t program = value & 0xFF;
      ESP_LOGD(TAG, "Time program=%d", program);
#ifdef USE_SELECT
      if (this->time_program_ != nullptr) {
        this->time_program_->publish_from_sdo(program);
      }
#endif
#ifdef USE_CLIMATE
      if (this->climate_ != nullptr)
        this->climate_->update_time_program(program);
#endif
    } else
#ifdef USE_SELECT
    if (this->zone_mode_ != nullptr && this->zone_mode_->matches(index, sub)) {
      uint8_t mode = value & 0xFF;
      this->zone_mode_->publish_from_sdo(mode);
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
        this->send_can_(this->sdo_tx_id_, req, 8);
        ESP_LOGD(TAG, "Segmented read of 0x501D started");
        return;
      }
    } else {
      ESP_LOGD(TAG, "SDO READ 0x%04X sub %d = 0x%08X (%u)", index, sub, (unsigned) value,
               (unsigned) value);
    }
  }
}

// --- Process buffered trending data (0x501D, 98 bytes) ---
void Remeha::process_trending_data_() {
  int len = this->seg_read_buffer_pos_;
  const uint8_t *d = this->seg_read_buffer_;

#ifdef USE_SENSOR
  // Boiler temperature (T aanvoer from trending): bytes 10-11, int16 LE × 0.01
  if (len > 11 && this->boiler_temperature_ != nullptr) {
    float val = (int16_t)(d[10] | (d[11] << 8)) * 0.01f;
    this->boiler_temperature_->publish_state(val);
    ESP_LOGD(TAG, "Boiler temperature=%.2f C", val);
  }

  // Flue gas temperature (Rookgastemperatuur): bytes 12-13, int16 LE × 0.01
  if (len > 13 && this->flue_gas_temperature_ != nullptr) {
    float val = (int16_t)(d[12] | (d[13] << 8)) * 0.01f;
    this->flue_gas_temperature_->publish_state(val);
    ESP_LOGD(TAG, "Flue gas temperature=%.2f C", val);
  }

  // Return temperature (T retour): bytes 14-15, int16 LE × 0.01
  if (len > 15 && this->return_temperature_ != nullptr) {
    float val = (int16_t)(d[14] | (d[15] << 8)) * 0.01f;
    this->return_temperature_->publish_state(val);
    ESP_LOGD(TAG, "Return temperature=%.2f C", val);
  }

  // DHW temperature (SWW tank temp): bytes 16-17, int16 LE × 0.01
  if (len > 17 && this->dhw_temperature_ != nullptr) {
    float val = (int16_t)(d[16] | (d[17] << 8)) * 0.01f;
    this->dhw_temperature_->publish_state(val);
    ESP_LOGD(TAG, "DHW temperature=%.2f C", val);
  }

  // Control temperature (Regeltemperatuur): bytes 20-21, int16 LE × 0.01
  if (len > 21 && this->control_temperature_ != nullptr) {
    float val = (int16_t)(d[20] | (d[21] << 8)) * 0.01f;
    this->control_temperature_->publish_state(val);
    ESP_LOGD(TAG, "Control temperature=%.2f C", val);
  }

  // Internal setpoint (Intern setpunt): bytes 22-23, int16 LE × 0.01
  if (len > 23 && this->internal_setpoint_ != nullptr) {
    float val = (int16_t)(d[22] | (d[23] << 8)) * 0.01f;
    this->internal_setpoint_->publish_state(val);
    ESP_LOGD(TAG, "Internal setpoint=%.2f C", val);
  }

  // Outside temperature boiler (Tbuiten): bytes 26-27, int16 LE × 0.01
  if (len > 27 && this->outside_temp_boiler_ != nullptr) {
    float val = (int16_t)(d[26] | (d[27] << 8)) * 0.01f;
    this->outside_temp_boiler_->publish_state(val);
    ESP_LOGD(TAG, "Outside temp boiler=%.2f C", val);
  }

  // Actual modulation (Act. rel.): bytes 36-37, uint16 LE × 0.01
  if (len > 37 && this->actual_modulation_ != nullptr) {
    float val = (uint16_t)(d[36] | (d[37] << 8)) * 0.01f;
    this->actual_modulation_->publish_state(val);
    ESP_LOGD(TAG, "Actual modulation=%.2f %%", val);
  }

  // Flame current (Vlamstroom): byte 39, uint8 × 0.1
  if (len > 39 && this->flame_current_ != nullptr) {
    float val = d[39] * 0.1f;
    this->flame_current_->publish_state(val);
    ESP_LOGD(TAG, "Flame current=%.1f uA", val);
  }

  // Pump speed (Pomptoerental): bytes 44-45, uint16 LE × 0.1
  if (len > 45 && this->pump_speed_ != nullptr) {
    float val = (uint16_t)(d[44] | (d[45] << 8)) * 0.1f;
    this->pump_speed_->publish_state(val);
    ESP_LOGD(TAG, "Pump speed=%.1f %%", val);
  }

  // Water pressure: byte 49, single byte × 0.1
  if (len > 49 && this->water_pressure_ != nullptr) {
    float wp = d[49] * 0.1f;
    this->water_pressure_->publish_state(wp);
    ESP_LOGD(TAG, "Water pressure=%.1f bar (raw=%d)", wp, d[49]);
  }

  // Room temperature (Truimte groep): bytes 71-72, uint16 LE × 0.1
  if (len > 72 && this->room_temperature_ != nullptr) {
    float room_temp = (uint16_t)(d[71] | (d[72] << 8)) * 0.1f;
    this->room_temperature_->publish_state(room_temp);
    ESP_LOGD(TAG, "Room temperature=%.1f C", room_temp);
  }

  // Calculated temperature (Berekende): bytes 79-80, int16 LE × 0.01
  if (len > 80 && this->calculated_temperature_ != nullptr) {
    float val = (int16_t)(d[79] | (d[80] << 8)) * 0.01f;
    this->calculated_temperature_->publish_state(val);
    ESP_LOGD(TAG, "Calculated temperature=%.2f C", val);
  }
#endif

#ifdef USE_CLIMATE
  // Update climate entity with current room temperature from bytes 71-72
  if (this->climate_ != nullptr && len > 72) {
    float room_temp2 = (uint16_t)(d[71] | (d[72] << 8)) * 0.1f;
    if (room_temp2 > 0.0f && room_temp2 < 50.0f)
      this->climate_->update_current_temperature(room_temp2);
  }
  // Update climate entity with active setpoint from bytes 75-76 (varZoneTRoomSetpoint)
  if (this->climate_ != nullptr && len > 76) {
    float active_setpoint = (uint16_t)(d[75] | (d[76] << 8)) * 0.1f;
    if (active_setpoint > 0.0f && active_setpoint < 50.0f)
      this->climate_->update_target_temperature(active_setpoint);
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
    case 1:   return "Heat Demand";
    case 2:   return "Generator start";
    case 3:   return "Generator CH";
    case 4:   return "Generator DHW";
    case 5:   return "Generator stop";
    case 6:   return "Pump Post Run";
    case 7:   return "Cooling Active";
    case 8:   return "Controlled Stop";
    case 9:   return "Blocking Mode";
    case 10:  return "Locking Mode";
    case 11:  return "Load test min";
    case 12:  return "Load test CH max";
    case 13:  return "Load test DHW max";
    case 14:  return "Load test custom";
    case 15:  return "Manual Heat Demand";
    case 16:  return "Frost Protection";
    case 17:  return "Deaeration";
    case 18:  return "Control unit Cooling";
    case 19:  return "Reset In Progress";
    case 20:  return "Auto Filling";
    case 21:  return "Halted";
    case 22:  return "Forced calibration";
    case 23:  return "Factory test";
    case 24:  return "Hydronic balancing";
    case 200: return "Device Mode";
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
