#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/**
 * @brief Initialize I2S audio driver
 * @return ESP_OK on success
 */
esp_err_t i2s_audio_init(void);

/**
 * @brief Record audio data from microphone
 * @param buffer Buffer to store audio data
 * @param size Size of buffer in bytes
 * @return ESP_OK on success
 */
esp_err_t i2s_audio_record(int16_t *buffer, size_t size);

/**
 * @brief Play audio data through speaker
 * @param buffer Buffer containing audio data
 * @param size Size of buffer in bytes
 * @return ESP_OK on success
 */
esp_err_t i2s_audio_play(const int16_t *buffer, size_t size);

/**
 * @brief Deinitialize I2S audio driver
 */
void i2s_audio_deinit(void);

#endif // I2S_AUDIO_H
