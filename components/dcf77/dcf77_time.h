#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/time/real_time_clock.h"
#include "dcf77_decoder.h"

namespace esphome::dcf77 {

/// Captures DCF77 pulse edges in an ISR and assembles them into a 59-bit
/// frame buffer.
///
/// Kept as a plain struct (not part of a class hierarchy) so the ISR handler
/// stays IRAM-safe and lightweight -- this mirrors the pattern used by
/// ESPHome's own duty_cycle/pulse_counter components. All timing logic below
/// is unchanged from the original hencou/Arduino-DCF77 `int0handler()`; only
/// the pin/timing primitives were swapped for ESPHome's cross-platform HAL
/// (works identically whether the underlying build is ESP-IDF or Arduino).
///
/// Note: because the GPIO pin is configured via ESPHome's normal pin schema,
/// an `inverted: true` pin config (e.g. for signals routed through an
/// opto-isolated input) is already applied before this code ever sees the
/// level -- a "high" reading here always means "the DCF77 receiver's active
/// pulse", regardless of the physical wiring polarity.
struct DCF77EdgeStore {
  // Timing windows, unchanged from the original library.
  static constexpr uint32_t REJECTION_TIME_MS = 700;       // pulse-to-pulse rejection time
  static constexpr uint32_t REJECT_PULSE_WIDTH_MS = 50;    // minimal plausible pulse width
  static constexpr uint32_t SPLIT_TIME_MS = 180;           // 100ms vs 200ms pulse split point
  static constexpr uint32_t SYNC_TIME_MS = 1500;           // end-of-minute gap (spec: 2000ms)

  ISRInternalGPIOPin pin;

  volatile bool up{false};
  volatile uint32_t leading_edge{0};
  volatile uint32_t previous_leading_edge{0};
  volatile int buffer_position{0};
  volatile uint64_t running_buffer{0};

  volatile bool filled_buffer_available{false};
  volatile uint64_t filled_buffer{0};

  static void gpio_intr(DCF77EdgeStore *arg);

 protected:
  static void append_signal(DCF77EdgeStore *arg, uint8_t signal);
  static void finalize_buffer(DCF77EdgeStore *arg);
};

class DCF77Time : public time::RealTimeClock {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }

  void setup() override;
  void loop() override;
  /// RealTimeClock derives from PollingComponent, which requires update() to
  /// be implemented. This component does its actual work continuously in
  /// loop() instead (a valid DCF77 frame arrives roughly once per minute,
  /// asynchronously, so a fixed update_interval poll doesn't fit), so this
  /// is intentionally a no-op.
  void update() override {}
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  InternalGPIOPin *pin_{nullptr};
  DCF77EdgeStore store_{};
};

}  // namespace esphome::dcf77
