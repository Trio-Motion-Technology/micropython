#include "win_hal.h"

#include "mpconfigport.h"
#include "trio_mphal.h"

#include "py/mpthread.h"

#include <io.h>
#include <sys/time.h>
#include <windows.h>
#include <stdint.h>

//#define F_OK 0
//#define W_OK 2
//#define R_OK 4
//
//#define STDIN_FILENO  0
#define STDOUT_FILENO 1
//#define STDERR_FILENO 2
//
//#define SEEK_CUR 1
//#define SEEK_END 2
//#define SEEK_SET 0

mp_uint_t w_mp_hal_stdout_tx_strn(const char* str, size_t len) {
    MP_THREAD_GIL_EXIT(); // Doesn't do anything without (experimental) threading enabled
    int ret = write(STDOUT_FILENO, str, len);
    MP_THREAD_GIL_ENTER();
    return ret < 0 ? 0 : ret; // return the number of bytes written, so in case of an error in the syscall, return 0
}

mp_uint_t w_mp_hal_ticks_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

mp_uint_t w_mp_hal_ticks_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t w_mp_hal_time_ns(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
}

mp_uint_t w_mp_hal_ticks_cpu(void) {
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    #ifdef _WIN64
    return value.QuadPart;
    #else
    return value.LowPart;
    #endif
}


void msec_sleep(double msec) {
    if (msec < 0.0) {
        msec = 0.0;
    }
    SleepEx((DWORD)msec, TRUE);
}

void w_mp_hal_delay_ms(mp_uint_t ms) {
#if MICROPY_ENABLE_SCHEDULER
    mp_uint_t start = mp_hal_ticks_ms();
    while (mp_hal_ticks_ms() - start < ms) {
        mp_event_wait_ms(1);
    }
#else
    msec_sleep((double)ms);
#endif
}