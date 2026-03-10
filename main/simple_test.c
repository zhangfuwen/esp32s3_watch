/**
 * @file simple_test.c
 * @brief Simple Display Test - Following working reference code
 */

#include "simple_test.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_vendor.h"

// Helper to send command (for debugging if needed)
static inline void lcd_cmd(esp_lcd_panel_io_handle_t io, uint8_t cmd, const void *data, size_t data_bytes) {
    esp_lcd_panel_io_tx_param(io, cmd, data, data_bytes);
}

static const char *TAG = "SIMPLE_TEST";

static esp_lcd_panel_io_handle_t panel_io = NULL;
static esp_lcd_panel_handle_t panel = NULL;

esp_err_t simple_test_run(void) {
    ESP_LOGI(TAG, "=== Simple Display Test ===");
    
    // SPI bus
    const spi_bus_config_t buscfg = {
        .mosi_io_num = DISPLAY_MOSI_PIN,
        .sclk_io_num = DISPLAY_SCLK_PIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * 20 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI OK");
    
    // Panel IO
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = DISPLAY_CS_PIN,
        .dc_gpio_num = DISPLAY_DC_PIN,
        .spi_mode = 0,
        .pclk_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io));
    ESP_LOGI(TAG, "IO OK");
    
    // Panel - EXACTLY like reference code
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = DISPLAY_RESET_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,  // Reference uses BGR
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
    ESP_LOGI(TAG, "Panel OK");
    
    // Reset
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Reset OK");
    
    // Init
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_LOGI(TAG, "Init OK");
    
    // Set offset - EXACTLY like reference code
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y));
    ESP_LOGI(TAG, "Offset OK: (%d, %d)", DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
    
    // Invert - EXACTLY like reference code
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_LOGI(TAG, "Invert OK");
    
    // Display ON - EXACTLY like reference code
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    ESP_LOGI(TAG, "Display ON");
    
    // Backlight
    gpio_config_t bl_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_BACKLIGHT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_conf);
    gpio_set_level(DISPLAY_BACKLIGHT_PIN, 1);
    ESP_LOGI(TAG, "Backlight ON");
    
    // Draw test pattern
    ESP_LOGI(TAG, "Drawing test pattern...");
    
    // Fill screen with RED (full 240x284)
    uint16_t red = 0xF800;
    ESP_LOGI(TAG, "Drawing RED: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y + 1, &red);
    }
    ESP_LOGI(TAG, "RED screen drawn");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Fill screen with GREEN
    uint16_t green = 0x07E0;
    ESP_LOGI(TAG, "Drawing GREEN: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y + 1, &green);
    }
    ESP_LOGI(TAG, "GREEN screen drawn");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Fill screen with BLUE
    uint16_t blue = 0x001F;
    ESP_LOGI(TAG, "Drawing BLUE: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y + 1, &blue);
    }
    ESP_LOGI(TAG, "BLUE screen drawn");
    
    ESP_LOGI(TAG, "=== Test Complete ===");
    ESP_LOGI(TAG, "You should see: RED (2s) → GREEN (2s) → BLUE (2s)");
    
    return ESP_OK;
}
