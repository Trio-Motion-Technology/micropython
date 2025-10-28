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

// TODO: Clearly mark which functions require being run from a thread
//    with a Micropython context

#ifndef MICROPYTHON_PORTAL_H
#define MICROPYTHON_PORTAL_H

#include <stdlib.h>
#include <stdbool.h>

#include "../mpconfigtypes.h"

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

extern bool MicropythonShouldPause(const char* fileName, size_t lineNo);
extern bool MicropythonStayPaused(const char* fileName);

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

// Initialises some state allowing the VM's abort flag to be set if needed
// 1. Wait for process state to be READY, in this time upy_abort cannot be used
// 2. Lock process resources, call upy_init, set active = TRUE
// 3. upy_abort can now be used normally
void upy_init(void);

// Compiles and runs provided Python code, saving debug information as it goes.
// Imports should always return .py files, not .mpy
// Will output uncaught exceptions with MicropythonSetException
// IMPORTANT: upy_init must be called first!
void upy_run_debug(const char* file_name, const char* src_start, const char* src_end);

// Runs provided Python bytecode
// Imports should always return .mpy files, not .py
// Will output uncaught exceptions with MicropythonSetException
// IMPORTANT: upy_init must be called first!
void upy_run(const char* file_name, const char* obj_start, const char* obj_end);

// Permanently stop the VM
// IMPORTANT: Cannot be used before upy_init is called
void upy_abort(upy_ctx *ctx);

// TODO: Consider taking printer approach here too
typedef struct {
   const char* block_name;
   const char* file;
   size_t line;
} stack_trace_frame_t;

// Gets the stack trace
// IMPORTANT:
//    - The VM must be paused
//    - Qstrs are emitted in stack_trace_frame_t - these are only guaranteed to live as
//      long as this Python VM!
// 
// TODO: Is the posibility that a VM is stopped and restarted quickly enough after
//    getting the stack trace that the Qstrs are invalidated a problem worth solving?
size_t upy_get_stack_trace(stack_trace_frame_t* stack_trace_frames, size_t max_frames, bool* stack_truncated);

// Printers used below as opposed to buffers to save memory
//    -> Printing code not implemented on Python side for separation of concerns

typedef struct {
   void(*print)(void* data);
   void* data;
} noinput_printer_t;

typedef struct {
   void(*print_flt)(void* data, double value);
   void* data;
} float_printer_t;

typedef struct {
   void(*print_strn)(void* data, const char* str, size_t len);
   void* data;
} printer_t;

typedef struct {
   // Will call:
   //    1. type_printer
   //    2. terminator_printer
   //    3. separator_printer
   //    4. value_printer
   //    5. terminator_printer
   //    6. separator_printer
   //    7. scope_printer
   //    8. terminator_printer
   //
   // For supported types, steps 1-5 will be replaced by one call to [type]_printer
   printer_t type_printer;
   printer_t value_printer;
   printer_t scope_printer;
   noinput_printer_t terminator_printer;
   noinput_printer_t separator_printer;
   noinput_printer_t timeout_printer; // Will be called instead of terminator_printer on timeout

   // Type specific
   float_printer_t float_printer;
} lookup_printers_t;

// Accesses a variable in the current or global scope. 
// IMPORTANT:
//    - The VM must be paused
//    - No dot-separated part of var_name may be longer than max_part_length. If it is, this will
//       always return false
//    - max_part_length bytes will be allocated on the stack - a too big value will cause a stack overflow
bool upy_access_variable(const char* var_name, size_t max_part_length, lookup_printers_t lookup_printers, int type_support);

typedef struct {
   // Will call:
   //    1. attribute_printer
   //    2. terminator_printer
   //    -- Stops here if no more attributes
   //    -- Jumps to overflow_printer if max_attrib exceeded
   //    3. separator_printer
   //    4. loop
   printer_t attribute_printer;
   noinput_printer_t separator_printer;
   noinput_printer_t terminator_printer;

   noinput_printer_t overflow_printer;
} attribute_printers_t;

// Lists the attributes on a variable. Will return 0 if the value has no attributes, and -1 if the value couldn't be found.
// Important:
//    - The VM must be paused
//    - No dot-separated part of var_name may be longer than max_part_length. If it is, this will
//       always return false
//    - max_part_length bytes will be allocated on the stack - a too big value will cause a stack overflow
int upy_list_attributes(const char* var_name, size_t max_part_length, attribute_printers_t attrib_printers, size_t max_attrib, size_t skip);

#endif