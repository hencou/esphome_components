#include "i2c_sniffer.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cinttypes>

namespace esphome
{
    namespace i2c_sniffer
    {

        static const char *const TAG = "i2c_sniffer";
#define I2C_IDLE 0
//#define I2C_START 1
#define I2C_TRX 2
        //#define I2C_RESP 3
        //#define I2C_STOP 4

        static volatile byte i2cStatus = I2C_IDLE; // Status of the I2C BUS
        static uint32_t lastStartMillis = 0;       // stoe the last time
        static volatile byte dataBuffer[9600];     // Array for storing data of the I2C communication
        static volatile uint16_t bufferPoiW = 0;   // points to the first empty position in the dataBufer to write
        static uint16_t bufferPoiR = 0;            // points to the position where to start read from
        static volatile byte bitCount = 0;         // counter of bit appeared on the BUS
        static volatile uint16_t byteCount = 0;    // counter of bytes were writen in one communication.
        static volatile byte i2cBitD = 0;          // Container of the actual SDA bit
        static volatile byte i2cBitD2 = 0;         // Container of the actual SDA bit
        static volatile byte i2cBitC = 0;          // Container of the actual SDA bit
        static volatile byte i2cClk = 0;           // Container of the actual SCL bit
        static volatile byte i2cAck = 0;           // Container of the last ACK value
        static volatile byte i2cCase = 0;          // Container of the last ACK value
        static volatile uint16_t falseStart = 0;   // Counter of false start events
        // static volatile byte respCount =0;//Auxiliary variable to help detect next byte instead of STOP
        // these variables just for statistic reasons
        static volatile uint16_t sclUpCnt = 0;   // Auxiliary variable to count rising SCL
        static volatile uint16_t sdaUpCnt = 0;   // Auxiliary variable to count rising SDA
        static volatile uint16_t sdaDownCnt = 0; // Auxiliary variable to count falling SDA

        

        I2cSniffer::I2cSniffer() {}

        void I2cSniffer::dump_config()
        {
        }

        ////////////////////////////
        //// Interrupt handlers
        /////////////////////////////

        /**
         * Rising SCL makes reading the SDA
         *
         */
        void IRAM_ATTR i2cTriggerOnRaisingSCL()
        {
            sclUpCnt++;

            // is it a false trigger?
            if (i2cStatus == I2C_IDLE)
            {
                falseStart++;
                // return;//this is not clear why do we have so many false START
            }

            // get the value from SDA
            i2cBitC = digitalRead(sda_pin);

            // decide wherewe are and what to do with incoming data
            i2cCase = 0; // normal case

            if (bitCount == 8) // ACK case
                i2cCase = 1;

            if (bitCount == 7 && byteCount == 0) // R/W if the first address byte
                i2cCase = 2;

            bitCount++;

            switch (i2cCase)
            {
            case 0:                                       // normal case
                dataBuffer[bufferPoiW++] = '0' + i2cBitC; // 48
                break;                                    // end of case 0 general
            case 1:                                       // ACK
                if (i2cBitC)                              // 1 NACK SDA HIGH
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

            } // end of switch

        } // END of i2cTriggerOnRaisingSCL()

        /**
         * This is for recognizing I2C START and STOP
         * This is called when the SDA line is changing
         * It is decided inside the function wheather it is a rising or falling change.
         * If SCL is on High then the falling change is a START and the rising is a STOP.
         * If SCL is LOW, then this is the action to set a data bit, so nothing to do.
         */
        void IRAM_ATTR i2cTriggerOnChangeSDA()
        {
            // make sure that the SDA is in stable state
            do
            {
                i2cBitD = digitalRead(sda_pin);
                i2cBitD2 = digitalRead(sda_pin);
            } while (i2cBitD != i2cBitD2);

            // i2cBitD =  digitalRead(sda_pin);

            if (i2cBitD) // RISING if SDA is HIGH (1)
            {

                i2cClk = digitalRead(scl_pin);
                if (i2cStatus = !I2C_IDLE && i2cClk == 1) // If SCL still HIGH then it is a STOP sign
                {
                    // i2cStatus = I2C_STOP;
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

                i2cClk = digitalRead(scl_pin);
                if (i2cStatus == I2C_IDLE && i2cClk) // If SCL still HIGH than this is a START
                {
                    i2cStatus = I2C_TRX;
                    // lastStartMillis = millis();//takes too long in an interrupt handler and caused timeout panic and CPU restart
                    bitCount = 0;
                    byteCount = 0;
                    dataBuffer[bufferPoiW++] = 'S'; // 83 STOP
                    // i2cStatus = START;
                }
                sdaDownCnt++;
            }
        } // END of i2cTriggerOnChangeSDA()

        ////////////////////////////////
        //// Functions
        ////////////////////////////////

        /**
         * Reset all important variable
         */
        void resetI2cVariable()
        {
            i2cStatus = I2C_IDLE;
            bufferPoiW = 0;
            bufferPoiR = 0;
            bitCount = 0;
            falseStart = 0;
        } // END of resetI2cVariable()

        /**
         * @DESC Write out the buffer to the serial console
         *
         */
        void processDataBuffer()
        {
            if (bufferPoiW == bufferPoiR) // There is nothing to say
                return;

            uint16_t pw = bufferPoiW;
            // print out falseStart
            ESP_LOGI(TAG, "\nSCL up: %d SDA up: %d SDA down: %d false start: %d\n", sclUpCnt, sdaUpCnt, sdaDownCnt, falseStart);
            // print out the content of the buffer
            ESP_LOGI(TAG, "Buffer: %04x", __builtin_bswap16(pw));;
            // for (int i = bufferPoiR; i < pw; i++)
            // {
            //     Serial.write(dataBuffer[i]);
            //     bufferPoiR++;
            // }

            // if there is no I2C action in progress and there wasn't during the Serial.print then buffer was printed out completly and can be reset.
            if (i2cStatus == I2C_IDLE && pw == bufferPoiW)
            {
                bufferPoiW = 0;
                bufferPoiR = 0;
            }
        } // END of processDataBuffer()

        /////////////////////////////////
        ////  MAIN entry point of the program
        /////////////////////////////////
        void I2cSniffer::setup()
        {

            // Define pins for SCL, SDA
            pinMode(scl_pin, INPUT_PULLUP);
            pinMode(sda_pin, INPUT_PULLUP);
            // pinMode(scl_pin, INPUT);
            // pinMode(sda_pin, INPUT);

            // reset variables
            resetI2cVariable();

            // Atach interrupt handlers to the interrupts on GPIOs
            attachInterrupt(scl_pin, i2cTriggerOnRaisingSCL, RISING); // trigger for reading data from SDA
            attachInterrupt(sda_pin, i2cTriggerOnChangeSDA, CHANGE);  // for I2C START and STOP

        } // END of setup

        /**
         * LOOP
         */
        void I2cSniffer::loop()
        {
            // if it is in IDLE, then write out the databuffer to the serial consol
            if (i2cStatus == I2C_IDLE)
            {
                processDataBuffer();
                ESP_LOGI(TAG, "\rStart delay    ");
                vTaskDelay(5000 / portTICK_RATE_MS);
                ESP_LOGI(TAG, "\rEnd delay    ");
                vTaskDelay(500 / portTICK_RATE_MS);
            }
        } // END of loop

    } // namespace i2c_sniffer
} // namespace esphome
