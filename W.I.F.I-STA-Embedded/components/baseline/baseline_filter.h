#ifndef BASELINE_FILTER_H
#define BASELINE_FILTER_H

#include <stdbool.h>

void baseline_init(void);
void baseline_update(float value);
float baseline_apply(float value);
bool baseline_is_ready(void);

#endif