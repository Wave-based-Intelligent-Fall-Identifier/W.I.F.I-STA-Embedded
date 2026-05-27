#pragma once 

#include <stdio.h>
#include <string.h>
#include <math.h>

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
#define  FAIL_BIT           BIT4

#define WIFI_SSID      "k"
#define WIFI_PASS      "ericeric0223"
#define MAXIMUM_RETRY  5

typedef struct {
    uint8_t len;
    int8_t raw_data[128]; 
} csi_packet_t;

extern QueueHandle_t csi_queue;

/**
 * @brief Wi-Fi 초기화 함수
 * @param[in] None
 * @retval esp_err_t 오류코드 반환
 */
esp_err_t wifiInit(void);

/**
 * @brief CSI 수신 데이터 큐 저장 함수
 * @param[in] void *ctx, wifi_csi_info_t *data
 * @retval None
 */
void csi_callback(void *ctx, wifi_csi_info_t *data);

/**
 * @brief CSI 데이터 계산(raw CSI를 amplitude로 바꿈), 출력 함수
 * @param[in] void* pvParameters
 * @retval None
 */
void csi_data_calculate(void* pvParameters);