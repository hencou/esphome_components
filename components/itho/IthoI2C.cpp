#include <Arduino.h>
#include <driver/i2c.h>

#include "IthoI2C.h"

namespace esphome
{
  namespace itho
  {
    IthoI2C::IthoI2C(SystemConfig *systemConfig) : systemConfig(systemConfig) {}

    void IthoI2C::i2c_master_init()
    {
      while (digitalRead((gpio_num_t)systemConfig->getI2C_SCL_Pin()) == LOW)
      {
        vTaskDelay(10 / portTICK_RATE_MS);
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
      
      i2c_param_config(I2C_MASTER_NUM, &conf);
      i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    }

    void IthoI2C::i2c_master_deinit()
    {
      i2c_driver_delete(I2C_MASTER_NUM);
    }

    void IthoI2C::i2c_slave_init()
    {
       while (digitalRead((gpio_num_t)systemConfig->getI2C_SCL_Pin()) == LOW)
      {
        vTaskDelay(10 / portTICK_RATE_MS);
      }
      
      i2c_config_t conf = {I2C_MODE_SLAVE, (gpio_num_t)systemConfig->getI2C_SDA_Pin(), I2C_SLAVE_SDA_PULLUP, (gpio_num_t)systemConfig->getI2C_SCL_Pin(), I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}};
      i2c_param_config(I2C_SLAVE_NUM, &conf);

      i2c_driver_install(I2C_SLAVE_NUM, conf.mode, I2C_SLAVE_RX_BUF_LEN, 0, 0);

      i2c_set_timeout(I2C_SLAVE_NUM, 0xFFFFF);
    }


    void IthoI2C::i2c_slave_deinit()
    {
      i2c_driver_delete(I2C_SLAVE_NUM);
    }

    esp_err_t IthoI2C::i2c_master_send(const char *buf, uint32_t len)
    {
      i2c_master_init();

      esp_err_t rc;
      i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

      i2c_cmd_handle_t link = i2c_cmd_link_create();
      i2c_master_start(link);
      i2c_master_write(link, (uint8_t *)buf, len, true);
      i2c_master_stop(link);
      rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_RATE_MS);
      i2c_cmd_link_delete(link);

      i2c_master_deinit();

      return rc;
    }

    esp_err_t IthoI2C::i2c_master_send_command(uint8_t addr, const uint8_t *cmd, uint32_t len)
    {
      i2c_master_init();

      esp_err_t rc;
      i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

      i2c_cmd_handle_t link = i2c_cmd_link_create();
      i2c_master_start(link);
      i2c_master_write_byte(link, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
      i2c_master_write(link, (uint8_t *)cmd, len, true);
      i2c_master_stop(link);
      rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_RATE_MS);
      i2c_cmd_link_delete(link);

      i2c_master_deinit();

      return rc;
    }

    esp_err_t IthoI2C::i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size)
    {
      i2c_master_init();

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
      i2c_slave_init();

      uint8_t i2cbuf[I2C_SLAVE_RX_BUF_LEN];
      size_t buflen;

      i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
      buflen = 1;

      while (1)
      {
        int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 50 / portTICK_RATE_MS);
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
