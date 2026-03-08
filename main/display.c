/**
 * Display Driver using ESP_LCD (Official ESP-IDF driver)
 */

#include <string.h>
#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF LCD driver
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_types.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "DISPLAY";
static esp_lcd_panel_io_handle_t panel_io = NULL;
static esp_lcd_panel_handle_t panel = NULL;

esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP_LCD ST7789 %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ESP_LOGI(TAG, "========================================");
    
    // SPI bus
    ESP_LOGI(TAG, "1. SPI bus...");
    const spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = DISPLAY_MOSI_PIN,
        .sclk_io_num = DISPLAY_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * 20 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI OK");
    
    // Panel IO
    ESP_LOGI(TAG, "2. Panel IO...");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = DISPLAY_CS_PIN,
        .dc_gpio_num = DISPLAY_DC_PIN,
        .spi_mode = 3,
        .pclk_hz = 20 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io));
    ESP_LOGI(TAG, "IO OK");
    
    // Panel
    ESP_LOGI(TAG, "3. ST7789 panel...");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = DISPLAY_RESET_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
    ESP_LOGI(TAG, "Panel OK");
    
    // Reset
    ESP_LOGI(TAG, "4. Reset...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_LOGI(TAG, "Reset OK");
    
    // Init
    ESP_LOGI(TAG, "5. Init...");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_LOGI(TAG, "Init OK");
    
    // Invert
    ESP_LOGI(TAG, "6. Invert...");
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_LOGI(TAG, "Invert OK");
    
    // Display ON
    ESP_LOGI(TAG, "7. Display ON...");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    ESP_LOGI(TAG, "Display ON OK");
    
    // Backlight
    ESP_LOGI(TAG, "8. Backlight (GPIO %d)...", DISPLAY_BACKLIGHT_PIN);
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
    
    // Test RED
    ESP_LOGI(TAG, "9. RED test...");
    uint16_t red = 0xF800;
    for (int y = 0; y < DISPLAY_HEIGHT; y += 10) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y + 10, &red);
    }
    ESP_LOGI(TAG, "RED OK");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "COMPLETE! Screen should be RED!");
    ESP_LOGI(TAG, "========================================");
    
    return ESP_OK;
}

void display_clear(void)
{
    uint16_t black = 0x0000;
    esp_lcd_panel_draw_bitmap(panel, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, &black);
}

void display_fill(uint16_t color)
{
    // Allocate line buffer on stack (240 * 2 = 480 bytes)
    uint16_t line_buf[DISPLAY_WIDTH];
    for (int i = 0; i < DISPLAY_WIDTH; i++) {
        line_buf[i] = color;
    }
    // Draw line by line
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y + 1, line_buf);
    }
}

void display_draw_text(int x, int y, const char *text) { ESP_LOGD(TAG, "Text: %s", text); }
void display_draw_pixel(int x, int y, uint16_t color) { esp_lcd_panel_draw_bitmap(panel, x, y, x+1, y+1, &color); }
void display_draw_rect(int x, int y, int w, int h, uint16_t color) { esp_lcd_panel_draw_bitmap(panel, x, y, x+w, y+h, &color); }
void display_draw_circle(int x, int y, int r, uint16_t color) { ESP_LOGD(TAG, "Circle"); }
void display_update(void) {}
void display_set_brightness(uint8_t b) { ESP_LOGD(TAG, "Brightness: %d", b); }
void display_sleep(void) { esp_lcd_panel_disp_on_off(panel, false); }
void display_wake(void) { esp_lcd_panel_disp_on_off(panel, true); }
