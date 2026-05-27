#ifndef BASELINE_FILTER_H
#define BASELINE_FILTER_H

#include <stdbool.h>

/**
 * @brief baseline 초기화 함수
 * @param[in] None
 * @retval None
 */
void baseline_init(void);

/**
 * @brief baseline 값 업데이트 함수
 * @param[in] float value
 * @retval None
 */
void baseline_update(float value);

/**
 * @brief baseline 감산 함수
 * @param[in] float value
 * @retval float 현재 측정 값과 베이스 라인 차이
 */
float baseline_apply(float value);

/**
 * @brief baseline이 다 생성 됐는지 확인하는 함수
 * @param[in] None
 * @retval bool baseline 생성 여부
 *  1. 준비됨 : 1
 *  2. 추가 업데이트 필요 : 0
 */
bool baseline_is_ready(void);

#endif