/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TRIO_MPHAL_H
#define TRIO_MPHAL_H

#include "py/mpconfig.h"

// #include "sleep.h"
#include "py/reader.h"

#include <errno.h>
#include <unistd.h>

#ifndef CHAR_CTRL_C
#define CHAR_CTRL_C (3)
#endif

// void set_hal_functions_int(HalFunctions hal_functions);

// HalFunctions* get_hal_functions();

 // In lieu of a WFI(), slow down polling from being a tight loop.
 //
 // Note that we don't delay for the full TIMEOUT_MS, as execution
 // can't be woken from the delay.
#define MICROPY_INTERNAL_WFE(TIMEOUT_MS) \
    do { \
        MP_THREAD_GIL_EXIT(); \
        mp_hal_delay_us(500); \
        MP_THREAD_GIL_ENTER(); \
    } while (0)

void mp_hal_set_interrupt_char(char c);

#define mp_hal_stdio_poll unused // this is not implemented, nor needed
void mp_hal_stdio_mode_raw(void);
void mp_hal_stdio_mode_orig(void);

//static inline void mp_hal_delay_us(mp_uint_t us) {
//    usleep(us);
//}

#define RAISE_ERRNO(err_flag, error_val) \
    { if (err_flag == -1) \
      { mp_raise_OSError(error_val); } }

void mp_hal_get_random(size_t n, void* buf);

// Don't use the unix version of this macro.
#undef MICROPY_INTERNAL_WFE

#if MICROPY_ENABLE_SCHEDULER
// Use minimum 1mSec sleep to make sure there is effectively a wait period:
// something like usleep(500) truncates and ends up calling Sleep(0).
#define MICROPY_INTERNAL_WFE(TIMEOUT_MS) msec_sleep(MAX(1.0, (double)(TIMEOUT_MS)))
#else
#define MICROPY_INTERNAL_WFE(TIMEOUT_MS) /* No-op */
#endif

#define MICROPY_HAL_HAS_VT100 (0)

void mp_hal_move_cursor_back(unsigned int pos);
void mp_hal_erase_line_from_cursor(unsigned int n_chars_to_erase);

mp_uint_t mp_hal_ticks_cpu(void);
void mp_hal_delay_ms(mp_uint_t ms);

static inline void mp_hal_delay_us(mp_uint_t us) {
   mp_hal_delay_ms(us / 1000);
}

void mp_reader_new_trio_src(mp_reader_t* reader, const char* src_start, const char* src_end);
void mp_reader_new_trio_obj(mp_reader_t* reader, const char* src_start, const char* src_end);
void mp_reader_new_file(mp_reader_t* reader, qstr filename);


#endif // !TRIO_MPHAL_H