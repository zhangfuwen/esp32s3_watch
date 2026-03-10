/**
 * @file display_test.c
 * @brief Simple Display Test - Draw colored rectangles
 */

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "board_config.h"

static const char *TAG = "DISP_TEST";

static spi_device_handle_t spi_handle;

static void spi_write_command(uint8_t cmd) {
    gpio_set_level(DISPLAY_DC_PIN, 0);
    spi_transaction_t trans = {.tx_buffer = &cmd, .length = 8};
    spi_device_transmit(spi_handle, &trans);
}

static void spi_write_data(uint8_t data) {
    gpio_set_level(DISPLAY_DC_PIN, 1);
    spi_transaction_t trans = {.tx_buffer = &data, .length = 8};
    spi_device_transmit(spi_handle, &trans);
}

static void spi_write_data16(uint16_t data) {
    gpio_set_level(DISPLAY_DC_PIN, 1);
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    spi_transaction_t trans = {.tx_buffer = buf, .length = 16};
    spi_device_transmit(spi_handle, &trans);
}

static void set_window(int x1, int y1, int x2, int y2) {
    spi_write_command(0x2A);  // CASET
    spi_write_data16(x1);
    spi_write_data16(x2);
    
    spi_write_command(0x2B);  // RASET
    spi_write_data16(y1);
    spi_write_data16(y2);
    
    spi_write_command(0x2C);  // RAMWR
}

static void fill_rect(int x1, int y1, int x2, int y2, uint16_t color) {
    set_window(x1, y1, x2, y2);
    gpio_set_level(DISPLAY_DC_PIN, 1);
    
    uint32_t count = (x2 - x1 + 1) * (y2 - y1 + 1);
    uint8_t high = color >> 8;
    uint8_t low = color & 0xFF;
    
    for (uint32_t i = 0; i < count; i++) {
        spi_write_data(high);
        spi_write_data(low);
    }
}

esp_err_t display_test_run(void) {
    ESP_LOGI(TAG, "=== Display Test Starting ===");
    
    // Initialize SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = DISPLAY_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = DISPLAY_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized");
    
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 20000000,
        .mode = 0,
        .spics_io_num = DISPLAY_CS_PIN,
        .queue_size = 1,
    };
    
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle));
    ESP_LOGI(TAG, "SPI device added");
    
    // Initialize GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_DC_PIN) | (1ULL << DISPLAY_CS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
    };
    gpio_config(&io_conf);
    
    if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
        io_conf.pin_bit_mask = (1ULL << DISPLAY_BACKLIGHT_PIN);
        gpio_config(&io_conf);
        gpio_set_level(DISPLAY_BACKLIGHT_PIN, 1);
        ESP_LOGI(TAG, "Backlight ON");
    }
    
    // Reset sequence
    ESP_LOGI(TAG, "Resetting display...");
    // No reset pin, skip
    
    // Initialization commands
    ESP_LOGI(TAG, "Sending init commands...");
    spi_write_command(0x01);  // SWRESET
    vTaskDelay(pdMS_TO_TICKS(150));
    
    spi_write_command(0x11);  // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(120));
    
    spi_write_command(0x3A);  // COLMOD
    spi_write_data(0x55);     // 16-bit color
    
    spi_write_command(0x36);  // MADCTL
    spi_write_data(0x00);     // No rotation, RGB
    
    spi_write_command(0x21);  // INVON
    spi_write_command(0x29);  // DISPON
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Display initialized, drawing test pattern...");
    
    // Draw test pattern
    fill_rect(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT / 4, 0xF800);     // Red
    fill_rect(0, DISPLAY_HEIGHT / 4, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT / 2, 0x07E0);  // Green
    fill_rect(0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH - 1, 3 * DISPLAY_HEIGHT / 4, 0x001F);  // Blue
    fill_rect(0, 3 * DISPLAY_HEIGHT / 4, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, 0xFFFF);  // White
    
    ESP_LOGI(TAG, "Test pattern drawn!");
    ESP_LOGI(TAG, "You should see: Red | Green | Blue | White stripes");
    
    return ESP_OK;
}
