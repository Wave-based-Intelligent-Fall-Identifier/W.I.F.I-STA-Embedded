#include "baseline_filter.h"

#define BASELINE_SAMPLE_COUNT 100

static float baseline_sum = 0.0f;
static float baseline_value = 0.0f;
static int sameple_count = 0;
static bool baseline_ready = false;

void baseline_init(void) {
    float baseline_sum = 0.0f;
    float baseline_value = 0.0f;
    int sameple_count = 0;
    bool baseline_ready = false;
}

void baseline_update(float value) {
    if (baseline_ready) {
        return;
    }

    baseline_sum += value;
    sample_count++;

    if (sample_count >= BASELINE_SAMPLE_COUNT) {
        baseline_value = baseline_sum / sample_count;
        baseline_ready = true;
    }
}

float baseline_apply(float value) {
    if (!baseline_ready) {
        return 0.0f;
    }

    return value - baseline_value;
}

bool baseline_is_ready(void) {
    return baseline_ready;
}
