#include "i2c_sniffer.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cinttypes>
#include <driver/gpio.h>

namespace esphome
{
    namespace i2c_sniffer
    {

#define I2C_IDLE 0
#define I2C_TRX 2

        static const char *const TAG = "i2c_sniffer";

        static volatile bool i2cStatus = I2C_IDLE; // Status of the I2C BUS
        static uint32_t lastStartMillis = 0;       // stoe the last time
        static uint8_t dataBuffer[9600];           // Array for storing data of the I2C communication
        static volatile uint16_t bufferPoiW = 0;   // points to the first empty position in the dataBufer to write
        static uint16_t bufferPoiR = 0;            // points to the position where to start read from
        static volatile uint8_t bitCount = 0;      // counter of bit appeared on the BUS
        static volatile uint16_t byteCount = 0;    // counter of bytes were writen in one communication.
        static volatile bool i2cBitD = 0;          // Container of the actual SDA bit
        static volatile bool i2cBitD2 = 0;         // Container of the actual SDA bit
        static volatile bool i2cBitC = 0;          // Container of the actual SDA bit
        static volatile bool i2cClk = 0;           // Container of the actual SCL bit
        static volatile uint8_t i2cCase = 0;       // Container of the last ACK value
        static volatile uint16_t falseStart = 0;   // Counter of false start events
        static volatile uint16_t sclUpCnt = 0;     // Auxiliary variable to count rising SCL
        static volatile uint16_t sdaUpCnt = 0;     // Auxiliary variable to count rising SDA
        static volatile uint16_t sdaDownCnt = 0;   // Auxiliary variable to count falling SDA

        uint8_t sda_pin = 21;
        uint8_t scl_pin = 22;

        void IRAM_ATTR i2cTriggerOnRaisingSCL()
        {
            sclUpCnt++;

            // is it a false trigger?
            if (i2cStatus == I2C_IDLE)
            {
                falseStart++;
            }

            // get the value from SDA
            i2cBitC = gpio_get_level((gpio_num_t)sda_pin);

            // decide wherewe are and what to do with incoming data
            i2cCase = 0; // normal case

            if (bitCount == 8) // ACK case
            {
                i2cCase = 1;
                bufferPoiW++;
            }

            if (bitCount == 7 && byteCount == 0) // R/W if the first address byte
            {
                i2cCase = 2;
                bufferPoiW++;
            }

            bitCount++;

            switch (i2cCase)
            {
            case 0: // normal case
                dataBuffer[bufferPoiW] *= 2;
                dataBuffer[bufferPoiW] += i2cBitC;
                break;       // end of case 0 general
            case 1:          // ACK
                if (i2cBitC) // 1 NACK SDA HIGH
                {
                    dataBuffer[bufferPoiW++] = '-'; // 45
                }
                else // 0 ACK SDA LOW
                {
                    dataBuffer[bufferPoiW++] = '+'; // 43
                }
                byteCount++;
                bitCount = 0;
                break; // end of case 1 ACK
            case 2:
                if (i2cBitC)
                {
                    dataBuffer[bufferPoiW++] = 'R'; // 82
                }
                else
                {
                    dataBuffer[bufferPoiW++] = 'W'; // 87
                }
                break; // end of case 2 R/W
            }
        }

        void IRAM_ATTR i2cTriggerOnChangeSDA()
        {
            // make sure that the SDA is in stable state
            do
            {
                i2cBitD = gpio_get_level((gpio_num_t)sda_pin);
                i2cBitD2 = gpio_get_level((gpio_num_t)sda_pin);
            } while (i2cBitD != i2cBitD2);

            if (i2cBitD) // RISING if SDA is HIGH (1)
            {

                i2cClk = gpio_get_level((gpio_num_t)scl_pin);
                if ((i2cStatus != I2C_IDLE) && i2cClk) // If SCL still HIGH then it is a STOP sign
                {
                    i2cStatus = I2C_IDLE;
                    bitCount = 0;
                    byteCount = 0;
                    bufferPoiW--;
                    dataBuffer[bufferPoiW++] = 's';  // 115
                    dataBuffer[bufferPoiW++] = '\n'; // 10
                }
                sdaUpCnt++;
            }
            else // FALLING if SDA is LOW
            {

                i2cClk = gpio_get_level((gpio_num_t)scl_pin);
                if (i2cStatus == I2C_IDLE && i2cClk) // If SCL still HIGH than this is a START
                {
                    i2cStatus = I2C_TRX;

                    bitCount = 0;
                    byteCount = 0;
                    dataBuffer[bufferPoiW++] = 'S'; // 83 STOP
                }
                sdaDownCnt++;
            }
        }

        void I2cSniffer::processDataBuffer()
        {
            if (bufferPoiW == bufferPoiR) // There is nothing to say
                return;

            uint16_t pw = bufferPoiW;
            std::string s;
            //  print out falseStart
            ESP_LOGI(TAG, "SCL up: %d SDA up: %d SDA down: %d false start: %d\n", sclUpCnt, sdaUpCnt, sdaDownCnt, falseStart);
            // print out the content of the buffer
            for (int i = bufferPoiR; i < pw; i++)
            {
                if (dataBuffer[i] == 'S') {s += (char)dataBuffer[i];}
                else if (dataBuffer[i] == 's') {s += (char)dataBuffer[i];}
                else if (dataBuffer[i] == 'W') {s += (char)dataBuffer[i];}
                else if (dataBuffer[i] == 'R') {s += (char)dataBuffer[i];}
                else if (dataBuffer[i] == '+') {s += (char)dataBuffer[i];}
                else if (dataBuffer[i] == '-') {s += (char)dataBuffer[i];}
                else if (dataBuffer[i] == '\n') {s += (char)dataBuffer[i];}
                else {
                    s += toHex(dataBuffer[i] >> 4);
                    s += toHex(dataBuffer[i] & 0xF);
                }

                bufferPoiR++;
            }
            ESP_LOGI(TAG, "Message: %s", s.c_str());

            // if there is no I2C action in progress then buffer was printed out completly and can be reset.
            if (i2cStatus == I2C_IDLE && pw == bufferPoiW)
            {
                bufferPoiW = 0;
                bufferPoiR = 0;
            }
        }

        void I2cSniffer::resetI2cVariable()
        {
            i2cStatus = I2C_IDLE;
            bufferPoiW = 0;
            bufferPoiR = 0;
            bitCount = 0;
            falseStart = 0;
        }

        char I2cSniffer::toHex(uint8_t c)
        {
            return c < 10 ? c + '0' : c + 'A' - 10;
        }

        I2cSniffer::I2cSniffer() {}

        void I2cSniffer::dump_config()
        {
            ESP_LOGI(TAG, "ESP32 ESPHome I2C logger. Legend:");
            ESP_LOGI(TAG, "S Start");
            ESP_LOGI(TAG, "s Stop");
            ESP_LOGI(TAG, "W Master will write");
            ESP_LOGI(TAG, "R Master will read");
            ESP_LOGI(TAG, "+ ACK");
            ESP_LOGI(TAG, "- NACK");
            ESP_LOGI(TAG, "Address, command and data values are shown in HEX format");
        }

        void I2cSniffer::setup()
        {
            sda_pin = sda_pin_;
            scl_pin = scl_pin_;

            gpio_config_t io_conf = {};
            io_conf.pin_bit_mask = (1ULL << sda_pin) | (1ULL << scl_pin);
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.intr_type = GPIO_INTR_DISABLE;
            gpio_config(&io_conf);

            resetI2cVariable();

            gpio_install_isr_service(0);
            gpio_set_intr_type((gpio_num_t)scl_pin, GPIO_INTR_POSEDGE);
            gpio_isr_handler_add((gpio_num_t)scl_pin, [](void *) { i2cTriggerOnRaisingSCL(); }, nullptr);
            gpio_set_intr_type((gpio_num_t)sda_pin, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add((gpio_num_t)sda_pin, [](void *) { i2cTriggerOnChangeSDA(); }, nullptr);

            ESP_LOGI(TAG, "Ready!");
        }

        void I2cSniffer::loop()
        {
            if (millis() - loopTimer > 5000)
            {
                loopTimer = millis();
                if (i2cStatus == I2C_IDLE)
                {
                    processDataBuffer();
                }
            }
        }

    } // namespace i2c_sniffer
} // namespace esphome
