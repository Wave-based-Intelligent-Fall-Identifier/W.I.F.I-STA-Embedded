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
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_flash.h"

#define WIFI_SSID      "wify_csi_ap"
#define WIFI_PASS      "ekdus825"

typedef struct {
    uint8_t len;
    int8_t raw_data[128]; 
} csi_packet_t;

extern QueueHandle_t csi_queue;

/** AT(STA) 접속 여부 — true=접속됨, false=대기 중. wifiHandler 가 갱신. */
extern volatile bool at_connected;

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

/**
 * @brief AT(STA) 미접속 시 "AT 연결 대기 중..." 로그를 주기 출력하는 태스크
 * @param[in] void* pvParameters
 * @retval None
 */
void wait_at_task(void* pvParameters);