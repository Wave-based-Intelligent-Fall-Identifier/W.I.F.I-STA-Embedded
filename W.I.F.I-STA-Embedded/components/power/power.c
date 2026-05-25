#include "power.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define PIN_NUM GPIO_NUM_13

static const char* TAG = "Power";

void power_ctrl(void) {  
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    while (1) {
        if (gpio_get_level(PIN_NUM) == 1) {
            vTaskDelay(pdMS_TO_TICKS(50)); 
            if (gpio_get_level(PIN_NUM) == 1) {
                esp_deep_sleep_enable_ext0_wakeup(PIN_NUM, 0);
                esp_deep_sleep_start();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}