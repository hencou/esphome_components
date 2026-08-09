#pragma once

#include <cstdint>
#include <ctime>

namespace esphome::dcf77 {

/// Pure, framework-independent DCF77 frame decoder.
///
/// Takes a raw 59-bit frame (as assembled by the edge-timing ISR in
/// DCF77EdgeStore) and converts it into a UTC unix timestamp. This class has
/// no Arduino or ESPHome dependencies at all -- it only needs <cstdint> and
/// <ctime> -- so it can be reused or unit-tested independently of ESPHome,
/// e.g. on a desktop build.
///
/// The bit layout and parity algorithm are unchanged from the original
/// hencou/Arduino-DCF77 library; only the surrounding time-handling was
/// rewritten to avoid the Arduino Time/TimeLib dependency.
class DCF77Decoder {
 public:
  /// Decodes a captured 59-bit DCF77 minute-frame.
  ///
  /// Returns false if the parity check fails -- this is the primary
  /// integrity check for a noisy/corrupted reception and such frames must be
  /// discarded.
  ///
  /// On success, *utc_epoch receives the UTC unix timestamp represented by
  /// the frame, and *cest (if non-null) reports whether CEST (Central
  /// European Summer Time) was in effect at the moment of reception.
  static bool decode_frame(uint64_t buffer, time_t *utc_epoch, bool *cest);
};

}  // namespace esphome::dcf77
