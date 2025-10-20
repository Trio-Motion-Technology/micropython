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
#include "py/persistentcode.h"

// void set_hal_functions(HalFunctions hal_functions) {
// 	set_hal_functions_int(hal_functions);
// }

void vstr_add_str_void(void* data, const char* str, size_t len) {
   vstr_add_strn((vstr_t*)data, str, len);
}

mp_print_t print_to_vstr(vstr_t* vstr) {
   mp_print_t print;
   print.data = vstr;
   print.print_strn = vstr_add_str_void;
   return print;
}

extern mp_uint_t mp_hal_stdout_tx_strn(const char* str, size_t len);

void upy_compile_code(const char* filename, const char* src_start, const char* src_end) {
   mp_reader_t reader;
   mp_reader_new_trio_src(&reader, src_start, src_end);
   mp_lexer_t* lex = mp_lexer_new(qstr_from_str(filename), reader);
   qstr source_name = lex->source_name;

   mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT); // Frees lexer automatically
   mp_compiled_module_t cm;
   cm.context = m_new_obj(mp_module_context_t);
   mp_compile_to_raw_code(&parse_tree, source_name, false, &cm);

   vstr_t vstr;
   vstr_init(&vstr, 16);

   // TODO: Print directly into object memory
   mp_print_t print = print_to_vstr(&vstr);
   mp_raw_code_save(&cm, &print);

   MicropythonSaveTrioObj(filename, vstr.buf, vstr.len);

   vstr_clear(&vstr);
}

static void emit_exception(mp_obj_t exc, bool compilation_error) {
   mp_hal_stdout_tx_strn("Python uncaught exception:\r\n", 28);
   mp_obj_print_exception(&mp_plat_print, exc);

   vstr_t exception_vstr;

   vstr_init(&exception_vstr, 16);

   mp_print_t print = print_to_vstr(&exception_vstr);

   size_t line = 1;
   qstr file = 0;

   if (mp_obj_is_exception_instance(exc)) {
      size_t n, * values;
      mp_obj_exception_get_traceback(exc, &n, &values);
      if (n > 0) {
         assert(n % 3 == 0);
         for (int i = n - 3; i >= 0; i -= 3) {
            file = values[i];
            line = values[i + 1];
            mp_printf(&print, "  File \"%q\", line %d", values[i], (int)values[i + 1]);
            // the block name can be NULL if it's unknown
            qstr block = values[i + 2];
            if (block == MP_QSTRnull) {
               mp_print_str(&print, "\n");
            }
            else {
               mp_printf(&print, ", in %q\n", block);
            }
         }
      }
   }
   mp_obj_print_helper(&print, exc, PRINT_EXC);
   mp_print_str(&print, "\n");

   const char* exception_cstr = vstr_null_terminated_str(&exception_vstr);

   MicropythonSetException(qstr_str(file), exception_cstr, line, compilation_error);

   vstr_clear(&exception_vstr);
}

bool upy_compile_code_no_env(const char* filename, const char* src_start, const char* src_end) {
   volatile int stack_dummy;
   void* stack_top = (void*)&stack_dummy;
   
   heap_def_t hd = GetMpHeapDefTrio();
   gc_init(hd.start, hd.end);
   mp_init();
   mp_cstack_init_with_top(stack_top, GetStackSizeTrio());

   nlr_buf_t nlr;
   nlr.ret_val = NULL;

   bool success;

   if (nlr_push(&nlr) == 0) {
      nlr_set_abort(&nlr);

      upy_compile_code(filename, src_start, src_end);

      nlr_pop();
      success = true;
   }
   else {
      mp_obj_t exc = (mp_obj_t)nlr.ret_val;
      emit_exception(exc, true);

      success = false;
   }

   mp_deinit();
   return success;
}

int upy_run_debug(const char* filename, const char* src_start, const char* src_end) {
   volatile int stack_dummy;
   void* stack_top = (void*)&stack_dummy;

   heap_def_t hd = GetMpHeapDefTrio();
   gc_init(hd.start, hd.end);
    mp_init();
    mp_cstack_init_with_top(stack_top, GetStackSizeTrio());

    MP_STATE_VM(trio_debug) = true;

    mp_hal_stdout_tx_strn("Running Python in debug mode\r\n", 30);

    nlr_buf_t nlr;
    nlr.ret_val = NULL;

    if (nlr_push(&nlr) == 0) {
        nlr_set_abort(&nlr);
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
           mp_reader_t reader;
           mp_reader_new_trio_src(&reader, src_start, src_end);
           mp_lexer_t* lex = mp_lexer_new(qstr_from_str(filename), reader);

           qstr source_name = lex->source_name;

           mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
           mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);

           mp_call_function_0(module_fun);
           nlr_pop();
        }
        else {
           mp_obj_t exc = (mp_obj_t)nlr.ret_val;
           emit_exception(exc, false);
        }
        nlr_pop();
    }
    else {
       if (nlr.ret_val == NULL) {
           mp_hal_stdout_tx_strn("Python VM aborted\r\n", 19);
       }
       else {
          // ! Other exception that should have been caught in inner nlr
          mp_obj_t exc = (mp_obj_t)nlr.ret_val;
          emit_exception(exc, false);
       }
    }

    mp_hal_stdout_tx_strn("Python exited\r\n", 15);

    mp_deinit();
    return 0;
}

// TODO: This also needs to support source as the file may not currently be compiled.
//     I would precompile the file before upy_run but the compiler requires a
//     Micropython environment and error handling so it is cleaner for upy_run
//     to call it.
int upy_run(const char* filename, const char* obj_start, const char* obj_end) {
   volatile int stack_dummy;
   void* stack_top = (void*)&stack_dummy;

   heap_def_t hd = GetMpHeapDefTrio();
   gc_init(hd.start, hd.end);

   gc_init(hd.start, hd.end);
   mp_init();
   mp_cstack_init_with_top(stack_top, GetStackSizeTrio());

   mp_hal_stdout_tx_strn("Running Python in normal mode\r\n", 31);

   nlr_buf_t nlr;
   nlr.ret_val = NULL;

   if (nlr_push(&nlr) == 0) {
      nlr_set_abort(&nlr);

      nlr_buf_t nlr;
      if (nlr_push(&nlr) == 0) {
         mp_reader_t reader;
         // Can use builtin mem reader as obj format is not special
         mp_reader_new_trio_obj(&reader, obj_start, obj_end);

         mp_compiled_module_t cm;
         mp_obj_t module_obj = mp_obj_new_module(MP_QSTR___main__);
         cm.context = module_obj;
         mp_raw_code_load(&reader, &cm);

         mp_obj_t module_fun = mp_make_function_from_proto_fun(cm.rc, cm.context, NULL);
         mp_call_function_0(module_fun);

         nlr_pop();
      }
      else {
         mp_obj_t exc = (mp_obj_t)nlr.ret_val;
         emit_exception(exc, false);
      }
   }
   else {
      if (nlr.ret_val == NULL) {
         mp_hal_stdout_tx_strn("Python VM aborted\r\n", 19);
      }
      else {
         // ! Other exception that should have been caught in do_str
         mp_obj_t exc = (mp_obj_t)nlr.ret_val;
         emit_exception(exc, false);
      }
   }

   mp_hal_stdout_tx_strn("Python exited\r\n", 15);

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
void upy_abort(upy_ctx *ctx) {
    //nlr_raise(mp_obj_new_exception(&mp_type_SystemExit));
    mp_state_ctx_t* state_ctx = (mp_state_ctx_t*)ctx;
    //state_ctx->vm.mp_kbd_exception.traceback_data = NULL;
    //state_ctx->thread.mp_pending_exception = MP_OBJ_FROM_PTR(&state_ctx->vm.mp_kbd_exception);

    // Cannot use mp_sched_vm_abort - called from external thread
    // Does nothing if nlr_abort is not set
    state_ctx->vm.vm_abort = true;
}

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void* dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)MP_STATE_THREAD(stack_top) - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    //gc_dump_info(&mp_plat_print);
}

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

// TODO: While they are cleaned up once unpaused, every variable inspection allocates
//    space on the Micropython heap so the heap could fill, with enough variables
//    being inspected or with the heap already beaing quite full.
const char* find_variable(mp_state_ctx_t* state_ctx, mp_code_state_t* code_state, scope_t* scope, const char* looking_for) {
   mp_obj_t* /*const*/ fastn;
   size_t n_state = code_state->n_state;
   fastn = &code_state->state[n_state - 1];

   for (int i = 0; i < scope->id_info_len; i++) {
      id_info_t* id = &scope->id_info[i];

      const char* var_name = qstr_str(id->qst);

      if (strcmp(var_name, looking_for) != 0) { // Not our variable
         continue;
      }

      mp_obj_t obj_shared = MP_OBJ_NULL;
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
            nlr_pop();
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
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
               vstr_init(&vstr, 16);

               mp_print_t print = print_to_vstr(&vstr);

               MP_OBJ_TYPE_GET_SLOT(type, print)(&print, obj_shared, PRINT_STR);

               ret = vstr_null_terminated_str(&vstr);

               nlr_pop();
            }
            else {
               const char* type_name = qstr_str(type->name);
               vstr_init(&vstr, strlen(type_name) + 36); // "<" + type_name + " object> [Error stringifying type]\0"
               vstr_add_char(&vstr, '<');
               vstr_add_str(&vstr, type_name);
               vstr_add_str(&vstr, " object> [Error stringifying type]");
               ret = vstr_null_terminated_str(&vstr);
            }
         }
         else {
            const char* type_name = qstr_str(type->name);
            vstr_init(&vstr, strlen(type_name) + 10); // "<" + type_name + " object>\0"
            vstr_add_char(&vstr, '<');
            vstr_add_str(&vstr, type_name);
            vstr_add_str(&vstr, " object>");
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

const char* upy_access_variable_internal(const char* var_name) {
   mp_state_ctx_t* state_ctx = MP_STATE_REF;
   if (!state_ctx->vm.trio_has_paused) return NULL;
   if (state_ctx->vm.trio_access_ongoing) return NULL; // ! Shouldn't be possible
   state_ctx->vm.trio_access_ongoing = true; // TODO: Make atomic

   mp_code_state_t* code_state = (mp_code_state_t*)state_ctx->vm.trio_paused_code_state;

   scope_t* scope = (scope_t*)state_ctx->vm.trio_paused_scope;
   if (scope == NULL) {
      state_ctx->vm.trio_access_ongoing = false; // TODO: Make atomic
      return NULL;
   }

   const char* ret = find_variable(state_ctx, code_state, scope, var_name);
   state_ctx->vm.trio_access_ongoing = false; // TODO: Make atomic
   return ret;
}

// Value returned only lives until execution resumes!
const char* upy_access_variable(const char* var_name) {
   volatile int stack_dummy;
   char* prev_stack_top = MP_STATE_THREAD(stack_top);
   MP_STATE_THREAD(stack_top) = (char*)&stack_dummy; // Allows stack checks to pass

   const char* ret = upy_access_variable_internal(var_name);

   MP_STATE_THREAD(stack_top) = prev_stack_top; // Restore proper stack checks
   return ret;
}