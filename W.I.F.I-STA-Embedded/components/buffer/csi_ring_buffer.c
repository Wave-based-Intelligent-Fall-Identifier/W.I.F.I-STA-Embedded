#include "csi_ring_buffer.h"
#include <string.h>

void csi_ring_buffer_init(csi_ring_buffer_t *rb)
{
    if (rb == NULL) {
        return;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->mux = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    memset(rb->buffer, 0, sizeof(rb->buffer));
}

bool csi_ring_buffer_push(csi_ring_buffer_t *rb, const csi_packet_t *packet)
{
    if (rb == NULL || packet == NULL) {
        return false;
    }

    bool result = false;

    portENTER_CRITICAL(&rb->mux);

    if (rb->count < CSI_RING_BUFFER_SIZE) {
        rb->buffer[rb->head] = *packet;
        rb->head = (rb->head + 1) % CSI_RING_BUFFER_SIZE;
        rb->count++;
        result = true;
    }

    portEXIT_CRITICAL(&rb->mux);

    return result;
}

bool csi_ring_buffer_pop(csi_ring_buffer_t *rb, csi_packet_t *packet)
{
    if (rb == NULL || packet == NULL) {
        return false;
    }

    bool result = false;

    portENTER_CRITICAL(&rb->mux);

    if (rb->count > 0) {
        *packet = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % CSI_RING_BUFFER_SIZE;
        rb->count--;
        result = true;
    }

    portEXIT_CRITICAL(&rb->mux);

    return result;
}

bool csi_ring_buffer_is_empty(csi_ring_buffer_t *rb)
{
    if (rb == NULL) {
        return true;
    }

    bool result;

    portENTER_CRITICAL(&rb->mux);
    result = (rb->count == 0);
    portEXIT_CRITICAL(&rb->mux);

    return result;
}

bool csi_ring_buffer_is_full(csi_ring_buffer_t *rb)
{
    if (rb == NULL) {
        return false;
    }

    bool result;

    portENTER_CRITICAL(&rb->mux);
    result = (rb->count >= CSI_RING_BUFFER_SIZE);
    portEXIT_CRITICAL(&rb->mux);

    return result;
}