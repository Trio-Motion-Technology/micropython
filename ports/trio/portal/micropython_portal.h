#ifndef MICROPYTHON_PORTAL_H
#define MICROPYTHON_PORTAL_H

#include "../mpconfigtypes.h"

typedef struct {
    mp_uint_t(*mp_hal_stdout_tx_strn)(const char* str, size_t len);
    mp_uint_t(*mp_hal_ticks_us)(void);
    mp_uint_t(*mp_hal_ticks_cpu)(void);
    void(*mp_hal_delay_ms)(mp_uint_t ms);
} HalFunctions;

void set_hal_functions(HalFunctions hal_functions);

int start_upy();
int start_upy_single(const char* src);

#endif