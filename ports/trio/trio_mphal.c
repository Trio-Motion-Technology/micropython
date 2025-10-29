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


#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpthread.h"
#include "py/mpstate.h"
#include "py/builtin.h"
#include "py/lexer.h"
#include "py/mperrno.h"

#include "portal/micropython_portal.h"

// static HalFunctions static_hal_functions;

// void set_hal_functions_int(HalFunctions hal_functions) {
//     static_hal_functions = hal_functions;
// }

// HalFunctions* get_hal_functions() {
//     return &static_hal_functions;
// }

//HANDLE std_in = NULL;
//HANDLE con_out = NULL;
//DWORD orig_mode = 0;

//static void assure_stdin_handle() {
//    if (!std_in) {
//        std_in = GetStdHandle(STD_INPUT_HANDLE);
//        assert(std_in != INVALID_HANDLE_VALUE);
//    }
//}
//
//static void assure_conout_handle() {
//    if (!con_out) {
//        con_out = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE,
//            FILE_SHARE_READ | FILE_SHARE_WRITE,
//            NULL, OPEN_EXISTING, 0, 0);
//        assert(con_out != INVALID_HANDLE_VALUE);
//    }
//}

void mp_hal_stdio_mode_raw(void) {
    /*assure_stdin_handle();
    GetConsoleMode(std_in, &orig_mode);
    DWORD mode = orig_mode;
    mode &= ~ENABLE_ECHO_INPUT;
    mode &= ~ENABLE_LINE_INPUT;
    mode &= ~ENABLE_PROCESSED_INPUT;
    SetConsoleMode(std_in, mode);*/
}

void mp_hal_stdio_mode_orig(void) {
    /*assure_stdin_handle();
    SetConsoleMode(std_in, orig_mode);*/
}

// Handler to be installed by SetConsoleCtrlHandler, currently used only to handle Ctrl-C.
// This handler has to be installed just once (this has to be done elsewhere in init code).
// Previous versions of the mp_hal code would install a handler whenever Ctrl-C input is
// allowed and remove the handler again when it is not. That is not necessary though (1),
// and it might introduce problems (2) because console notifications are delivered to the
// application in a separate thread.
// (1) mp_hal_set_interrupt_char effectively enables/disables processing of Ctrl-C via the
// ENABLE_PROCESSED_INPUT flag so in raw mode console_sighandler won't be called.
// (2) if mp_hal_set_interrupt_char would remove the handler while Ctrl-C was issued earlier,
// the thread created for handling it might not be running yet so we'd miss the notification.
//BOOL WINAPI console_sighandler(DWORD evt) {
//    if (evt == CTRL_C_EVENT) {
//        if (MP_STATE_MAIN_THREAD(mp_pending_exception) == MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception))) {
//            // this is the second time we are called, so die straight away
//            exit(1);
//        }
//        mp_sched_keyboard_interrupt();
//        return TRUE;
//    }
//    return FALSE;
//}

void mp_hal_set_interrupt_char(char c) {
    /*assure_stdin_handle();
    if (c == CHAR_CTRL_C) {
        DWORD mode;
        GetConsoleMode(std_in, &mode);
        mode |= ENABLE_PROCESSED_INPUT;
        SetConsoleMode(std_in, mode);
    } else {
        DWORD mode;
        GetConsoleMode(std_in, &mode);
        mode &= ~ENABLE_PROCESSED_INPUT;
        SetConsoleMode(std_in, mode);
    }*/
}

void mp_hal_move_cursor_back(unsigned int pos) {
    //if (!pos) {
    //    return;
    //}
    //assure_conout_handle();
    //CONSOLE_SCREEN_BUFFER_INFO info;
    //GetConsoleScreenBufferInfo(con_out, &info);
    //info.dwCursorPosition.X -= (short)pos;
    //// Move up a line if needed.
    //while (info.dwCursorPosition.X < 0) {
    //    info.dwCursorPosition.X = info.dwSize.X + info.dwCursorPosition.X;
    //    info.dwCursorPosition.Y -= 1;
    //}
    //// Caller requested to move out of the screen. That's not possible so just clip,
    //// it's the caller's responsibility to not let this happen.
    //if (info.dwCursorPosition.Y < 0) {
    //    info.dwCursorPosition.X = 0;
    //    info.dwCursorPosition.Y = 0;
    //}
    //SetConsoleCursorPosition(con_out, info.dwCursorPosition);
}

void mp_hal_erase_line_from_cursor(unsigned int n_chars_to_erase) {
    /*assure_conout_handle();
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(con_out, &info);
    DWORD written;
    FillConsoleOutputCharacter(con_out, ' ', n_chars_to_erase, info.dwCursorPosition, &written);
    FillConsoleOutputAttribute(con_out, info.wAttributes, n_chars_to_erase, info.dwCursorPosition, &written);*/
}

//typedef struct item_t {
//    WORD vkey;
//    const char *seq;
//} item_t;

// map virtual key codes to key sequences known by MicroPython's readline implementation
//static item_t keyCodeMap[] = {
//    {VK_UP, "[A"},
//    {VK_DOWN, "[B"},
//    {VK_RIGHT, "[C"},
//    {VK_LEFT, "[D"},
//    {VK_HOME, "[H"},
//    {VK_END, "[F"},
//    {VK_DELETE, "[3~"},
//    {0, ""} // sentinel
//};

// likewise, but with Ctrl key down
//static item_t ctrlKeyCodeMap[] = {
//    {VK_LEFT, "b"},
//    {VK_RIGHT, "f"},
//    {VK_DELETE, "d"},
//    {VK_BACK, "\x7F"},
//    {0, ""} // sentinel
//};

//static const char *cur_esc_seq = NULL;

//static int esc_seq_process_vk(WORD vk, bool ctrl_key_down) {
//    for (item_t *p = (ctrl_key_down ? ctrlKeyCodeMap : keyCodeMap); p->vkey != 0; ++p) {
//        if (p->vkey == vk) {
//            cur_esc_seq = p->seq;
//            return 27; // ESC, start of escape sequence
//        }
//    }
//    return 0; // nothing found
//}

//static int esc_seq_chr() {
//    if (cur_esc_seq) {
//        const char c = *cur_esc_seq++;
//        if (c) {
//            return c;
//        }
//        cur_esc_seq = NULL;
//    }
//    return 0;
//}

int mp_hal_stdin_rx_chr(void) {
    // currently processing escape seq?
    //const int ret = esc_seq_chr();
    //if (ret) {
    //    return ret;
    //}

    //// poll until key which we handle is pressed
    //assure_stdin_handle();
    //BOOL status;
    //DWORD num_read;
    //INPUT_RECORD rec;
    //for (;;) {
    //    MP_THREAD_GIL_EXIT();
    //    status = ReadConsoleInput(std_in, &rec, 1, &num_read);
    //    MP_THREAD_GIL_ENTER();
    //    if (!status || !num_read) {
    //        return CHAR_CTRL_C; // EOF, ctrl-D
    //    }
    //    if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) { // only want key down events
    //        continue;
    //    }
    //    const bool ctrl_key_down = (rec.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) ||
    //        (rec.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED);
    //    const int ret = esc_seq_process_vk(rec.Event.KeyEvent.wVirtualKeyCode, ctrl_key_down);
    //    if (ret) {
    //        return ret;
    //    }
    //    const char c = rec.Event.KeyEvent.uChar.AsciiChar;
    //    if (c) { // plain ascii char, return it
    //        return c;
    //    }
    //}
    return CHAR_CTRL_C;
}

extern mp_uint_t MpHalStdoutTxStrnTrio(const char* str, size_t len);
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    //MP_THREAD_GIL_EXIT(); // Doesn't do anything without (experimental) threading enabled
    //int ret = write(STDOUT_FILENO, str, len);
    //MP_THREAD_GIL_ENTER();
    //return ret < 0 ? 0 : ret; // return the number of bytes written, so in case of an error in the syscall, return 0
    // return get_hal_functions()->mp_hal_stdout_tx_strn(str, len);
    return MpHalStdoutTxStrnTrio(str, len);
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    mp_hal_stdout_tx_strn(str, len);
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

extern mp_uint_t MpHalTicksUsTrio(void);

mp_uint_t mp_hal_ticks_us(void) {
    /*struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;*/
    // return get_hal_functions()->mp_hal_ticks_us();
    return MpHalTicksUsTrio();
}

mp_uint_t mp_hal_ticks_ms(void) {
    /*struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;*/
    return mp_hal_ticks_us() / 1000;
}

uint64_t mp_hal_time_ns(void) {
    //struct timeval tv;
    //gettimeofday(&tv, NULL);
    //return (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
    return ((uint64_t)mp_hal_ticks_us()) * 1000;
}

extern mp_uint_t MpHalTicksCpuTrio(void);
mp_uint_t mp_hal_ticks_cpu(void) {
    //LARGE_INTEGER value;
    //QueryPerformanceCounter(&value);
    //#ifdef _WIN64
    //return value.QuadPart;
    //#else
    //return value.LowPart;
    //#endif
    // return get_hal_functions()->mp_hal_ticks_cpu();
    return MpHalTicksCpuTrio();
}



//void msec_sleep(double msec) {
//    if (msec < 0.0) {
//        msec = 0.0;
//    }
//    SleepEx((DWORD)msec, TRUE);
//}
//
//#ifdef _MSC_VER
//int usleep(__int64 usec) {
//    msec_sleep((double)usec / 1000.0);
//    return 0;
//}
//#endif

extern void MpHalDelayMsTrio(mp_uint_t delay);
void mp_hal_delay_ms(mp_uint_t ms) {
    #if MICROPY_ENABLE_SCHEDULER
    mp_uint_t start = mp_hal_ticks_ms();
    while (mp_hal_ticks_ms() - start < ms) {
        mp_event_wait_ms(1);
    }
    #else
    //msec_sleep((double)ms);
    // get_hal_functions()->mp_hal_delay_ms(ms);
    
    bool abort_after = false;
    if (MP_STATE_VM(trio_debug) && MP_STATE_VM(trio_timeout_ms) != 0) {
       if (mp_hal_ticks_ms() > MP_STATE_VM(trio_timeout_ms)) {
          nlr_jump_timeout_abort();
          return;
       }
       else {
          mp_uint_t ttl = MP_STATE_VM(trio_timeout_ms) - mp_hal_ticks_ms();
          if (ttl < ms) {
             ms = ttl;
             abort_after = true;
          }
       }
    }

    MpHalDelayMsTrio(ms);

    if (abort_after) nlr_jump_timeout_abort();
    #endif
}

extern upy_ctx* GetMpStateCtxTrio(void);
mp_state_ctx_t* get_mp_state_ctx(void) {
   return (mp_state_ctx_t*) GetMpStateCtxTrio();
}

extern upy_import_stat_t GetImportStatTrio(const char* path);
mp_import_stat_t mp_import_stat(const char* path) {
    switch (GetImportStatTrio(path)) {
        case upy_import_stat_dir:
            return MP_IMPORT_STAT_DIR;
        case upy_import_stat_file:
            return MP_IMPORT_STAT_FILE;
        case upy_import_stat_no_exist:
            return MP_IMPORT_STAT_NO_EXIST;
    }
    return MP_IMPORT_STAT_NO_EXIST;
}

//extern mp_int_t MicropythonReadPythonFileLength(const char* filename);
//extern mp_uint_t MicropythonReadPythonFileName(const char* filename, char* buf);

/*
   Source file:
      [1 bytes] ?
      [1 bytes] Line length inc. 'header' and trailing 0x01
      [1 bytes] ? and whether there is a break
      [x bytes] (Line length -  3) bytes of line
*/
typedef struct {
   const byte* beg;
   const byte* cur;
   const byte* next_header;
   const byte* end;
} mp_reader_trio_src_t;

static mp_uint_t mp_reader_trio_src_readbyte(void* data) {
   mp_reader_trio_src_t* reader = (mp_reader_trio_src_t*)data;

   while (reader->cur == reader->next_header && reader->cur < reader->end) {
      bool first_line = reader->cur == reader->beg;

      reader->cur++;
      byte length = *reader->cur;
      reader->cur += 2; // Skip to first byte of line

      reader->next_header = reader->cur + (length - 3);

      if (!first_line) {
         return '\n'; // Insert newline
      }
   }

   if (reader->cur < reader->end) {
      return *reader->cur++;
   }
   else {
      return MP_READER_EOF;
   }
}

static void mp_reader_trio_src_close(void* data) {
   mp_reader_trio_src_t* reader = (mp_reader_trio_src_t*)data;
   m_del_obj(mp_reader_trio_src_t, reader);
}

void mp_reader_new_trio_src(mp_reader_t* reader, const char* src_start, const char* src_end) {
   mp_reader_trio_src_t* rm = m_new_obj(mp_reader_trio_src_t);
   rm->beg = src_start;
   rm->cur = src_start;
   rm->next_header = src_start;
   rm->end = src_end;
   reader->data = rm;
   reader->readbyte = mp_reader_trio_src_readbyte;
   reader->close = mp_reader_trio_src_close;
}

/*
   Object files: Just raw bytes
*/
typedef struct {
   const byte* beg;
   const byte* cur;
   const byte* end;
} mp_reader_trio_obj_t;

static mp_uint_t mp_reader_trio_obj_readbyte(void* data) {
   mp_reader_trio_obj_t* reader = (mp_reader_trio_obj_t*)data;

   if (reader->cur < reader->end) {
      //mp_printf(&mp_plat_print, "%d ", *reader->cur);
      //MpHalDelayMsTrio(1000);
      return *reader->cur++;
   }
   else {
      return MP_READER_EOF;
   }
}

static void mp_reader_trio_obj_close(void* data) {
   mp_reader_trio_obj_t* reader = (mp_reader_trio_obj_t*)data;
   m_del_obj(mp_reader_trio_obj_t, reader);
}

// Skips over Trio source bytes and inserts newlines
void mp_reader_new_trio_obj(mp_reader_t* reader, const char* src_start, const char* src_end) {
   mp_reader_trio_obj_t* rm = m_new_obj(mp_reader_trio_obj_t);
   rm->beg = src_start;
   rm->cur = src_start;
   rm->end = src_end;
   reader->data = rm;
   reader->readbyte = mp_reader_trio_obj_readbyte;
   reader->close = mp_reader_trio_obj_close;
}

mp_lexer_t* mp_lexer_new_from_file(qstr filename) {
   const char* fname = qstr_str(filename);

   const char* src_start;
   const char* src_end;
   mp_uint_t ret = MicropythonGetTrioSrc(fname, &src_start, &src_end);
   if (ret == -1) mp_raise_OSError(MP_EIO);
   mp_reader_t reader;
   mp_reader_new_trio_src(&reader, src_start, src_end);
   mp_lexer_t* lex = mp_lexer_new(filename, reader);

   return lex;
}

// Should really only be used in mp_raw_code_load_file on .mpy
// files but make it work for .py files because why not
void mp_reader_new_file(mp_reader_t* reader, qstr filename) {
   size_t flen = qstr_len(filename);
   const char* fname = qstr_str(filename);

   if (flen >= 3 && fname[flen - 3] == '.' && fname[flen - 2] == 'p' && fname[flen - 1] == 'y') {
      // Source
      const char* src_start;
      const char* src_end;
      mp_uint_t ret = MicropythonGetTrioSrc(fname, &src_start, &src_end);
      if (ret == -1) mp_raise_OSError(MP_EIO);
      mp_reader_new_trio_src(reader, src_start, src_end);
      return;
   }
   else if (flen >= 4 && fname[flen - 4] == '.' && fname[flen - 3] == 'm' && fname[flen - 2] == 'p' && fname[flen - 1] == 'y') {
      // Object
      const char* obj_start;
      const char* obj_end;
      mp_uint_t ret = MicropythonGetTrioObj(fname, &obj_start, &obj_end);
      if (ret == -1) mp_raise_OSError(MP_EIO);
      mp_reader_new_trio_obj(reader, obj_start, obj_end);
      return;
   }
   else {
      mp_raise_OSError(MP_EIO);
   }
}

//void mp_hal_get_random(size_t n, void *buf) {
//    NTSTATUS result = BCryptGenRandom(NULL, (unsigned char *)buf, n, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
//    if (!BCRYPT_SUCCESS(result)) {
//        mp_raise_OSError(errno);
//    }
//}
