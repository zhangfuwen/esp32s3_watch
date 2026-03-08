/**
 * @file event_bus.c
 * @brief 事件总线实现
 */

#include "event_bus.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "EVENT_BUS";

#define MAX_HANDLERS_PER_EVENT 5
#define EVENT_QUEUE_SIZE 20

typedef struct {
    event_handler_t handlers[MAX_HANDLERS_PER_EVENT];
    uint8_t count;
} handler_list_t;

static handler_list_t handlers[EVENT_APP_SHOW_SETTINGS + 1];
static QueueHandle_t event_queue = NULL;
static TaskHandle_t event_task_handle = NULL;

static void event_task(void *pvParameters) {
    event_t event;
    while (1) {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY)) {
            if (event.type <= EVENT_APP_SHOW_SETTINGS) {
                handler_list_t *list = &handlers[event.type];
                for (int i = 0; i < list->count; i++) {
                    if (list->handlers[i]) {
                        list->handlers[i](&event);
                    }
                }
            }
        }
    }
}

esp_err_t event_bus_init(void) {
    memset(handlers, 0, sizeof(handlers));
    
    event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(event_t));
    if (!event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    
    BaseType_t ret = xTaskCreate(event_task, "event_task", 2048, NULL, 5, &event_task_handle);
    if (ret != pdPASS) {
        vQueueDelete(event_queue);
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Event bus initialized");
    return ESP_OK;
}

esp_err_t event_bus_subscribe(event_type_t type, event_handler_t handler) {
    if (type > EVENT_APP_SHOW_SETTINGS || !handler) {
        return ESP_ERR_INVALID_ARG;
    }
    
    handler_list_t *list = &handlers[type];
    if (list->count >= MAX_HANDLERS_PER_EVENT) {
        return ESP_ERR_NO_MEM;
    }
    
    list->handlers[list->count++] = handler;
    return ESP_OK;
}

esp_err_t event_bus_unsubscribe(event_type_t type, event_handler_t handler) {
    if (type > EVENT_APP_SHOW_SETTINGS || !handler) {
        return ESP_ERR_INVALID_ARG;
    }
    
    handler_list_t *list = &handlers[type];
    for (int i = 0; i < list->count; i++) {
        if (list->handlers[i] == handler) {
            // Shift remaining handlers
            for (int j = i; j < list->count - 1; j++) {
                list->handlers[j] = list->handlers[j + 1];
            }
            list->count--;
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t event_bus_publish(const event_t *event) {
    if (!event || !event_queue) {
        return ESP_ERR_INVALID_ARG;
    }
    
    event_t evt = *event;
    evt.timestamp = xTaskGetTickCount();
    
    if (xQueueSend(event_queue, &evt, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full, dropping event %d", event->type);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t event_bus_publish_simple(event_type_t type) {
    event_t event = {
        .type = type,
        .timestamp = 0,
        .data_u32 = 0
    };
    return event_bus_publish(&event);
}
