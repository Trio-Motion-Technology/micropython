#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
//#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
//#include "shared/runtime/pyexec.h"
#include "trio_mphal.h"
#include "py/mpstate.h"
#include "py/emitglue.h"
#include "py/scope.h"

// void set_hal_functions(HalFunctions hal_functions) {
// 	set_hal_functions_int(hal_functions);
// }

extern mp_uint_t mp_hal_stdout_tx_strn(const char* str, size_t len);
void do_str(const char* file_name, const char* src, mp_parse_input_kind_t input_kind) {
    
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t* lex = mp_lexer_new_from_str_len(qstr_from_str(file_name), src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    }
    else {
        //printf("Uncaught exception\n");
        // uncaught exception
        mp_hal_stdout_tx_strn("Python uncaught exception:\r\n", 28);
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        //mp_hal_stdout_tx_strn("Uncaught exception\r\n", 20);
    }
}

static char* stack_top;

int start_upy_single(const char* file_name, const char* src, bool debug) {
    heap_def hd = GetMpHeapDefTrio();
    int stack_dummy;
    stack_top = (char*)&stack_dummy;
    gc_init(hd.start, hd.end);
    mp_init();
    MP_STATE_VM(trio_debug) = debug;

    nlr_buf_t nlr;
    nlr.ret_val = NULL;

    if (nlr_push(&nlr) == 0) {
        nlr_set_abort(&nlr);
        do_str(file_name, src, MP_PARSE_FILE_INPUT);
        nlr_pop();
    }
    else {
       if (nlr.ret_val == NULL) {
           mp_hal_stdout_tx_strn("Python VM aborted\r\n", 19);
       }
       else {
           // Other exception
       }
    }

    mp_deinit();
    return 0;
}

//int start_upy() {
//    int stack_dummy;
//    stack_top = (char*)&stack_dummy;
//    gc_init(heap, heap + sizeof(heap));
//    mp_init();
//
//    /*do_str("print(12)", MP_PARSE_SINGLE_INPUT);
//    do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
//    do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
//    do_str("try:\n    print(input('x: '))\nexcept Exception as e:\n    print('x', e)", MP_PARSE_FILE_INPUT);*/
//
//    do_str("i = 0\nwhile True:\n    i += 1\n    print(i)", MP_PARSE_FILE_INPUT);
//
//    mp_deinit();
//    return 0;
//}

// ! Called from external thread
void interrupt_upy(upy_ctx *ctx) {
    //nlr_raise(mp_obj_new_exception(&mp_type_SystemExit));
    mp_state_ctx_t* state_ctx = (mp_state_ctx_t*)ctx;
    //state_ctx->vm.mp_kbd_exception.traceback_data = NULL;
    //state_ctx->thread.mp_pending_exception = MP_OBJ_FROM_PTR(&state_ctx->vm.mp_kbd_exception);

    // Cannot use mp_sched_vm_abort - called from external thread
    // Does nothing if nlr_abort is not set
    state_ctx->vm.vm_abort = true;
}

#if MICROPY_ENABLE_GC
void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void* dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    //gc_dump_info(&mp_plat_print);
}
#endif

void nlr_jump_fail(void* val) {
    while (1) {
        ;
    }
}

void MP_NORETURN __fatal_error(const char* msg) {
    printf("Fatal error: %s", msg);
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char* file, int line, const char* func, const char* expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
