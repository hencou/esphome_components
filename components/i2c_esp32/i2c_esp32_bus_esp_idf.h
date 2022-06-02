#pragma once

#include "i2c_esp32_bus.h"
#include "esphome/core/component.h"
#include <driver/i2c.h>

namespace esphome {
namespace i2c_esp32 {

enum RecoveryCode {
  RECOVERY_FAILED_SCL_LOW,
  RECOVERY_FAILED_SDA_LOW,
  RECOVERY_COMPLETED,
};

class IDFI2CBus : public I2CBus, public Component {
 public:
  void setup() override;
  void dump_config() override;
  ErrorCode readv(uint8_t address, ReadBuffer *buffers, size_t cnt) override;
  ErrorCode write_bytes_raw(const char* buffer, uint32_t len);
  size_t slave_receive(uint8_t address_, uint8_t i2c_receive_buf[]);
  ErrorCode writev(uint8_t address, WriteBuffer *buffers, size_t cnt, bool stop) override;
  float get_setup_priority() const override { return setup_priority::BUS; }

  void set_scan(bool scan) { scan_ = scan; }
  void set_sda_pin(uint8_t sda_pin) { sda_pin_ = sda_pin; }
  void set_sda_pullup_enabled(bool sda_pullup_enabled) { sda_pullup_enabled_ = sda_pullup_enabled; }
  void set_scl_pin(uint8_t scl_pin) { scl_pin_ = scl_pin; }
  void set_scl_pullup_enabled(bool scl_pullup_enabled) { scl_pullup_enabled_ = scl_pullup_enabled; }
  void set_frequency(uint32_t frequency) { frequency_ = frequency; }

 private:
  void init(uint8_t address, i2c_mode_t mode);
  void recover_();
  RecoveryCode recovery_result_;

 protected:
  i2c_port_t port_;
  uint8_t sda_pin_;
  bool sda_pullup_enabled_;
  uint8_t scl_pin_;
  bool scl_pullup_enabled_;
  uint32_t frequency_;
  bool initialized_ = false;
};

}  // namespace i2c_esp32
}  // namespace esphome

