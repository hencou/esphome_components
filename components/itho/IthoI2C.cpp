#include <driver/i2c.h>
#include <driver/gpio.h>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "IthoI2C.h"

namespace esphome
{
  namespace itho
  {
    static const char *const TAG = "itho.i2c";

    IthoI2C::IthoI2C(SystemConfig *systemConfig) : systemConfig(systemConfig) {}

    bool IthoI2C::i2c_wait_for_idle()
    {
      const gpio_num_t scl = (gpio_num_t)systemConfig->getI2C_SCL_Pin();
      for (uint32_t waited = 0; waited < I2C_BUS_IDLE_TIMEOUT_MS; waited += 10)
      {
        if (gpio_get_level(scl) != 0)
        {
          return true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
      }

      const int result = i2c_clear_bus();
      if (result == 0)
      {
        ESP_LOGW(TAG, "I2C bus was stuck, recovered by clearing it");
        return true;
      }
      ESP_LOGE(TAG, "I2C bus is stuck and could not be cleared (reason %d)", result);
      return false;
    }

    int IthoI2C::i2c_clear_bus()
    {
      const gpio_num_t sda = (gpio_num_t)systemConfig->getI2C_SDA_Pin();
      const gpio_num_t scl = (gpio_num_t)systemConfig->getI2C_SCL_Pin();

      gpio_config_t conf = {};
      conf.pin_bit_mask = (1ULL << sda) | (1ULL << scl);
      conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
      conf.pull_up_en = GPIO_PULLUP_ENABLE;
      conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
      conf.intr_type = GPIO_INTR_DISABLE;
      gpio_config(&conf);
      gpio_set_level(sda, 1);
      gpio_set_level(scl, 1);
      delayMicroseconds(10);

      if (gpio_get_level(scl) == 0)
      {
        return 1; // SCL is held low by another device, nothing we can do
      }

      // clock the bus until the slave releases SDA, then send a stop condition
      for (int pulses = 0; pulses < 20 && gpio_get_level(sda) == 0; pulses++)
      {
        gpio_set_level(scl, 0);
        delayMicroseconds(10);
        gpio_set_level(scl, 1);
        delayMicroseconds(10);
        if (gpio_get_level(scl) == 0)
        {
          return 2; // slave keeps stretching the clock
        }
      }
      if (gpio_get_level(sda) == 0)
      {
        return 3; // SDA still held low
      }

      gpio_set_level(sda, 0);
      delayMicroseconds(10);
      gpio_set_level(sda, 1);
      delayMicroseconds(10);

      return 0;
    }

    bool IthoI2C::i2c_master_init()
    {
      if (!i2c_wait_for_idle())
      {
        return false;
      }
      
      #ifdef ESPRESSIF32_3_5_0
        i2c_config_t conf = {I2C_MODE_MASTER, (gpio_num_t)systemConfig->getI2C_SDA_Pin(), I2C_MASTER_SDA_PULLUP, (gpio_num_t)systemConfig->getI2C_SCL_Pin(), I2C_MASTER_SCL_PULLUP, {.master = {I2C_MASTER_FREQ_HZ}}};
      #else
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = systemConfig->getI2C_SDA_Pin(),
            .scl_io_num = systemConfig->getI2C_SCL_Pin(),
            .sda_pullup_en = I2C_MASTER_SDA_PULLUP,
            .scl_pullup_en = I2C_MASTER_SCL_PULLUP,
            .master = {
                .clk_speed = I2C_MASTER_FREQ_HZ,
            },
            .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL, // optional
        };
      #endif
      
      esp_err_t rc = i2c_param_config(I2C_MASTER_NUM, &conf);
      if (rc == ESP_OK)
      {
        rc = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
      }
      if (rc != ESP_OK)
      {
        ESP_LOGE(TAG, "I2C master init failed: %s", esp_err_to_name(rc));
        return false;
      }
      return true;
    }

    void IthoI2C::i2c_master_deinit()
    {
      i2c_driver_delete(I2C_MASTER_NUM);
    }

    bool IthoI2C::i2c_slave_init()
    {
      if (!i2c_wait_for_idle())
      {
        return false;
      }
      
      #ifdef ESPRESSIF32_3_5_0
        i2c_config_t conf_slave = {I2C_MODE_SLAVE, slave_sda_pin, I2C_SLAVE_SDA_PULLUP, slave_scl_pin, I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}};
      #else
        i2c_config_t conf_slave = {
            .mode = I2C_MODE_SLAVE,
            .sda_io_num = systemConfig->getI2C_SDA_Pin(),
            .scl_io_num = systemConfig->getI2C_SCL_Pin(),
            .sda_pullup_en = I2C_SLAVE_SDA_PULLUP,
            .scl_pullup_en = I2C_SLAVE_SCL_PULLUP,
            .slave = {
                .addr_10bit_en = 0,
                .slave_addr = I2C_SLAVE_ADDRESS,
                .maximum_speed = 400000UL,
            },
            .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL, // optional
        };
      #endif
      
      esp_err_t rc = i2c_param_config(I2C_SLAVE_NUM, &conf_slave);
      if (rc == ESP_OK)
      {
        rc = i2c_driver_install(I2C_SLAVE_NUM, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, 0, 0);
      }
      if (rc != ESP_OK)
      {
        ESP_LOGE(TAG, "I2C slave init failed: %s", esp_err_to_name(rc));
        return false;
      }

      i2c_set_timeout(I2C_SLAVE_NUM, 0xFFFFF);
      return true;
    }


    void IthoI2C::i2c_slave_deinit()
    {
      i2c_driver_delete(I2C_SLAVE_NUM);
    }

    esp_err_t IthoI2C::i2c_master_send(const char *buf, uint32_t len)
    {
      if (!i2c_master_init())
      {
        return ESP_ERR_INVALID_STATE;
      }

      esp_err_t rc;
      i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

      i2c_cmd_handle_t link = i2c_cmd_link_create();
      i2c_master_start(link);
      i2c_master_write(link, (uint8_t *)buf, len, true);
      i2c_master_stop(link);
      rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200);
      i2c_cmd_link_delete(link);

      i2c_master_deinit();

      return rc;
    }

    esp_err_t IthoI2C::i2c_master_send_command(uint8_t addr, const uint8_t *cmd, uint32_t len)
    {
      if (!i2c_master_init())
      {
        return ESP_ERR_INVALID_STATE;
      }

      esp_err_t rc;
      i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

      i2c_cmd_handle_t link = i2c_cmd_link_create();
      i2c_master_start(link);
      i2c_master_write_byte(link, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
      i2c_master_write(link, (uint8_t *)cmd, len, true);
      i2c_master_stop(link);
      rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200);
      i2c_cmd_link_delete(link);

      i2c_master_deinit();

      return rc;
    }

    esp_err_t IthoI2C::i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size)
    {
      if (!i2c_master_init())
      {
        return ESP_ERR_INVALID_STATE;
      }

      esp_err_t rc;
      i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

      i2c_cmd_handle_t link = i2c_cmd_link_create();
      i2c_master_start(link);
      i2c_master_write_byte(link, (addr << 1) | READ_BIT, ACK_CHECK_EN);
      if (size > 1)
      {
        i2c_master_read(link, data_rd, size - 1, (i2c_ack_type_t)ACK_VAL);
      }
      i2c_master_read_byte(link, data_rd + size - 1, (i2c_ack_type_t)NACK_VAL);
      i2c_master_stop(link);
      rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 2000);
      i2c_cmd_link_delete(link);

      i2c_master_deinit();

      return rc;
    }

    size_t IthoI2C::i2c_slave_receive(uint8_t i2c_receive_buf[])
    {
      if (!i2c_slave_init())
      {
        return 0;
      }

      uint8_t i2cbuf[I2C_SLAVE_RX_BUF_LEN];
      size_t buflen;

      i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
      buflen = 1;

      while (1)
      {
        int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 50);
        if (len1 <= 0)
          break;
        buflen += len1;
      }
      if (buflen > 1)
      {
        for (uint16_t i = 0; i < buflen; i++)
        {
          i2c_receive_buf[i] = i2cbuf[i];
        }
      }
      i2c_slave_deinit();
      return buflen;
    }

  }
}

