#include "dcf77_decoder.h"
#include <cstring>

namespace esphome::dcf77 {

namespace {

// Bit layout of a DCF77 minute frame -- identical to the original
// hencou/Arduino-DCF77 `DCF77Buffer` struct.
struct DCF77Frame {
  uint64_t prefix : 17;
  uint64_t cest : 1;    // CEST (summer time) flag
  uint64_t cet : 1;     // CET (winter time) flag
  uint64_t unused : 2;  // unused bits
  uint64_t minute : 7;  // minutes, BCD
  uint64_t p1 : 1;      // parity minutes
  uint64_t hour : 6;    // hours, BCD
  uint64_t p2 : 1;      // parity hours
  uint64_t day : 6;     // day of month, BCD
  uint64_t weekday : 3; // day of week
  uint64_t month : 5;   // month, BCD
  uint64_t year : 8;    // year (BCD, offset from 2000)
  uint64_t p3 : 1;      // parity date
};

/// Days since 1970-01-01 for a given civil (proleptic Gregorian) date.
/// Portable, TZ-independent replacement for mktime()/timegm().
/// Algorithm: Howard Hinnant, "chrono-Compatible Low-Level Date Algorithms"
/// (public domain).
int64_t days_from_civil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);              // [0, 399]
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;    // [0, 365]
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;             // [0, 146096]
  return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

}  // namespace

bool DCF77Decoder::decode_frame(uint64_t buffer, time_t *utc_epoch, bool *cest) {
  DCF77Frame frame;
  std::memcpy(&frame, &buffer, sizeof(frame));

  // Calculate parity over the minute/hour/date segments, exactly as the
  // original library does.
  bool parity_flag = false;
  bool parity_min = false;
  bool parity_hour = false;
  bool parity_date = false;
  for (int pos = 0; pos < 59; pos++) {
    const bool bit = (buffer >> pos) & 1ULL;
    // Reset parity flag at the start of each segment.
    if (pos == 21 || pos == 29 || pos == 36) {
      parity_flag = false;
    }
    // Capture the running parity when each segment ends.
    if (pos == 28) parity_min = parity_flag;
    if (pos == 35) parity_hour = parity_flag;
    if (pos == 58) parity_date = parity_flag;
    if (bit) parity_flag = !parity_flag;
  }

  if (parity_min != frame.p1 || parity_hour != frame.p2 || parity_date != frame.p3 ||
      frame.cest == frame.cet) {
    return false;  // Corrupted / noisy frame -- reject it.
  }

  const int minute = (frame.minute / 16) * 10 + (frame.minute % 16);
  const int hour = (frame.hour / 16) * 10 + (frame.hour % 16);
  const int day = (frame.day / 16) * 10 + (frame.day % 16);
  const unsigned month = (frame.month / 16) * 10 + (frame.month % 16);
  const int year = 2000 + (frame.year / 16) * 10 + (frame.year % 16);

  const bool is_cest = frame.cest;
  const int utc_offset_hours = is_cest ? 2 : 1;

  // Convert the received CET/CEST local time to a UTC epoch using a portable,
  // TZ-independent calendar calculation.
  const int64_t local_days = days_from_civil(year, month, static_cast<unsigned>(day));
  const time_t local_epoch = static_cast<time_t>(local_days) * 86400 + hour * 3600 + minute * 60;

  *utc_epoch = local_epoch - utc_offset_hours * 3600;
  if (cest != nullptr) {
    *cest = is_cest;
  }
  return true;
}

}  // namespace esphome::dcf77
