// Include MicroPython API.
#include "py/runtime.h"

#include "py/mphal.h"

#include <stdlib.h>

static mp_obj_t example_add_ints(mp_obj_t a_obj, mp_obj_t b_obj) {
    int a = mp_obj_get_int(a_obj);
    int b = mp_obj_get_int(b_obj);

    return mp_obj_new_int(a + b);
}

static MP_DEFINE_CONST_FUN_OBJ_2(example_add_ints_obj, example_add_ints);

// ------------ Rapidstop

extern void MotionAPI_Rapidstop(int32_t mode);

static mp_obj_t rapidstop(mp_obj_t a_obj) {
   int a = mp_obj_get_int(a_obj);

   if (a >= 0 && a <= 2) MotionAPI_Rapidstop((int32_t)a);
   else {
      mp_raise_ValueError(MP_ERROR_TEXT("rapidstop mode out of range"));
   }

   return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(rapidstop_obj, rapidstop);

// ------------- Forward

extern void MotionAPI_Forward(int32_t axis, bool bufferwait, void* moveStatus);

static mp_obj_t forward() {
   MotionAPI_Forward(0, false, NULL);

   return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_0(forward_obj, forward);

// -------------- Op

extern void MotionAPI_SingleOutput(uint32_t output, uint32_t on);

static mp_obj_t op(mp_obj_t a_obj, mp_obj_t b_obj) {
   int output = mp_obj_get_int(a_obj);
   int on = mp_obj_get_int(b_obj); // Also supports True and False

   MotionAPI_SingleOutput((uint32_t)output, (uint32_t)on);

   return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(op_obj, op);

// Defs

static const mp_rom_map_elem_t module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_trio) },
    { MP_ROM_QSTR(MP_QSTR_add_ints), MP_ROM_PTR(&example_add_ints_obj) },
    { MP_ROM_QSTR(MP_QSTR_rapidstop), MP_ROM_PTR(&rapidstop_obj) },
    { MP_ROM_QSTR(MP_QSTR_forward), MP_ROM_PTR(&forward_obj) },
    { MP_ROM_QSTR(MP_QSTR_op), MP_ROM_PTR(&op_obj) },
};
static MP_DEFINE_CONST_DICT(module_globals, module_globals_table);


const mp_obj_module_t trio_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_trio, trio_cmodule);
