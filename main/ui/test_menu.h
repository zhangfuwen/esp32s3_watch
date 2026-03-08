/**
 * @file test_menu.h
 * @brief Test Menu - Hardware test interface for ESP32-S3 Watch
 */

#ifndef TEST_MENU_H
#define TEST_MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

esp_err_t test_menu_init(void);

esp_err_t test_menu_start(void);

esp_err_t test_menu_stop(void);

void test_menu_show_main(void);

void test_menu_handle_button_press(uint8_t button_id);

lv_obj_t* test_menu_create_test_screen(const char *title);

void test_imu_update(void);
void test_battery_update(void);

#endif // TEST_MENU_H
