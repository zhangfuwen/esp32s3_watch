/**
 * @file test_imu.c
 * @brief LVGL IMU Test - QMI8658C sensor data display
 */

#include "test_menu.h"
#include "imu_driver.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "TEST_IMU";

static lv_obj_t *acc_x_label;
static lv_obj_t *acc_y_label;
static lv_obj_t *acc_z_label;
static lv_obj_t *gyr_x_label;
static lv_obj_t *gyr_y_label;
static lv_obj_t *gyr_z_label;
static lv_obj_t *temp_label;
static lv_obj_t *status_label;

void test_imu_create(lv_obj_t *parent) {
    ESP_LOGI(TAG, "IMU test started");
    
    lv_obj_t *screen = test_menu_create_test_screen("IMU Test");
    
    lv_obj_t *acc_title = lv_label_create(screen);
    lv_label_set_text(acc_title, "Accelerometer (g)");
    lv_obj_set_style_text_color(acc_title, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_pad_bottom(acc_title, 5, 0);
    
    acc_x_label = lv_label_create(screen);
    acc_y_label = lv_label_create(screen);
    acc_z_label = lv_label_create(screen);
    lv_label_set_text(acc_x_label, "X: --");
    lv_label_set_text(acc_y_label, "Y: --");
    lv_label_set_text(acc_z_label, "Z: --");
    lv_obj_set_style_text_color(acc_x_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(acc_y_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(acc_z_label, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t *gyr_title = lv_label_create(screen);
    lv_label_set_text(gyr_title, "Gyroscope (dps)");
    lv_obj_set_style_text_color(gyr_title, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_pad_top(gyr_title, 10, 0);
    lv_obj_set_style_pad_bottom(gyr_title, 5, 0);
    
    gyr_x_label = lv_label_create(screen);
    gyr_y_label = lv_label_create(screen);
    gyr_z_label = lv_label_create(screen);
    lv_label_set_text(gyr_x_label, "X: --");
    lv_label_set_text(gyr_y_label, "Y: --");
    lv_label_set_text(gyr_z_label, "Z: --");
    lv_obj_set_style_text_color(gyr_x_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(gyr_y_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(gyr_z_label, lv_color_hex(0xFFFFFF), 0);
    
    temp_label = lv_label_create(screen);
    lv_label_set_text(temp_label, "Temp: --");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_pad_top(temp_label, 10, 0);
    
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Status: Initializing...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_pad_top(status_label, 10, 0);
    
    if (imu_driver_is_available()) {
        lv_label_set_text(status_label, "Status: QMI8658C OK");
        ESP_LOGI(TAG, "IMU sensor available");
    } else {
        lv_label_set_text(status_label, "Status: Not found!");
        ESP_LOGW(TAG, "IMU sensor not found");
    }
}

void test_imu_update(void) {
    imu_data_t data;
    esp_err_t ret = imu_driver_read_all(&data);
    
    if (ret == ESP_OK) {
        char buf[32];
        snprintf(buf, sizeof(buf), "X: %.2f", data.x);
        lv_label_set_text(acc_x_label, buf);
        snprintf(buf, sizeof(buf), "Y: %.2f", data.y);
        lv_label_set_text(acc_y_label, buf);
        snprintf(buf, sizeof(buf), "Z: %.2f", data.z);
        lv_label_set_text(acc_z_label, buf);
        
        imu_gyr_data_t gyr;
        imu_driver_read_gyr(&gyr);
        snprintf(buf, sizeof(buf), "X: %.1f", gyr.x);
        lv_label_set_text(gyr_x_label, buf);
        snprintf(buf, sizeof(buf), "Y: %.1f", gyr.y);
        lv_label_set_text(gyr_y_label, buf);
        snprintf(buf, sizeof(buf), "Z: %.1f", gyr.z);
        lv_label_set_text(gyr_z_label, buf);
        
        snprintf(buf, sizeof(buf), "Temp: %.1fC", data.temperature);
        lv_label_set_text(temp_label, buf);
    }
}
