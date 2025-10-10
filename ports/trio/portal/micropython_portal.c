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
#include "py/objfun.h"
#include "py/bc.h"

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

int start_upy_single(const char* file_name, const char* src, bool debug, size_t stack_size) {
 heap_def hd = GetMpHeapDefTrio();
    int stack_dummy;
    stack_top = (char*)&stack_dummy;
    gc_init(hd.start, hd.end);
    mp_init();
    mp_cstack_init_with_top((void*)stack_top, stack_size);
    MP_STATE_VM(trio_debug) = debug;

    if (debug) {
        mp_hal_stdout_tx_strn("Running Python in debug mode\r\n", 30);
    }

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
          // ! Other exception that should have been caught in do_str
          mp_hal_stdout_tx_strn("Python uncaught exception during VM abort:\r\n", 28);
          mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
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
    mp_printf(&mp_plat_print, "nlr_jump_fail");
    while (1) {
        ;
    }
}

void MP_NORETURN __fatal_error(const char* msg) {
    mp_printf(&mp_plat_print, "Fatal error: %s", msg);
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char* file, int line, const char* func, const char* expr) {
    mp_printf(&mp_plat_print, "Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

void vstr_add_str_void(void *vstr, const char *str, size_t len) {
   vstr_add_strn((vstr_t*) vstr, str, len);
}

const char* find_variable(mp_state_ctx_t* state_ctx, mp_code_state_t* code_state, scope_t* scope, const char* looking_for) {
   mp_obj_t* /*const*/ fastn;
   size_t n_state = code_state->n_state;
   fastn = &code_state->state[n_state - 1];

   for (int i = 0; i < scope->id_info_len; i++) {
      id_info_t* id = &scope->id_info[i];

      if (strcmp(qstr_str(id->qst), looking_for) != 0) { // Not our variable
         continue;
      }

      mp_obj_t obj_shared;
      switch (id->kind) {
      case ID_INFO_KIND_LOCAL:
      case ID_INFO_KIND_FREE:
         obj_shared = fastn[-(id->local_num)];
         break;
      case ID_INFO_KIND_CELL:
         obj_shared = mp_obj_cell_get(fastn[-(id->local_num)]);
         break;
      case ID_INFO_KIND_GLOBAL_EXPLICIT:
      case ID_INFO_KIND_GLOBAL_IMPLICIT:
      case ID_INFO_KIND_GLOBAL_IMPLICIT_ASSIGNED:
         nlr_buf_t nlr;
         nlr.ret_val = NULL;
         if (nlr_push(&nlr) == 0) {
            obj_shared = mp_load_global(id->qst);
         }
         else {
            obj_shared = MP_OBJ_NULL;
         }
         break;
      };

      if (obj_shared == MP_OBJ_NULL) {
         return "undefined";
      }
      else {
         const mp_obj_type_t* type = mp_obj_get_type(obj_shared);

         vstr_t vstr;
         char* ret;
         if (MP_OBJ_TYPE_HAS_SLOT(type, print)) {
            vstr_init(&vstr, 16);

            mp_print_t print;
            print.data = &vstr;
            print.print_strn = vstr_add_str_void;

            MP_OBJ_TYPE_GET_SLOT(type, print)(&print, obj_shared, PRINT_STR);

            ret = vstr_null_terminated_str(&vstr);
         }
         else {
            const char* type_name = qstr_str(type->name);
            vstr_init(&vstr, strlen(type_name) + 3); // < + type_name + > + \0
            vstr_add_char(&vstr, '<');
            vstr_add_str(&vstr, type_name);
            vstr_add_char(&vstr, '>');
            ret = vstr_null_terminated_str(&vstr);
         }

         trio_to_free_t* new_to_free = m_new(trio_to_free_t, 1);
         *new_to_free = (trio_to_free_t){
            .next = state_ctx->vm.trio_to_free,
            .str = vstr
         };
         state_ctx->vm.trio_to_free = new_to_free;
         return ret;
      }
   }

   return NULL; // Not found
}

// Value returned only lives until execution resumes!
const char* upy_access_variable(const char* var_name) {
   mp_state_ctx_t* state_ctx = MP_STATE_REF;
   if (!state_ctx->vm.trio_has_paused) return NULL;
   if (state_ctx->vm.trio_access_ongoing) return NULL; // ! Shouldn't be possible
   state_ctx->vm.trio_access_ongoing = true; // TODO: Make atomic

   mp_code_state_t* code_state = (mp_code_state_t*) state_ctx->vm.trio_paused_code_state;
   trio_scope_t* trio_scope = state_ctx->vm.trio_scopes;

   while (trio_scope != NULL) {
      scope_t* scope = (scope_t*)trio_scope->scopes;

      while (scope != NULL) {
         if (scope->raw_code->fun_data == (void*)code_state->fun_bc->bytecode) {
            const char* ret = find_variable(state_ctx, code_state, scope, var_name);
            state_ctx->vm.trio_access_ongoing = false; // TODO: Make atomic
            return ret;
         }

         scope = scope->next;
      }

      trio_scope = trio_scope->next;
   }

   state_ctx->vm.trio_access_ongoing = false; // TODO: Make atomic
   return NULL;
}