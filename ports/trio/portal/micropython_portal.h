/*
====================== Micropython terminology =======================

Interface:
   Currently the naming is a mess and will be looked at but in general
   Micropython and PascalCase indicates that a function is from the
   firmware side whereas upy and snake_case indicates that a function
   is from the Micropython side.

Heap:
   In Micropython this refers to an arena given to Micropython managed
   by the garbage collector (gc), not the std malloc and free heap.

   ! Many standard library memory functions (malloc, free, etc.) are
   redefined as macros in terms of this 'heap'

   The Micropython heap isn't only used by a running Python process,
   everything uses it e.g. the lexer, the parser, etc.

Qstr:
   Qstrings are interned strings - strings added to a central lookup
   table that can be referenced by an ID. These can be created at
   compile time thanks to a pre-compilation script with any identifier
   beginning with MP_QSTR_ evaluating to a reference to one e.g. 
   MP_QSTR__lt_stdin_gt_ will at runtime reference an interned string 
   with value '<stdin>'. They can also be created at runtime e.g. to 
   store a file name that has been imported which may later be used in
   an error message.

Vstr:
   Vstrings are dynamic strings (stored on the Micropython heap, not
   the 'real' heap). 

Nlr:
   Non-local returns - the core of Micropythons exceptions handling.
   While by default these are implemented with assembly per-platform,
   this is currently disabled and setjmp is used instead.

   In general you can assume that any calls to mp_ functions may not
   return control to you, unless you wrap your code like this:

   -------------------------------------------------------------------
   nlr_buf_t nlr;
   if (nlr_push(&nlr) == 0) {
      // Code
      nlr_pop();
   }
   else {
      // Exception handling
   }
   -------------------------------------------------------------------

   If you want to run cleanup code but don't want to catch the
   exception (example from parse.c):

   -------------------------------------------------------------------
   // Register cleanup callback
   MP_DEFINE_NLR_JUMP_CALLBACK_FUNCTION_1(ctx, mp_lexer_free, lex);
   nlr_push_jump_callback(&ctx.callback, mp_call_function_1_from_nlr_jump_callback);

   // Deregister callback (true indicates to also run the callback)
   nlr_pop_jump_callback(true);
   -------------------------------------------------------------------

mpconfig.h:
   mpconfig.h contains all the configuration options for Micropython
   and what they do. mpconfigport.h changes these from their
   defaults.
*/

#ifndef MICROPYTHON_PORTAL_H
#define MICROPYTHON_PORTAL_H

#include <stdlib.h>
#include <stdbool.h>

#include "../mpconfigtypes.h"

typedef struct {
    mp_uint_t(*mp_hal_stdout_tx_strn)(const char* str, size_t len);
    mp_uint_t(*mp_hal_ticks_us)(void);
    mp_uint_t(*mp_hal_ticks_cpu)(void);
    void(*mp_hal_delay_ms)(mp_uint_t ms);
} HalFunctions;

// void set_hal_functions(HalFunctions hal_functions);


// TODO: This is very janky but we can't include mp_state_ctx_t
// directly as it uses Micropython imports (e.g. py/xyz), not
// Trio firmware ones.

// Representation of mp_state_ctx_t 692 bytes - aligned to 4 bytes
typedef struct {
    mp_uint_t __dummy[2048 / sizeof(mp_uint_t)];
} upy_ctx;

typedef struct {
   void* start;
   void* end;
} heap_def_t;

extern mp_uint_t MpHalStdoutTxStrnTrio(const char* str, size_t len);
extern mp_uint_t MpHalTicksUsTrio(void);
extern mp_uint_t MpHalTicksCpuTrio(void);
extern void MpHalDelayMsTrio(mp_uint_t delay);
extern upy_ctx* GetMpStateCtxTrio(void);
extern heap_def_t GetMpHeapDefTrio(void);
extern mp_uint_t GetStackSizeTrio(void);

typedef struct {
   bool continueExec;
   char* readVariable;
   char** variableOut;
} LineChangeResponse;

extern bool MicropythonShouldPause(const char* fileName, size_t lineNo);
extern bool MicropythonStayPaused(void);

typedef enum {
   upy_import_stat_dir,
   upy_import_stat_file,
   upy_import_stat_no_exist,
} upy_import_stat_t;

extern upy_import_stat_t GetImportStatTrio(const char* path);

//extern mp_int_t MicropythonReadPythonFileLength(const char* filename);
//extern mp_uint_t MicropythonReadPythonFileName(const char* filename, char* buf);
extern mp_uint_t MicropythonGetTrioSrc(const char* filename, const char** outSrcStart, const char** outSrcEnd);
extern mp_uint_t MicropythonGetTrioObj(const char* filename, const char** outObjStart, const char** outObjEnd);

extern mp_uint_t MicropythonSaveTrioObj(const char* filename, const char* data, size_t length);

extern void MicropythonSetException(const char* file, const char* exception, size_t line, bool compilationError);

// Compiles Python file - must be called from within a Micropython environment
// IMPORTANT: Handles exceptions using Micropython nlr - 
//     if an execptions is thrown during compilation, control
//     will not return to the calling code!
void upy_compile_code(const char* filename, const char* src_start, const char* src_end);

// Compiles Python files without requiring an existing Micropython environment,
// state, etc.
// Will output any exception encountered with MicropythonSetException
// IMPORTANT: This overwrite the current job's state!
bool upy_compile_code_no_env(const char* filename, const char* src_start, const char* src_end);

// Compiles and runs provided Python code, saving debug information as it goes.
// Imports should always return .py files, not .mpy
// Will output uncaught exceptions with MicropythonSetException
int upy_run_debug(const char* file_name, const char* src_start, const char* src_end);

// Runs provided Python bytecode
// Imports should always return .mpy files, not .py
// Will output uncaught exceptions with MicropythonSetException
int upy_run(const char* file_name, const char* obj_start, const char* obj_end);

// Permanently stop the VM
void upy_abort(upy_ctx *ctx);

// Accesses a variable currently in scope
// IMPORTANT:
//    - The VM must be paused
//    - The string returned will become invalid upon VM resumption
const char* upy_access_variable(const char* var_name);

#endif