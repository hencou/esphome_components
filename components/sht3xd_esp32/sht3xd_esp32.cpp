#include "sht3xd_esp32.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sht3xd_esp32 {

static const char *const TAG = "sht3xd_esp32";

static const uint16_t SHT3XD_COMMAND_READ_SERIAL_NUMBER = 0x3780;
static const uint16_t SHT3XD_COMMAND_READ_STATUS = 0xF32D;
static const uint16_t SHT3XD_COMMAND_CLEAR_STATUS = 0x3041;
static const uint16_t SHT3XD_COMMAND_HEATER_ENABLE = 0x306D;
static const uint16_t SHT3XD_COMMAND_HEATER_DISABLE = 0x3066;
static const uint16_t SHT3XD_COMMAND_SOFT_RESET = 0x30A2;
static const uint16_t SHT3XD_COMMAND_POLLING_H = 0x2400;
static const uint16_t SHT3XD_COMMAND_FETCH_DATA = 0xE000;

void SHT3XDComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SHT3xD...");
  uint16_t raw_serial_number[2];
  if (!this->get_register(SHT3XD_COMMAND_READ_SERIAL_NUMBER, raw_serial_number, 2)) {
    this->mark_failed();
    return;
  }
  uint32_t serial_number = (uint32_t(raw_serial_number[0]) << 16) | uint32_t(raw_serial_number[1]);
  ESP_LOGV(TAG, "    Serial Number: 0x%08X", serial_number);
}
void SHT3XDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SHT3xD:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with SHT3xD failed!");
  }
  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
}
float SHT3XDComponent::get_setup_priority() const { return setup_priority::DATA; }
void SHT3XDComponent::update() {
  if (this->status_has_warning()) {
    ESP_LOGD(TAG, "Retrying to reconnect the sensor.");
    this->write_command(SHT3XD_COMMAND_SOFT_RESET);
  }
  if (!this->write_command(SHT3XD_COMMAND_POLLING_H)) {
    this->status_set_warning();
    return;
  }

  this->set_timeout(50, [this]() {
    uint16_t raw_data[2];
    if (!this->read_data(raw_data, 2)) {
      this->status_set_warning();
      return;
    }

    float temperature = 175.0f * float(raw_data[0]) / 65535.0f - 45.0f;
    float humidity = 100.0f * float(raw_data[1]) / 65535.0f;

    ESP_LOGD(TAG, "Got temperature=%.2f°C humidity=%.2f%%", temperature, humidity);
    if (this->temperature_sensor_ != nullptr)
      this->temperature_sensor_->publish_state(temperature);
    if (this->humidity_sensor_ != nullptr)
      this->humidity_sensor_->publish_state(humidity);
    this->status_clear_warning();
  });
}

// To avoid memory allocations for small writes a stack buffer is used
static const size_t BUFFER_STACK_SIZE = 16;

bool SHT3XDComponent::read_data(uint16_t *data, uint8_t len) {
  const uint8_t num_bytes = len * 3;
  std::vector<uint8_t> buf(num_bytes);

  last_error_ = this->read(buf.data(), num_bytes);
  if (last_error_ != i2c_esp32::ERROR_OK) {
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    const uint8_t j = 3 * i;
    uint8_t crc = sht_crc_(buf[j], buf[j + 1]);
    if (crc != buf[j + 2]) {
      ESP_LOGE(TAG, "CRC8 Checksum invalid at pos %d! 0x%02X != 0x%02X", i, buf[j + 2], crc);
      last_error_ = i2c_esp32::ERROR_CRC;
      return false;
    }
    data[i] = encode_uint16(buf[j], buf[j + 1]);
  }
  return true;
}
/***
 * write command with parameters and insert crc
 * use stack array for less than 4 paramaters. Most sensirion i2c commands have less parameters
 */
bool SHT3XDComponent::write_command_(uint16_t command, CommandLen command_len, const uint16_t *data,
                                        uint8_t data_len) {
  uint8_t temp_stack[BUFFER_STACK_SIZE];
  std::unique_ptr<uint8_t[]> temp_heap;
  uint8_t *temp;
  size_t required_buffer_len = data_len * 3 + 2;

  // Is a dynamic allocation required ?
  if (required_buffer_len >= BUFFER_STACK_SIZE) {
    temp_heap = std::unique_ptr<uint8_t[]>(new uint8_t[required_buffer_len]);
    temp = temp_heap.get();
  } else {
    temp = temp_stack;
  }
  // First byte or word is the command
  uint8_t raw_idx = 0;
  if (command_len == 1) {
    temp[raw_idx++] = command & 0xFF;
  } else {
    // command is 2 bytes
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    temp[raw_idx++] = command >> 8;
    temp[raw_idx++] = command & 0xFF;
#else
    temp[raw_idx++] = command & 0xFF;
    temp[raw_idx++] = command >> 8;
#endif
  }
  // add parameters folllowed by crc
  // skipped if len == 0
  for (size_t i = 0; i < data_len; i++) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    temp[raw_idx++] = data[i] >> 8;
    temp[raw_idx++] = data[i] & 0xFF;
#else
    temp[raw_idx++] = data[i] & 0xFF;
    temp[raw_idx++] = data[i] >> 8;
#endif
    temp[raw_idx++] = sht_crc_(data[i]);
  }
  last_error_ = this->write(temp, raw_idx);
  return last_error_ == i2c_esp32::ERROR_OK;
}

bool SHT3XDComponent::get_register_(uint16_t reg, CommandLen command_len, uint16_t *data, uint8_t len,
                                       uint8_t delay_ms) {
  if (!this->write_command_(reg, command_len, nullptr, 0)) {
    ESP_LOGE(TAG, "Failed to write i2c_esp32 register=0x%X (%d) err=%d,", reg, command_len, this->last_error_);
    return false;
  }
  vTaskDelay(delay_ms / portTICK_RATE_MS);
  bool result = this->read_data(data, len);
  if (!result) {
    ESP_LOGE(TAG, "Failed to read data from register=0x%X err=%d,", reg, this->last_error_);
  }
  return result;
}

// The 8-bit CRC checksum is transmitted after each data word
uint8_t SHT3XDComponent::sht_crc_(uint16_t data) {
  uint8_t bit;
  uint8_t crc = 0xFF;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  crc ^= data >> 8;
#else
  crc ^= data & 0xFF;
#endif
  for (bit = 8; bit > 0; --bit) {
    if (crc & 0x80) {
      crc = (crc << 1) ^ crc_polynomial_;
    } else {
      crc = (crc << 1);
    }
  }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  crc ^= data & 0xFF;
#else
  crc ^= data >> 8;
#endif
  for (bit = 8; bit > 0; --bit) {
    if (crc & 0x80) {
      crc = (crc << 1) ^ crc_polynomial_;
    } else {
      crc = (crc << 1);
    }
  }
  return crc;
}

}  // namespace sht3xd_esp32
}  // namespace esphome
