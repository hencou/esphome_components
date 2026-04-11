/**
 * RF24 ESP-IDF utility implementations.
 *
 * When using the RF24 library's esp-idf branch, PlatformIO's Library
 * Dependency Finder does not compile the utility source files in
 * utility/esp_idf/ (gpio.cpp, spi.cpp, compatibility.cpp).
 *
 * This file provides the same implementations directly in the mi
 * component so they are always compiled when needed.
 *
 * Based on RF24 library (GPL-2.0) by TMRh20, Avamander, 2bndy5
 * https://github.com/nRF24/RF24/tree/esp-idf/utility/esp_idf
 */

#if defined(ESP_PLATFORM) && !defined(ARDUINO)

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

// --- Forward declarations matching RF24's esp-idf utility headers ---

typedef uint8_t rf24_gpio_pin_t;

// --- GPIO implementation ---

#include "RF24_config.h"  // pulls in utility/esp_idf/gpio.h via RF24_arch_config.h

GPIO::GPIO() {}

void GPIO::open(rf24_gpio_pin_t port, gpio_mode_t direction)
{
    close(port);
    esp_err_t ret = gpio_set_direction((gpio_num_t)port, direction);
    ESP_ERROR_CHECK(ret);
    ret = gpio_set_level((gpio_num_t)port, (uint32_t)0);
    ESP_ERROR_CHECK(ret);
}

void GPIO::close(rf24_gpio_pin_t port)
{
    esp_err_t ret = gpio_reset_pin((gpio_num_t)port);
    ESP_ERROR_CHECK(ret);
}

int GPIO::read(rf24_gpio_pin_t port)
{
    return gpio_get_level((gpio_num_t)port);
}

void GPIO::write(rf24_gpio_pin_t port, int value)
{
    esp_err_t ret = gpio_set_level((gpio_num_t)port, value);
    ESP_ERROR_CHECK(ret);
}

GPIO::~GPIO() {}

// --- SPI implementation ---

SPIClass::SPIClass() : bus(nullptr) {}

void SPIClass::begin(spi_host_device_t busNo, uint32_t speed)
{
    spi_bus_config_t busConfig;
    memset(&busConfig, 0, sizeof(busConfig));
#ifdef RF24_DEFAULT_MOSI
    busConfig.mosi_io_num = RF24_DEFAULT_MOSI;
#elif defined(CONFIG_RF24_DEFAULT_MOSI)
    busConfig.mosi_io_num = CONFIG_RF24_DEFAULT_MOSI;
#else
    busConfig.mosi_io_num = -1;
#endif
#ifdef RF24_DEFAULT_MISO
    busConfig.miso_io_num = RF24_DEFAULT_MISO;
#elif defined(CONFIG_RF24_DEFAULT_MISO)
    busConfig.miso_io_num = CONFIG_RF24_DEFAULT_MISO;
#else
    busConfig.miso_io_num = -1;
#endif
#ifdef RF24_DEFAULT_SCLK
    busConfig.sclk_io_num = RF24_DEFAULT_SCLK;
#elif defined(CONFIG_RF24_DEFAULT_SCLK)
    busConfig.sclk_io_num = CONFIG_RF24_DEFAULT_SCLK;
#else
    busConfig.sclk_io_num = -1;
#endif
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.data4_io_num = -1;
    busConfig.data5_io_num = -1;
    busConfig.data6_io_num = -1;
    busConfig.data7_io_num = -1;
    busConfig.max_transfer_sz = 33;

    begin(busNo, speed, SPI_MODE0, &busConfig);
}

void SPIClass::begin(spi_host_device_t busNo, uint32_t speed, uint8_t mode, spi_bus_config_t* busConfig)
{
    esp_err_t ret = spi_bus_initialize(busNo, busConfig, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t device_conf;
    memset(&device_conf, 0, sizeof(device_conf));
    device_conf.mode = mode;
    device_conf.clock_speed_hz = speed;
    device_conf.spics_io_num = -1;
    device_conf.queue_size = 1;

    ret = spi_bus_add_device(busNo, &device_conf, &bus);
    ESP_ERROR_CHECK(ret);
}

uint8_t SPIClass::transfer(uint8_t tx_)
{
    uint8_t recv = 0;
    transfernb(&tx_, &recv, 1);
    return recv;
}

void SPIClass::transfernb(const uint8_t* txBuf, uint8_t* rxBuf, uint32_t len)
{
    spi_transaction_t transactionConfig;
    memset(&transactionConfig, 0, sizeof(transactionConfig));
    transactionConfig.length = len * 8;
    transactionConfig.tx_buffer = txBuf;
    transactionConfig.rx_buffer = rxBuf;
    esp_err_t ret = spi_device_transmit(bus, &transactionConfig);
    ESP_ERROR_CHECK(ret);
}

void SPIClass::transfern(const uint8_t* buf, uint32_t len)
{
    transfernb(buf, NULL, len);
}

void SPIClass::beginTransaction()
{
    esp_err_t ret = spi_device_acquire_bus(bus, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
}

void SPIClass::endTransaction()
{
    spi_device_release_bus(bus);
}

SPIClass::~SPIClass()
{
    if (bus != nullptr) {
        esp_err_t ret = spi_bus_remove_device(bus);
        ESP_ERROR_CHECK(ret);
    }
}

// --- Compatibility (millis/msleep) implementation ---

#ifdef __cplusplus
extern "C" {
#endif

void __msleep(int64_t milisec)
{
    usleep(milisec * 1000);
}

uint32_t __millis()
{
    return (uint32_t)(esp_timer_get_time() / 1000L);
}

#ifdef __cplusplus
}
#endif

#endif // defined(ESP_PLATFORM) && !defined(ARDUINO)
