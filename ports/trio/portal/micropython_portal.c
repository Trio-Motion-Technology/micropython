#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "trio_mphal.h"
#include "py/mpstate.h"
#include "py/emitglue.h"
#include "py/scope.h"
#include "py/objfun.h"
#include "py/bc.h"
#include "py/persistentcode.h"


void vstr_add_str_void(void* data, const char* str, size_t len) {
   vstr_add_strn((vstr_t*)data, str, len);
}

mp_print_t print_to_vstr(vstr_t* vstr) {
   mp_print_t print;
   print.data = (void*) vstr;
   print.print_strn = vstr_add_str_void;
   return print;
}

typedef struct {
   char* buffer;
   size_t current_offset;
   size_t buffer_size;
} buffer_data_t;

void buffer_add_str_void(void* data, const char* str, size_t len) {
   buffer_data_t* bdata = (buffer_data_t*)data;

   size_t copy_amount;
   if (len <= bdata->buffer_size - bdata->current_offset) copy_amount = len;
   else copy_amount = bdata->buffer_size - bdata->current_offset;

   if (copy_amount != 0) {
      memcpy((void*)(bdata->buffer + bdata->current_offset), (void*)str, copy_amount);
      bdata->current_offset += copy_amount;

      // Add ellipsis if too long
      if (bdata->current_offset == bdata->buffer_size && bdata->buffer_size > 3) {
         bdata->buffer[bdata->buffer_size - 1] = '.';
         bdata->buffer[bdata->buffer_size - 2] = '.';
         bdata->buffer[bdata->buffer_size - 3] = '.';
      }
   }
}

mp_print_t print_to_buffer(buffer_data_t* buffer_data) {
   mp_print_t print;
   print.data = (void*) buffer_data;
   print.print_strn = buffer_add_str_void;
   return print;
}

mp_print_t printer_to_mp_print(printer_t printer) {
   return (mp_print_t) {
      .print_strn = printer.print_strn,
      .data = printer.data
   };
}

void noinput_print(noinput_printer_t printer) {
   printer.print(printer.data);
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

// Currently the only difference from mp_obj_print_exception is subtracting 1 from the
// line number
static void mp_obj_print_exception_trio(const mp_print_t* print, mp_obj_t exc) {
   if (mp_obj_is_exception_instance(exc)) {
      size_t n, * values;
      mp_obj_exception_get_traceback(exc, &n, &values);
      if (n > 0) {
         assert(n % 3 == 0);
         mp_print_str(print, "Traceback (most recent call last):\n");
         for (int i = n - 3; i >= 0; i -= 3) {
#if MICROPY_ENABLE_SOURCE_LINE
            mp_printf(print, "  File \"%q\", line %d", values[i], (int)values[i + 1] - 1);
#else
            mp_printf(print, "  File \"%q\"", values[i]);
#endif
            // the block name can be NULL if it's unknown
            qstr block = values[i + 2];
            if (block == MP_QSTRnull) {
               mp_print_str(print, "\n");
            }
            else {
               mp_printf(print, ", in %q\n", block);
            }
         }
      }
   }
   mp_obj_print_helper(print, exc, PRINT_EXC);
   mp_print_str(print, "\n");
}

static void emit_exception(mp_obj_t exc, bool compilation_error) {
   if (compilation_error) {
      mp_hal_stdout_tx_strn("\r\nPython uncaught compilation exception:\r\n\r\n", 44);
   }
   else {
      mp_hal_stdout_tx_strn("\r\nPython uncaught runtime exception:\r\n\r\n", 40);

   }
   mp_obj_print_exception_trio(&mp_plat_print, exc);

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
            mp_printf(&print, "File \"%q\", line %d", values[i], (int)values[i + 1] - 1);
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

   mp_printf(&print, "    ");
   mp_obj_print_helper(&print, exc, PRINT_EXC);

   const char* exception_cstr = vstr_null_terminated_str(&exception_vstr);

   MicropythonSetException(qstr_str(file), exception_cstr, line - 1, compilation_error);

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

void upy_init(void) {
   heap_def_t hd = GetMpHeapDefTrio();
   gc_init(hd.start, hd.end);
   mp_init();
}

void upy_run_debug(const char* filename, const char* src_start, const char* src_end) {
    volatile int stack_dummy;
    void* stack_top = (void*)&stack_dummy;

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
    return;
}

void upy_run(const char* filename, const char* obj_start, const char* obj_end) {
   volatile int stack_dummy;
   void* stack_top = (void*)&stack_dummy;

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
   return;
}

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

// TODO: Can execute arbitrary Python - needs to call GC
void lookup_output_obj(mp_obj_t obj, scope_t* scope, lookup_printers_t lookup_printers, int type_support, size_t value_timeout_ms) {
   // Custom type/value handling
   if (obj != MP_OBJ_NULL && type_support >= 1 && mp_obj_is_float(obj)) {
      double flt = mp_obj_get_float_to_d(obj);

      lookup_printers.float_printer.print_flt(lookup_printers.float_printer.data, flt);
   }
   // Normal type/value handling
   else {
      mp_print_t type_print = printer_to_mp_print(lookup_printers.type_printer);
      mp_print_t value_print = printer_to_mp_print(lookup_printers.value_printer);

      if (obj == MP_OBJ_NULL) {
         mp_printf(&type_print, "undefined");
         noinput_print(lookup_printers.terminator_printer);
         noinput_print(lookup_printers.separator_printer);
         mp_printf(&value_print, "undefined");
      }
      else {
         const mp_obj_type_t* type = mp_obj_get_type(obj);
         mp_printf(&type_print, "%q", type->name);

         noinput_print(lookup_printers.terminator_printer);
         noinput_print(lookup_printers.separator_printer);

         if (MP_OBJ_TYPE_HAS_SLOT(type, print)) {
            nlr_buf_t nlr;
            nlr_set_timeout_abort(&nlr);
            MP_STATE_VM(trio_timeout_ms) = mp_hal_ticks_ms() + value_timeout_ms;

            if (nlr_push(&nlr) == 0) {
               MP_OBJ_TYPE_GET_SLOT(type, print)(&value_print, obj, PRINT_STR);
               noinput_print(lookup_printers.terminator_printer);
               nlr_pop();
            }
            else {
               // Timeout
               if (nlr.ret_val == NULL) {
                  noinput_print(lookup_printers.timeout_printer);
               }
               else {
                  mp_obj_t exc = (mp_obj_t)nlr.ret_val;
                  emit_exception(exc, false);
                  mp_printf(&value_print, "<%q object> [Error stringifying type]", type->name);
                  noinput_print(lookup_printers.terminator_printer);
               }
            }

            MP_STATE_VM(trio_timeout_ms) = 0;
         }
         else {
            mp_printf(&value_print, "<%q object>", type->name);
            noinput_print(lookup_printers.terminator_printer);
         }
      }

      
   }
   
   noinput_print(lookup_printers.separator_printer);

   mp_print_t scope_print = printer_to_mp_print(lookup_printers.type_printer);
   mp_printf(&scope_print, "%q", scope->simple_name);

   noinput_print(lookup_printers.terminator_printer);
}

// Returns MP_OBJ_SENTINEL if not found
// May return MP_OBJ_NULL if the variable could be defined but isn't
mp_obj_t find_variable(mp_code_state_t* code_state, scope_t* scope, const char* looking_for, size_t max_part_length) {
   char* current_section = (char*)alloca(max_part_length + 1);
   const char* section = looking_for;

   mp_obj_t value = MP_OBJ_SENTINEL;

   do {
      // Read part up to '.'
      size_t i = 0;
      while (true) {
         if (i == max_part_length) return MP_OBJ_SENTINEL; // Name too long

         if (section[i] == '\0') {
            section = NULL;
            break;
         }
         if (section[i] == '.') {
            section += i + 1;
            break;
         }

         current_section[i] = section[i];

         i++;
      }
      current_section[i] = '\0';


      if (value == MP_OBJ_SENTINEL) { // First iteration
         mp_obj_t* /*const*/ fastn;
         size_t n_state = code_state->n_state;
         fastn = &code_state->state[n_state - 1];

         for (int i = 0; i < scope->id_info_len; i++) {
            id_info_t* id = &scope->id_info[i];

            const char* var_name = qstr_str(id->qst);

            if (strcmp(var_name, &current_section[0]) != 0) { // Not our variable
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
            case ID_INFO_KIND_GLOBAL_IMPLICIT_ASSIGNED: {
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
            }
            };

            value = obj_shared;
            break;
         }
         if (value == MP_OBJ_SENTINEL) return MP_OBJ_SENTINEL; // Not found
         continue;
      }

      if (value == MP_OBJ_NULL) {
         return MP_OBJ_SENTINEL; // Can't access on NULL
      }

      // Attribute names are always interned as Qstrings, so if one doesn't exist, the attribute doesn't
      qstr attr_qstr = qstr_find_strn(&current_section[0], i);
      if (attr_qstr == MP_QSTRnull) return MP_OBJ_SENTINEL;

      nlr_buf_t nlr;
      if (nlr_push(&nlr) == 0) {
         value = mp_load_attr(value, attr_qstr);
         nlr_pop();
      }
      else {
         return MP_OBJ_SENTINEL;
      }

   } while (section != NULL);

   return value;
}

bool upy_access_variable_internal(const char* var_name, size_t max_part_length, lookup_printers_t lookup_printers, int type_support, size_t value_timeout_ms) {
   mp_state_ctx_t* state_ctx = MP_STATE_REF;
   if (!state_ctx->vm.trio_has_paused) return false;
   if (state_ctx->vm.trio_access_ongoing) return false; // ! Shouldn't be possible
   state_ctx->vm.trio_access_ongoing = true; // TODO: Proper locks

   mp_code_state_t* code_state = (mp_code_state_t*)state_ctx->vm.trio_paused_code_state;

   scope_t* scope = (scope_t*)state_ctx->vm.trio_paused_scope;
   if (scope == NULL) {
      state_ctx->vm.trio_access_ongoing = false;
      return false;
   }

   mp_obj_t ret = find_variable(code_state, scope, var_name, max_part_length);
   state_ctx->vm.trio_access_ongoing = false;
   
   if (ret != MP_OBJ_SENTINEL) {
      lookup_output_obj(ret, scope, lookup_printers, type_support, value_timeout_ms);
      return true;
   }
   else {
      return false;
   }
}

bool upy_access_variable(const char* var_name, size_t max_part_length, lookup_printers_t lookup_printers, int type_support, size_t value_timeout_ms) {
   volatile int stack_dummy;
   char* prev_stack_top = MP_STATE_THREAD(stack_top);
   MP_STATE_THREAD(stack_top) = (char*)&stack_dummy; // Allows stack checks to pass

   bool ret = upy_access_variable_internal(var_name, max_part_length, lookup_printers, type_support, value_timeout_ms);


   MP_STATE_THREAD(stack_top) = prev_stack_top; // Restore proper stack checks
   return ret;
}

size_t upy_get_stack_trace(stack_trace_frame_t* stack_trace_frames, size_t max_frames, bool* stack_truncated) {
   mp_state_ctx_t* state_ctx = MP_STATE_REF;
   mp_code_state_t* code_state = (mp_code_state_t*)state_ctx->vm.trio_paused_code_state;
   
   size_t frames_used = 0;
   stack_trace_frame_t* current_frame = stack_trace_frames;

   mp_code_state_t *unwind = code_state;
   while (unwind != NULL && frames_used < max_frames) {
       const byte* ip = unwind->fun_bc->bytecode;
       MP_BC_PRELUDE_SIG_DECODE(ip);
       MP_BC_PRELUDE_SIZE_DECODE(ip);
       const byte* line_info_top = ip + n_info;
       const byte* bytecode_start = ip + n_info + n_cell;
       size_t bc = unwind->ip - bytecode_start;
       qstr block_name = mp_decode_uint_value(ip);
       for (size_t i = 0; i < 1 + n_pos_args + n_kwonly_args; ++i) {
          ip = mp_decode_uint_skip(ip);
       }

       block_name = unwind->fun_bc->context->constants.qstr_table[block_name];
       qstr source_file = unwind->fun_bc->context->constants.qstr_table[0];
       
       size_t source_line = mp_bytecode_get_source_line(ip, line_info_top, bc) - 1;
       
       current_frame->block_name = qstr_str(block_name);
       current_frame->file = qstr_str(source_file);
       current_frame->line = source_line;
       
       unwind = unwind->prev;
       current_frame++;
       frames_used++;
   }

   if (stack_truncated != NULL) {
      *stack_truncated = unwind != NULL;
   }

   mp_printf(&mp_plat_print, "\r\n");

   return frames_used;
}

int list_attribs_on_variable(mp_obj_t var, attribute_printers_t attrib_printers, size_t max_attrib, size_t skip) {
   size_t found = 0;
   size_t outputted = 0;
   
   mp_print_t attrib_print = printer_to_mp_print(attrib_printers.attribute_printer);

   // From mp_builtin_dir in modbuiltins.c
   // Make a list of names in the given object
   // Implemented by probing all possible qstrs with mp_load_method_maybe

   // TODO: Very slow
   size_t nqstr = QSTR_TOTAL();
   for (size_t i = MP_QSTR_ + 1; i < nqstr; ++i) {
      

      mp_obj_t dest[2];
      mp_load_method_protected(var, i, dest, false);
      if (dest[0] != MP_OBJ_NULL) {
         if (found >= skip) {
            if (outputted == max_attrib) {
               noinput_print(attrib_printers.overflow_printer);
               break;
            }

            if (outputted != 0) noinput_print(attrib_printers.separator_printer);

            mp_printf(&attrib_print, "%q", i);

            noinput_print(attrib_printers.terminator_printer);

            outputted++;
         }

         found++;

         
      }
   }

   return outputted;
}

int list_local_attribs(mp_code_state_t* code_state, scope_t* scope, attribute_printers_t attrib_printers, size_t max_attrib, size_t skip) {
   size_t outputted = 0;
   
   mp_print_t attrib_print = printer_to_mp_print(attrib_printers.attribute_printer);

   for (int i = 0; i < scope->id_info_len; i++) {
      id_info_t* id = &scope->id_info[i];

      if (i >= skip) {
         if (outputted != 0) noinput_print(attrib_printers.separator_printer);

         if (outputted == max_attrib) {
            noinput_print(attrib_printers.overflow_printer);
            break;
         }

         mp_printf(&attrib_print, "%q", id->qst);

         noinput_print(attrib_printers.terminator_printer);

         outputted++;
      }
   }

   return outputted;
}

int upy_list_attributes_internal(const char* var_name, size_t max_part_length, attribute_printers_t attrib_printers, size_t max_attrib, size_t skip) {
   mp_state_ctx_t* state_ctx = MP_STATE_REF;
   if (!state_ctx->vm.trio_has_paused) return -1;
   if (state_ctx->vm.trio_access_ongoing) return -1; // ! Shouldn't be possible
   state_ctx->vm.trio_access_ongoing = true; // TODO: Proper locks

   mp_code_state_t* code_state = (mp_code_state_t*)state_ctx->vm.trio_paused_code_state;

   scope_t* scope = (scope_t*)state_ctx->vm.trio_paused_scope;
   if (scope == NULL) {
      state_ctx->vm.trio_access_ongoing = false;
      return -1;
   }

   if (var_name[0] == '\0') {
      int ret = list_local_attribs(code_state, scope, attrib_printers, max_attrib, skip);
      state_ctx->vm.trio_access_ongoing = false;
      return ret;
   }

   mp_obj_t var = find_variable(code_state, scope, var_name, max_part_length);

   if (var != MP_OBJ_SENTINEL) {
      int ret = list_attribs_on_variable(var, attrib_printers, max_attrib, skip);
      state_ctx->vm.trio_access_ongoing = false;
      return ret;
   }
   else {
      state_ctx->vm.trio_access_ongoing = false;
      return -1;
   }
}

int upy_list_attributes(const char* var_name, size_t max_part_length, attribute_printers_t attrib_printers, size_t max_attrib, size_t skip) {
   volatile int stack_dummy;
   char* prev_stack_top = MP_STATE_THREAD(stack_top);
   MP_STATE_THREAD(stack_top) = (char*)&stack_dummy; // Allows stack checks to pass

   int ret = upy_list_attributes_internal(var_name, max_part_length, attrib_printers, max_attrib, skip);


   MP_STATE_THREAD(stack_top) = prev_stack_top; // Restore proper stack checks
   return ret;
}