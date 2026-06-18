#pragma once

#include "esp_err.h"

/**
 * @brief ESP-NOW 초기화 함수 (브로드캐스트 피어 등록)
 * @param[in] None
 * @retval esp_err_t 오류코드 반환
 */
esp_err_t espnow_init_setup(void);

/**
 * @brief 페어링 요청 브로드캐스트 송신 함수
 * @param[in] None
 * @retval esp_err_t 오류코드 반환
 */
esp_err_t espnow_send_pairing_request(void);
