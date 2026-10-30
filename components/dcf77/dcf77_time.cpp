#include "dcf77_time.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome::dcf77 {

static const char *const TAG = "dcf77.time";

void IRAM_ATTR DCF77EdgeStore::append_signal(DCF77EdgeStore *arg, uint8_t signal) {
  arg->running_buffer |= (static_cast<uint64_t>(signal) << arg->buffer_position);
  arg->buffer_position++;
  if (arg->buffer_position > 59) {
    // Buffer filled up before an end-of-minute gap was ever seen -- this
    // indicates noise rather than a real frame; discard it.
    finalize_buffer(arg);
  }
}

void IRAM_ATTR DCF77EdgeStore::finalize_buffer(DCF77EdgeStore *arg) {
  if (arg->buffer_position == 59) {
    arg->filled_buffer = arg->running_buffer;
    arg->filled_buffer_available = true;
  }
  arg->running_buffer = 0;
  arg->buffer_position = 0;
}

void IRAM_ATTR DCF77EdgeStore::gpio_intr(DCF77EdgeStore *arg) {
  const uint32_t now = millis();
  const bool level = arg->pin.digital_read();

  // Noise rejection -- unchanged from the original library. Unsigned
  // subtraction wraps correctly even across a millis() rollover.
  if ((now - arg->previous_leading_edge) < REJECTION_TIME_MS) {
    return;
  }
  if ((now - arg->leading_edge) < REJECT_PULSE_WIDTH_MS) {
    return;
  }

  if (level) {
    // Rising (active) edge.
    if (!arg->up) {
      arg->leading_edge = now;
      arg->up = true;
    }
  } else {
    // Falling edge.
    if (arg->up) {
      const uint32_t trailing_edge = now;
      const uint32_t pulse_width = trailing_edge - arg->leading_edge;

      if ((arg->leading_edge - arg->previous_leading_edge) > SYNC_TIME_MS) {
        // Gap since the previous pulse indicates the start of a new minute.
        finalize_buffer(arg);
      }
      arg->previous_leading_edge = arg->leading_edge;

      // DCF77 encodes bit 0 as a ~100ms pulse and bit 1 as a ~200ms pulse.
      append_signal(arg, pulse_width < SPLIT_TIME_MS ? 0 : 1);
      arg->up = false;
    }
  }
}

void DCF77Time::setup() {
  this->pin_->setup();
  this->store_.pin = this->pin_->to_isr();
  this->pin_->attach_interrupt(DCF77EdgeStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
}

void DCF77Time::loop() {
  if (!this->store_.filled_buffer_available) {
    return;
  }

  // Briefly disable interrupts to atomically copy the ISR-shared buffer.
  uint64_t buffer;
  {
    InterruptLock lock;
    buffer = this->store_.filled_buffer;
    this->store_.filled_buffer_available = false;
  }

  time_t utc_epoch;
  bool cest;
  if (!DCF77Decoder::decode_frame(buffer, &utc_epoch, &cest)) {
    ESP_LOGW(TAG, "Received frame failed parity check, discarding");
    return;
  }

  ESP_LOGD(TAG, "Synchronized time from DCF77: epoch=%lld (%s)", static_cast<long long>(utc_epoch),
           cest ? "CEST" : "CET");
  this->synchronize_epoch_(static_cast<uint32_t>(utc_epoch));
}

void DCF77Time::dump_config() {
  ESP_LOGCONFIG(TAG, "DCF77 Time:");
  LOG_PIN("  Pin: ", this->pin_);
  RealTimeClock::dump_config();
}

}  // namespace esphome::dcf77
