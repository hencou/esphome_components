#include "i2c_sniffer.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cinttypes>
#include <esp_task_wdt.h>

namespace esphome
{
    namespace i2c_sniffer
    {

        static const char *const TAG = "i2c_sniffer";

        I2cSniffer::I2cSniffer() {}

        uint32_t I2cSniffer::waitCl(bool expected)
        {
            uint32_t i = 0;
            while (digitalRead(scl_pin) != expected)
            {
                while ((digitalRead(scl_pin) != expected) && (++i % PAUSE != 0))
                {
                    esp_task_wdt_reset();
                    yield();
                }

                if (i % PAUSE == 0)
                {
                    std::string s;
                    uint32_t i = 0;
                    while (&buf[i] != bufI)
                    {
                        s += buf[i++];
                        if (i % 9 == 0)
                            s += ' ';
                    }
                    if (bufI != &buf[0])
                        ESP_LOGI(TAG, s.c_str());
                    bufI = &buf[0];
                }
                esp_task_wdt_reset();
                yield();
            }
            return i;
        }

        void I2cSniffer::dump_config()
        {
        }

        void I2cSniffer::setup()
        {
            pinMode(sda_pin, INPUT);
            pinMode(scl_pin, INPUT);

            ESP_LOGI(TAG, "Ready!");
        }

        void I2cSniffer::loop()
        {
            uint32_t incr = waitCl(HIGH);
            *bufI++ = digitalRead(sda_pin);
            waitCl(LOW);
        }

    } // namespace i2c_sniffer
} // namespace esphome
