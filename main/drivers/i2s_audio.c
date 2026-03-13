/**
 * @file i2s_audio.c
 * @brief I2S Audio Driver for ES8311 Codec (ESP-IDF v5.2)
 */

#include "i2s_audio.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_heap_caps.h"

static const char *TAG = "I2S_AUDIO";

static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
static bool is_initialized = false;

// ES8311 I2C address
#define ES8311_ADDR         0x18

// Simple ES8311 register write
static esp_err_t es8311_write_reg(uint8_t reg, uint8_t value) {
    uint8_t write_buf[2] = {reg, value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, ES8311_ADDR, write_buf, 2, pdMS_TO_TICKS(100));
}

// Initialize ES8311 codec
static esp_err_t es8311_init(void) {
    ESP_LOGI(TAG, "Initializing ES8311 codec...");
    
    // ES8311 configuration via I2C
    es8311_write_reg(0x00, 0x80);  // Chip reset
    vTaskDelay(pdMS_TO_TICKS(10));
    es8311_write_reg(0x00, 0x00);  // Release reset
    es8311_write_reg(0x01, 0x00);  // Power up all
    
    ESP_LOGI(TAG, "ES8311 initialized");
    return ESP_OK;
}

esp_err_t i2s_audio_init(void) {
    if (is_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing I2S audio...");
    
    // Initialize I2C for codec
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
        .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);
    
    // Initialize ES8311 codec
    es8311_init();
    
    // I2S configuration for ESP-IDF v5.2
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AUDIO_I2S_GPIO_BCLK,
            .ws = AUDIO_I2S_GPIO_WS,
            .dout = AUDIO_I2S_GPIO_DOUT,
            .din = AUDIO_I2S_GPIO_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    // Channel configuration
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    
    // Create TX channel (playback)
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    
    // Create RX channel (record)
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    
    is_initialized = true;
    ESP_LOGI(TAG, "I2S audio initialized (16kHz, mono, 16-bit)");
    
    return ESP_OK;
}

esp_err_t i2s_audio_record(int16_t *buffer, size_t size) {
    if (!is_initialized || !rx_handle) {
        return ESP_ERR_INVALID_STATE;
    }
    
    size_t bytes_read;
    esp_err_t ret = i2s_channel_read(rx_handle, buffer, size, &bytes_read, pdMS_TO_TICKS(100));
    
    return (ret == ESP_OK && bytes_read > 0) ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t i2s_audio_play(const int16_t *buffer, size_t size) {
    if (!is_initialized || !tx_handle) {
        return ESP_ERR_INVALID_STATE;
    }
    
    size_t bytes_written;
    esp_err_t ret = i2s_channel_write(tx_handle, buffer, size, &bytes_written, pdMS_TO_TICKS(100));
    
    return (ret == ESP_OK && bytes_written > 0) ? ESP_OK : ESP_ERR_TIMEOUT;
}

void i2s_audio_deinit(void) {
    if (tx_handle) {
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        tx_handle = NULL;
    }
    if (rx_handle) {
        i2s_channel_disable(rx_handle);
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
    }
    i2c_driver_delete(I2C_MASTER_NUM);
    is_initialized = false;
}
