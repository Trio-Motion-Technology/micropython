// Include MicroPython API.
#include "py/runtime.h"

#include "py/mphal.h"
#include "py/objexcept.h"

#include <stdlib.h>

#include <PythonInterface.h>

// ------------ Basic Error

MP_DEFINE_EXCEPTION(BasicError, Exception);

static mp_int_t basic_error_maybe_raise(mp_int_t code) {
   if (code <= 0) {
      return code;
   }

   BasicErrorTexts ts = MpcGetBasicErrorCTX(code);


   mp_obj_t args[2];

   if (ts.extraText[0] != '\0') {
      vstr_t* vstr;
      size_t len = strlen(ts.errorText) + 2 + strlen(ts.extraText);
      vstr = vstr_new(len + 1);
      vstr_printf(vstr, "%s: %s", ts.errorText, ts.extraText);
      args[0] = mp_obj_new_int(code);
      args[1] = mp_obj_new_str(vstr->buf, vstr->len);
      vstr_free(vstr);
   }
   else {
      args[0] = mp_obj_new_int(code);
      args[1] = mp_obj_new_str(ts.errorText, strlen(ts.errorText));
   }

   mp_obj_t exc = mp_obj_new_exception_args(&mp_type_BasicError, 2, args);
   nlr_raise(exc);

}

// ------------ Axis Parameters

typedef struct {
   mp_obj_base_t base;
} axis_param_obj_t;

static void axis_param_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest) {
   axis_param_obj_t* self = MP_OBJ_TO_PTR(self_in);

   // Read
   if (dest[0] == MP_OBJ_NULL) {
      if (attr == MP_QSTR_MERGE) {
         mp_int_t val = 0;
         basic_error_maybe_raise(MpcBasicGetMERGE(&val));
         dest[0] = mp_obj_new_bool(val != 0);
         return;
      }
      else if (attr == MP_QSTR_SERVO) {
         mp_int_t val = 0;
         basic_error_maybe_raise(MpcBasicGetSERVO(&val));
         dest[0] = mp_obj_new_bool(val != 0);
         return;
      }
      else { // Float params
         mp_float_t val;

         switch (attr) {
         case MP_QSTR_FORCE_SPEED:
            basic_error_maybe_raise(MpcBasicGetFORCE_SPEED(&val));
            break;
         case MP_QSTR_STARTMOVE_SPEED:
            basic_error_maybe_raise(MpcBasicGetSTARTMOVE_SPEED(&val));
            break;
         case MP_QSTR_ENDMOVE_SPEED:
            basic_error_maybe_raise(MpcBasicGetENDMOVE_SPEED(&val));
            break;
         default:
            // Not handled, local_dict will be checked instead
            dest[1] = MP_OBJ_SENTINEL;
            return;
         }

         dest[0] = mp_obj_new_float(val);
         return;
      }
   }

   // Store / delete
   if (dest[0] == MP_OBJ_SENTINEL) {
      // Delete not allowed
      if (dest[1] == MP_OBJ_NULL) {
         mp_raise_TypeError(MP_ERROR_TEXT("Delete not permitted on axis parameters"));
         return; 
      }

      // Store/write
      if (attr == MP_QSTR_MERGE) {
         basic_error_maybe_raise(MpcBasicSetMERGE(mp_obj_get_int(dest[1]) != 0));
         dest[0] = MP_OBJ_NULL;  // store succeeded
         return;
      }
      else if (attr == MP_QSTR_SERVO) {
         basic_error_maybe_raise(MpcBasicSetSERVO(mp_obj_get_int(dest[1]) != 0));
         dest[0] = MP_OBJ_NULL;  // store succeeded
         return;
      }
      else { // Float params
         mp_float_t val = mp_obj_get_float(dest[1]);

         switch (attr) {
         case MP_QSTR_FORCE_SPEED:
            basic_error_maybe_raise(MpcBasicSetFORCE_SPEED(val));
            break;
         case MP_QSTR_STARTMOVE_SPEED:
            basic_error_maybe_raise(MpcBasicSetSTARTMOVE_SPEED(val));
            break;
         case MP_QSTR_ENDMOVE_SPEED:
            basic_error_maybe_raise(MpcBasicSetENDMOVE_SPEED(val));
            break;
         default:
            // Not handled, local_dict will be checked instead
            dest[1] = MP_OBJ_SENTINEL;
            return;
         }

         dest[0] = MP_OBJ_NULL;
         return;
      }
   }
}

static MP_DEFINE_CONST_OBJ_TYPE(
   axis_param_type,
   MP_QSTR_AxisParameters,
   MP_TYPE_FLAG_NONE,
   attr, axis_param_attr
);

static axis_param_obj_t axis_param_singleton = {
    .base = { &axis_param_type },
};

// ------------ System Parameters

typedef struct {
   mp_obj_base_t base;
} system_param_obj_t;

static void system_param_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest) {
   system_param_obj_t* self = MP_OBJ_TO_PTR(self_in);

   // Read
   if (dest[0] == MP_OBJ_NULL) {
      switch (attr) {
      case MP_QSTR_OUTDEVICE:
         dest[0] = mp_obj_new_int(MpcGetChannelCTX());
         break;
      case MP_QSTR_WDOG:
         dest[0] = mp_obj_new_bool(MpcBasicGetWDOG());
         break;
      case MP_QSTR_LIMIT_BUFFERED:
         dest[0] = mp_obj_new_int(MpcBasicGetLIMIT_BUFFERED());
         break;
      default:
         // Not handled, local_dict will be checked instead
         dest[1] = MP_OBJ_SENTINEL;
      }
      
      return;
   }

   // Store / delete
   if (dest[0] == MP_OBJ_SENTINEL) {
      // Delete not allowed
      if (dest[1] == MP_OBJ_NULL) {
         mp_raise_TypeError(MP_ERROR_TEXT("Delete not permitted on system parameters"));
         return;
      }

      // Store/write
      switch (attr) {
      case MP_QSTR_OUTDEVICE:
         MpcSetChannelCTX(mp_obj_get_int(dest[1]));
         break;
      case MP_QSTR_LIMIT_BUFFERED:
         mp_float_t val = mp_obj_get_int(dest[1]);
         basic_error_maybe_raise(MpcBasicSetLIMIT_BUFFERED(val));
         break;
      case MP_QSTR_WDOG:
         MpcBasicSetWDOG(mp_obj_get_int(dest[1]));
         break;
      default:
         // Not handled, local_dict will be checked instead
         dest[1] = MP_OBJ_SENTINEL;
         return;
      }
      dest[0] = MP_OBJ_NULL;
      return;
   }
}

static MP_DEFINE_CONST_OBJ_TYPE(
   system_param_type,
   MP_QSTR_SystemParameters,
   MP_TYPE_FLAG_NONE,
   attr, system_param_attr
);

static system_param_obj_t system_param_singleton = {
    .base = { &system_param_type },
};

// ------------ BASE

static mp_obj_t base(size_t n_args, const mp_obj_t* args) {
   if (n_args < 1 || n_args > 64) { // TODO: The limit should be the axis limit
      mp_raise_ValueError(MP_ERROR_TEXT("Expected 1 to 64 integer arguments"));
   }

   mp_int_t* int_array = m_malloc(n_args * sizeof(mp_int_t));
   if (!int_array) {
      mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Memory allocation failed"));
   }

   for (size_t i = 0; i < n_args; ++i) {
      int_array[i] = mp_obj_get_int(args[i]);
   }

   basic_error_maybe_raise(MpcBasicBASE(int_array, n_args));

   return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(base_obj, 1, 64, base);

// ------------ MOVEABSSP

static mp_obj_t moveabssp(size_t n_args, const mp_obj_t* args) {
   if (n_args < 1 || n_args > 64) { // TODO: The limit should be the axis limit
      mp_raise_ValueError(MP_ERROR_TEXT("Expected 1 to 64 float arguments"));
   }

   double* flt_array = m_malloc(n_args * sizeof(double));
   if (!flt_array) {
      mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Memory allocation failed"));
   }


   for (size_t i = 0; i < n_args; ++i) {
      flt_array[i] = (double)mp_obj_get_float(args[i]);
   }

   basic_error_maybe_raise(MpcBasicMOVEABSSP(flt_array, n_args));

   return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(moveabssp_obj, 1, 64, moveabssp);

// Defs

static const mp_rom_map_elem_t module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_trio) },
    { MP_ROM_QSTR(MP_QSTR_BasicError), MP_ROM_PTR(&mp_type_BasicError) },
    { MP_ROM_QSTR(MP_QSTR_BASE), MP_ROM_PTR(&base_obj) },
    { MP_ROM_QSTR(MP_QSTR_MOVEABSSP), MP_ROM_PTR(&moveabssp_obj) },
    { MP_ROM_QSTR(MP_QSTR_AXIS), MP_ROM_PTR(&axis_param_singleton) },
    { MP_ROM_QSTR(MP_QSTR_SYSTEM), MP_ROM_PTR(&system_param_singleton) },
};
static MP_DEFINE_CONST_DICT(module_globals, module_globals_table);


const mp_obj_module_t trio_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_trio, trio_cmodule);