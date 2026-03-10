/**
 * LVGL Porting Layer - OPTIMIZED
 * 
 * @file lvgl_port.c
 * @brief Optimized LVGL display driver
 */

#include "lvgl_port.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_types.h"

static const char *TAG = "LVGL_PORT";

// Display handles
static esp_lcd_panel_io_handle_t panel_io = NULL;
static esp_lcd_panel_handle_t panel = NULL;

// OPTIMIZED: Larger buffer for better performance
#define LVGL_BUF_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 2)  // Half screen buffer
static lv_disp_draw_buf_t disp_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// Display flush callback - OPTIMIZED
static void lvgl_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // Direct bitmap draw - LVGL handles partial updates automatically
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    
    lv_disp_flush_ready(disp);
}

// Initialize display driver for LVGL - OPTIMIZED
esp_err_t lvgl_display_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LVGL Display Init %dx%d (OPTIMIZED)", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ESP_LOGI(TAG, "========================================");
    
    // SPI bus - OPTIMIZED: Higher speed
    const spi_bus_config_t buscfg = {
        .mosi_io_num = DISPLAY_MOSI_PIN,
        .sclk_io_num = DISPLAY_SCLK_PIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * 40 * sizeof(uint16_t),  // Larger transfers
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI OK (20MHz)");
    
    // Panel IO - OPTIMIZED: Higher clock speed
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = DISPLAY_CS_PIN,
        .dc_gpio_num = DISPLAY_DC_PIN,
        .spi_mode = 0,
        .pclk_hz = 20 * 1000 * 1000,  // 20MHz (up from 10MHz)
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io));
    ESP_LOGI(TAG, "IO OK");
    
    // Panel
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = DISPLAY_RESET_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
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
    
    // Set offset
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y));
    ESP_LOGI(TAG, "Offset OK");
    
    // Invert
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_LOGI(TAG, "Invert OK");
    
    // Display ON
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    ESP_LOGI(TAG, "Display ON OK");
    
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
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LVGL Display Init Complete (OPTIMIZED)");
    ESP_LOGI(TAG, "========================================");
    
    return ESP_OK;
}

// Initialize LVGL - OPTIMIZED
esp_err_t lvgl_init_system(void)
{
    ESP_LOGI(TAG, "LVGL System Init (OPTIMIZED)...");
    
    // First initialize display hardware
    ESP_LOGI(TAG, "Calling lvgl_display_init()...");
    esp_err_t ret = lvgl_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lvgl_display_init() failed: 0x%x", ret);
        return ret;
    }
    ESP_LOGI(TAG, "lvgl_display_init() SUCCESS!");
    
    // LVGL init
    lv_init();
    ESP_LOGI(TAG, "lv_init() OK");
    
    // Allocate buffers - OPTIMIZED: Use DMA capable memory
    ESP_LOGI(TAG, "Allocating LVGL buffers: %d bytes each", LVGL_BUF_SIZE * sizeof(lv_color_t));
    buf1 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "DMA buffer alloc failed, trying DRAM");
        buf1 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
        buf2 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
    }
    
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Buffer alloc failed");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Buffers allocated at: buf1=%p, buf2=%p", buf1, buf2);
    
    // Initialize draw buffer
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_BUF_SIZE);
    
    // Register display driver - OPTIMIZED
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_disp_flush;
    disp_drv.draw_buf = &disp_buf;
    
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    
    if (disp) {
        ESP_LOGI(TAG, "LVGL Display Driver Registered (OPTIMIZED)");
        ESP_LOGI(TAG, "Buffer size: %d pixels (%d bytes)", LVGL_BUF_SIZE, LVGL_BUF_SIZE * sizeof(lv_color_t));
    } else {
        ESP_LOGE(TAG, "Failed to register LVGL display driver");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// LVGL tick task - OPTIMIZED: 10ms
static void lvgl_tick_task(void *pvParameters)
{
    while (1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// LVGL main task - OPTIMIZED: 20ms (50 FPS)
static void lvgl_main_task(void *pvParameters)
{
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS instead of 20 FPS
    }
}

// Start LVGL tasks - OPTIMIZED
esp_err_t lvgl_start_tasks(void)
{
    ESP_LOGI(TAG, "Starting LVGL tasks (OPTIMIZED)...");
    
    // Create tick task
    xTaskCreate(lvgl_tick_task, "lvgl_tick", 4096, NULL, 5, NULL);
    
    // Create main task - OPTIMIZED: Higher priority
    xTaskCreate(lvgl_main_task, "lvgl_main", 8192, NULL, 6, NULL);
    
    ESP_LOGI(TAG, "LVGL tasks started (50 FPS)");
    
    return ESP_OK;
}
