#ifndef WIN_HAL_H
#define WIN_HAL_H

#include "trio_mphal.h"

mp_uint_t w_mp_hal_stdout_tx_strn(const char* str, size_t len);
mp_uint_t w_mp_hal_ticks_us(void);
mp_uint_t w_mp_hal_ticks_cpu(void);
void w_mp_hal_delay_ms(mp_uint_t ms);

static inline HalFunctions win_hal_functions() {
    HalFunctions f = {
        .mp_hal_stdout_tx_strn = w_mp_hal_stdout_tx_strn,
        .mp_hal_ticks_us = w_mp_hal_ticks_us,
        .mp_hal_ticks_cpu = w_mp_hal_ticks_cpu,
        .mp_hal_delay_ms = w_mp_hal_delay_ms,
    };
    return f;
}

#endif // !WIN_HAL_H