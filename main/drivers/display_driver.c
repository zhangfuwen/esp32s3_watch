/**
 * @file display_driver.c
 * @brief ST7789 LCD Display Driver Implementation
 */

#include "display_driver.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "rom/gpio.h"

static const char *TAG = "ST7789";

static bool s_initialized = false;
static spi_device_handle_t s_spi_handle = NULL;
static display_rotation_t s_rotation = DISPLAY_ROTATION_0;
static uint16_t s_width = ST7789_WIDTH;
static uint16_t s_height = ST7789_HEIGHT;
static bool s_inverted = false;
static bool m_swap_xy = false;

static gpio_num_t s_pin_cs = GPIO_NUM_12;
static gpio_num_t s_pin_dc = GPIO_NUM_11;
static gpio_num_t s_pin_rst = GPIO_NUM_NC;
static gpio_num_t s_pin_backlight = GPIO_NUM_47;

static void display_driver_write_command(uint8_t cmd)
{
    gpio_set_level(s_pin_dc, 0);
    spi_transaction_t trans = {
        .tx_buffer = &cmd,
        .length = 8,
    };
    spi_device_transmit(s_spi_handle, &trans);
}

static void display_driver_write_data(uint8_t data)
{
    gpio_set_level(s_pin_dc, 1);
    spi_transaction_t trans = {
        .tx_buffer = &data,
        .length = 8,
    };
    spi_device_transmit(s_spi_handle, &trans);
}

static void display_driver_write_data16(uint16_t data)
{
    gpio_set_level(s_pin_dc, 1);
    uint8_t data_buf[2] = { data >> 8, data & 0xFF };
    spi_transaction_t trans = {
        .tx_buffer = data_buf,
        .length = 16,
    };
    spi_device_transmit(s_spi_handle, &trans);
}

static void display_driver_write_buffer(const uint8_t *buffer, size_t length)
{
    gpio_set_level(s_pin_dc, 1);
    spi_transaction_t trans = {
        .tx_buffer = buffer,
        .length = length * 8,
    };
    spi_device_transmit(s_spi_handle, &trans);
}

static esp_err_t display_driver_reset(void)
{
    if (s_pin_rst != GPIO_NUM_NC) {
        gpio_set_level(s_pin_rst, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(s_pin_rst, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }
    return ESP_OK;
}

static esp_err_t display_driver_config_pins(const display_driver_config_t *config)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << s_pin_dc) | (1ULL << s_pin_cs),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    if (s_pin_rst != GPIO_NUM_NC) {
        io_conf.pin_bit_mask = (1ULL << s_pin_rst);
        gpio_config(&io_conf);
    }

    if (s_pin_backlight != GPIO_NUM_NC) {
        io_conf.pin_bit_mask = (1ULL << s_pin_backlight);
        gpio_config(&io_conf);
        gpio_set_level(s_pin_backlight, 1);
    }

    return ESP_OK;
}

static void display_driver_set_rotation_internal(void)
{
    uint8_t madctl = 0;

    switch (s_rotation) {
        case DISPLAY_ROTATION_0:
            s_width = ST7789_WIDTH;
            s_height = ST7789_HEIGHT;
            if (m_swap_xy) {
                madctl = ST7789_MADCTL_MV;
            }
            break;
        case DISPLAY_ROTATION_90:
            s_width = ST7789_HEIGHT;
            s_height = ST7789_WIDTH;
            madctl = ST7789_MADCTL_MV | ST7789_MADCTL_MX;
            break;
        case DISPLAY_ROTATION_180:
            s_width = ST7789_WIDTH;
            s_height = ST7789_HEIGHT;
            madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MY;
            break;
        case DISPLAY_ROTATION_270:
            s_width = ST7789_HEIGHT;
            s_height = ST7789_WIDTH;
            madctl = ST7789_MADCTL_MV | ST7789_MADCTL_MY;
            break;
    }

    madctl |= ST7789_MADCTL_BGR;
    display_driver_write_command(ST7789_MADCTL);
    display_driver_write_data(madctl);
}

esp_err_t display_driver_init(const display_driver_config_t *config)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing ST7789 display driver");

    if (config) {
        s_pin_cs = (gpio_num_t)config->pin_cs;
        s_pin_dc = (gpio_num_t)config->pin_dc;
        s_pin_rst = (gpio_num_t)config->pin_rst;
        s_pin_backlight = (gpio_num_t)config->pin_backlight;
        s_rotation = config->rotation;
        s_inverted = config->invert_colors;
        m_swap_xy = config->swap_xy;
    }

    display_driver_config_pins(config);

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = DISPLAY_MOSI_PIN,
        .miso_io_num = DISPLAY_MISO_PIN,
        .sclk_io_num = DISPLAY_SCLK_PIN,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = ST7789_WIDTH * ST7789_HEIGHT * 2 + 8,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = config ? config->spi_speed : 20000000,
        .mode = 0,
        .spics_io_num = s_pin_cs,
        .queue_size = 1,
        .flags = 0,
        .duty_cycle_pos = 128,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    display_driver_reset();

    display_driver_write_command(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    display_driver_write_command(ST7789_COLMOD);
    display_driver_write_data(ST7789_COLOR_MODE_16BIT);

    display_driver_set_rotation_internal();

    display_driver_write_command(ST7789_INVOFF);
    if (s_inverted) {
        display_driver_write_command(ST7789_INVON);
    }

    display_driver_write_command(ST7789_CASET);
    display_driver_write_data16(0);
    display_driver_write_data16(s_width);

    display_driver_write_command(ST7789_RASET);
    display_driver_write_data16(0);
    display_driver_write_data16(s_height);

    display_driver_write_command(ST7789_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));

    display_driver_write_command(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    s_initialized = true;
    ESP_LOGI(TAG, "Display initialized: %dx%d", s_width, s_height);

    return ESP_OK;
}

esp_err_t display_driver_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    if (s_spi_handle) {
        spi_bus_remove_device(s_spi_handle);
        s_spi_handle = NULL;
    }

    spi_bus_free(SPI2_HOST);
    s_initialized = false;

    return ESP_OK;
}

esp_err_t display_driver_set_rotation(display_rotation_t rotation)
{
    s_rotation = rotation;
    display_driver_set_rotation_internal();

    display_driver_write_command(ST7789_CASET);
    display_driver_write_data16(0);
    display_driver_write_data16(s_width);

    display_driver_write_command(ST7789_RASET);
    display_driver_write_data16(0);
    display_driver_write_data16(s_height);

    return ESP_OK;
}

esp_err_t display_driver_set_window(const display_window_t *window)
{
    display_driver_write_command(ST7789_CASET);
    display_driver_write_data16(window->x);
    display_driver_write_data16(window->x + window->width - 1);

    display_driver_write_command(ST7789_RASET);
    display_driver_write_data16(window->y);
    display_driver_write_data16(window->y + window->height - 1);

    display_driver_write_command(ST7789_RAMWR);

    return ESP_OK;
}

esp_err_t display_driver_write_pixels(const uint16_t *pixels, size_t count)
{
    if (!s_initialized || !pixels || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_set_level(s_pin_dc, 1);

    static uint8_t pixel_buf[4096];
    size_t buf_idx = 0;

    for (size_t i = 0; i < count; i++) {
        pixel_buf[buf_idx++] = pixels[i] >> 8;
        pixel_buf[buf_idx++] = pixels[i] & 0xFF;

        if (buf_idx >= sizeof(pixel_buf)) {
            spi_transaction_t trans = {
                .tx_buffer = pixel_buf,
                .length = buf_idx * 8,
            };
            spi_device_transmit(s_spi_handle, &trans);
            buf_idx = 0;
        }
    }

    if (buf_idx > 0) {
        spi_transaction_t trans = {
            .tx_buffer = pixel_buf,
            .length = buf_idx * 8,
        };
        spi_device_transmit(s_spi_handle, &trans);
    }

    return ESP_OK;
}

esp_err_t display_driver_fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (x < 0 || y < 0 || width <= 0 || height <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (x >= s_width || y >= s_height) {
        return ESP_ERR_INVALID_ARG;
    }

    if (x + width > s_width) {
        width = s_width - x;
    }
    if (y + height > s_height) {
        height = s_height - y;
    }

    display_window_t window = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
    display_driver_set_window(&window);

    uint32_t pixel_count = (uint32_t)width * height;
    uint8_t color_buf[2] = { color >> 8, color & 0xFF };

    gpio_set_level(s_pin_dc, 1);

    for (uint32_t i = 0; i < pixel_count; i++) {
        spi_transaction_t trans = {
            .tx_buffer = color_buf,
            .length = 16,
        };
        spi_device_transmit(s_spi_handle, &trans);
    }

    return ESP_OK;
}

esp_err_t display_driver_fill_screen(uint16_t color)
{
    return display_driver_fill_rect(0, 0, s_width, s_height, color);
}

esp_err_t display_driver_set_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (x < 0 || x >= s_width || y < 0 || y >= s_height) {
        return ESP_ERR_INVALID_ARG;
    }

    display_window_t window = {
        .x = x,
        .y = y,
        .width = 1,
        .height = 1,
    };
    display_driver_set_window(&window);

    return display_driver_write_pixels(&color, 1);
}

esp_err_t display_driver_sleep(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    display_driver_write_command(ST7789_SLPIN);
    vTaskDelay(pdMS_TO_TICKS(120));

    if (s_pin_backlight != GPIO_NUM_NC) {
        gpio_set_level(s_pin_backlight, 0);
    }

    return ESP_OK;
}

esp_err_t display_driver_wake(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    display_driver_write_command(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    if (s_pin_backlight != GPIO_NUM_NC) {
        gpio_set_level(s_pin_backlight, 1);
    }

    return ESP_OK;
}

esp_err_t display_driver_set_backlight(uint8_t brightness)
{
    if (s_pin_backlight == GPIO_NUM_NC) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (brightness > 0) {
        gpio_set_level(s_pin_backlight, 1);
    } else {
        gpio_set_level(s_pin_backlight, 0);
    }

    return ESP_OK;
}

uint16_t display_driver_get_width(void)
{
    return s_width;
}

uint16_t display_driver_get_height(void)
{
    return s_height;
}
