#ifndef CSI_RING_BUFFER_H
#define CSI_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

//링 버퍼 사이즈 (8도 ㄱㄴ)
#define CSI_RING_BUFFER_SIZE 16
#define CSI_DATA_MAX_LEN 64

typedef struct
{
    int8_t csi_data[CSI_DATA_MAX_LEN];
    uint8_t len;
} csi_packet_t;

typedef struct {
    csi_packet_t buffer[CSI_RING_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    portMUX_TYPE mux;
} csi_ring_buffer_t;

/**
 * @brief ring buffer 초기화 함수
 * @param[in] csi_ring_buffer_t *rb
 * @retval None
 */
void csi_ring_buffer_init(csi_ring_buffer_t *rb);

/**
 * @brief CSI 데이터 저장 함수
 * @param[in] csi_ring_buffer_t *rb, const csi_packet_t *packet
 * @retval bool 저장 성공/NULL
 *  1. 저장 성공 : 1
 *  2. 가득 참/NULL : 0
 */
bool csi_ring_buffer_push(csi_ring_buffer_t *rb, const csi_packet_t *packet);

/**
 * @brief 퍼버에서 데이터 꺼내는 함수
 * @param[in] csi_ring_buffer_t *rb, csi_packet_t *packet
 * @retval bool
 *  1. 읽기 성공 : 1
 *  2. 비어있음/NULL : 0
 */
bool csi_ring_buffer_pop(csi_ring_buffer_t *rb, csi_packet_t *packet);

/**
 * @brief 버퍼 데이터 empty 확인 여부
 * @param[in] csi_ring_buffer_t *rb
 * @retval bool
 *  1. 비어있음 : 1
 *  2. 저장 공간 있음/NULL : 0
 */
bool csi_ring_buffer_is_empty(csi_ring_buffer_t *rb);

/**
 * @brief 버퍼 데이터 full 확인 함수
 * @param[in] csi_ring_buffer_t *rb
 * @retval bool
 *  1. 가득참 : 1
 *  2. 저장 공간 있음/NULL : 0
 */
bool csi_ring_buffer_is_full(csi_ring_buffer_t *rb);

#endif