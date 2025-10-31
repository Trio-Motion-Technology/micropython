/*
====================== Micropython terminology =======================

Naming conventions:
   - snake_case functions prefixed with `upy_` are functions called by the firmware
      -> These are suffixed with `_CTX` if they require a micropython context

   - CamelCase functions prefixed with `Mpc` (MicroPython Call) are
      firmware fucntions called by micropython
      -> These are suffixed with `CTX` if they require a micropython context

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

#include <PythonInterface.h>

// Compiles Python file - must be called from within a Micropython environment
// IMPORTANT: Handles exceptions using Micropython nlr - 
//     if an execptions is thrown during compilation, control
//     will not return to the calling code!
void upy_compile_code_CTX(const char* filename, const char* src_start, const char* src_end);

// Compiles Python files without requiring an existing Micropython environment,
// state, etc.
// Will output any exception encountered with MicropythonSetException
// IMPORTANT: This overwrite the current job's state!
bool upy_compile_code_no_env(const char* filename, const char* src_start, const char* src_end);

// Initialises some state allowing the VM's abort flag to be set if needed
// 1. Wait for process state to be READY, in this time upy_abort cannot be used
// 2. Lock process resources, call upy_init_CTX, set active = TRUE
// 3. upy_abort can now be used normally
void upy_init_CTX(void);

// Compiles and runs provided Python code, saving debug information as it goes.
// Imports should always return .py files, not .mpy
// Will output uncaught exceptions with MicropythonSetException
// IMPORTANT: upy_init_CTX must be called first!
void upy_run_debug_CTX(const char* file_name, const char* src_start, const char* src_end);

// Runs provided Python bytecode
// Imports should always return .mpy files, not .py
// Will output uncaught exceptions with MicropythonSetException
// IMPORTANT: upy_init_CTX must be called first!
void upy_run_CTX(const char* file_name, const char* obj_start, const char* obj_end);

// Permanently stop the VM
// IMPORTANT: Cannot be used before upy_init_CTX is called
void upy_abort(upy_ctx *ctx);

// Gets the stack trace
// IMPORTANT:
//    - The VM must be paused
//    - Qstrs are emitted in stack_trace_frame_t - these are only guaranteed to live as
//      long as this Python VM!
// 
// TODO: Is the posibility that a VM is stopped and restarted quickly enough after
//    getting the stack trace that the Qstrs are invalidated a problem worth solving?
//       -> Could be solved by printers
size_t upy_get_stack_trace_CTX(stack_trace_frame_t* stack_trace_frames, size_t max_frames, bool* stack_truncated);



// Accesses a variable in the current or global scope. 
// IMPORTANT:
//    - The VM must be paused
//    - No dot-separated part of var_name may be longer than max_part_length. If it is, this will
//       always return false
//    - max_part_length bytes will be allocated on the stack - a too big value will cause a stack overflow
bool upy_access_variable_CTX(const char* var_name, size_t max_part_length, lookup_printers_t lookup_printers, int type_support, size_t value_timeout_ms);


// Lists the attributes on a variable. Will return 0 if the value has no attributes, and -1 if the value couldn't be found.
// Important:
//    - The VM must be paused
//    - No dot-separated part of var_name may be longer than max_part_length. If it is, this will
//       always return false
//    - max_part_length bytes will be allocated on the stack - a too big value will cause a stack overflow
int upy_list_attributes_CTX(const char* var_name, size_t max_part_length, attribute_printers_t attrib_printers,size_t max_attrib, size_t skip);

#endif