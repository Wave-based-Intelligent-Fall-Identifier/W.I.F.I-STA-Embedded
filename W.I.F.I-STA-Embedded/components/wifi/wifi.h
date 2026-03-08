#pragma once 

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_flash.h"

#define  CONNECTED_BIT      BIT0
#define  GOT_IP_BIT         BIT2

#define WIFI_SSID      "k"
#define WIFI_PASS      "ericeric0223"
#define MAXIMUM_RETRY  5

esp_err_t wifiInit();
static void wifiHandler(void *args, esp_event_base_t eventBase, int32_t eventId, void* eventData);