/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

// options to control how MicroPython is built

// Variant-specific definitions.
// #include "mpconfigvariant.h"
#include "mpconfigtypes.h"

//#define MICROPY_EXPOSE_MP_COMPILE_TO_RAW_CODE (1)

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

#define MICROPY_OPT_MAP_LOOKUP_CACHE (1)

#define MICROPY_ENABLE_VM_ABORT (1)

// MPZ is Micropython's BigInt implementation
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)

// We don't have OS errors
#define MICROPY_PY_ERRNO (0)

#define MICROPY_ENABLE_COMPILER     (1)
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_BUILTINS_SLICE (1)
#define MICROPY_MULTIPLE_INHERITANCE (1)
#define MICROPY_PY_ASYNC_AWAIT (1)
#define MICROPY_PY_ASSIGN_EXPR (1)
#define MICROPY_PY_BUILTINS_BYTES_HEX (1)
// TODO: Consider
//#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_STR_CENTER (1)
#define MICROPY_PY_BUILTINS_STR_COUNT (1)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (1)
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_DICT_FROMKEYS (1)
#define MICROPY_PY_BUILTINS_SET (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_PY_BUILTINS_PROPERTY (1)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS (1)
#define MICROPY_PY_BUILTINS_RANGE_BINOP (1)
#define MICROPY_PY_BUILTINS_NEXT2 (1)
#define MICROPY_PY_BUILTINS_ROUND_INT (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS (1)
#define MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS (1)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS (1)
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#define MICROPY_PY_BUILTINS_FILTER (1)
#define MICROPY_PY_BUILTINS_REVERSED (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (1)
#define MICROPY_PY_BUILTINS_MIN_MAX (1)
#define MICROPY_PY_BUILTINS_POW3 (1)
#define MICROPY_PY_ARRAY (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN (1)
#define MICROPY_PY_COLLECTIONS (1)
#define MICROPY_PY_COLLECTIONS_DEQUE_ITER (1)
#define MICROPY_PY_COLLECTIONS_DEQUE_SUBSCR (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (1)
#define MICROPY_PY_COLLECTIONS_NAMEDTUPLE__ASDICT (1)
#define MICROPY_PY_MATH (1)
#define MICROPY_PY_MATH_CONSTANTS (1)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (1)
#define MICROPY_PY_MATH_FACTORIAL (1)
#define MICROPY_PY_MATH_ISCLOSE (1)
#define MICROPY_PY_CMATH (1)
#define MICROPY_PY_ASYNCIO (1)
#define MICROPY_PY_DEFLATE (1)
#define MICROPY_PY_DEFLATE_COMPRESS (1)
#define MICROPY_PY_JSON (1)
#define MICROPY_PY_RE (1)
#define MICROPY_PY_RE_MATCH_GROUPS (1)
#define MICROPY_PY_RE_MATCH_SPAN_START_END (1)
#define MICROPY_PY_RE_SUB (1)
#define MICROPY_PY_HEAPQ (1)


#define MICROPY_PERSISTENT_CODE_SAVE (1)

#define MICROPY_COMP_CONST_FOLDING (1)
#define MICROPY_COMP_CONST_TUPLE (1)
#define MICROPY_COMP_CONST_LITERAL (1)
#define MICROPY_COMP_MODULE_CONST (1)
#define MICROPY_COMP_CONST (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_RETURN_IF_EXPR (1)
#define MICROPY_COMP_CONST_FLOAT (1)
#define MICROPY_COMP_RETURN_IF_EXPR (1)


// Check how much stack we've used when recursing to prevent stack overflow
#define MICROPY_STACK_CHECK (1)

#define MICROPY_OPT_LOAD_ATTR_FAST_PATH (1)
#define MICROPY_OPT_MAP_LOOKUP_CACHE (1)
#define MICROPY_OPT_MPZ_BITWISE (1)
#define MICROPY_OPT_MATH_FACTORIAL (1)

#define MICROPY_PY_SYS_SETTRACE (0)

// #define MICROPY_QSTR_EXTRA_POOL           mp_qstr_frozen_const_pool
#define MICROPY_ENABLE_GC                 (1)
//#define MICROPY_HELPER_REPL               (1)
#define MICROPY_REPL_EVENT_DRIVEN         (0)
#define MICROPY_ENABLE_EXTERNAL_IMPORT    (1)

#define MICROPY_ALLOC_PATH_MAX            (256)

// Use the minimum headroom in the chunk allocator for parse nodes.
#define MICROPY_ALLOC_PARSE_CHUNK_INIT    (16)

#define MICROPY_ENABLE_SOURCE_LINE (1)
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_DETAILED)

// Disable all optional sys module features.
#define MICROPY_PY_SYS_MODULES            (1)
#define MICROPY_PY_SYS_EXIT               (0)
#define MICROPY_PY_SYS_PATH               (0)
#define MICROPY_PY_SYS_ARGV               (0)
#define MICROPY_PY_MATH                   (1)

#ifndef MICROPY_FLOAT_IMPL
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
#endif

// We don't have OS errors
#define MICROPY_USE_INTERNAL_ERRNO (1)

// NLR is too platform/assembly dependant. Use setjmp
#define MICROPY_NLR_SETJMP (1)


#define MICROPY_HAS_FILE_READER (1)

// Micropy will use the heap as its stack, not falling back to recursing
//    1. Without stack checking (although this can be done) we risk a segfault
//    2. As thread stack space is statically allocated anyway, would rather increase uPy
//       heap size than thread stack size for all processes
//    3. Adds prev attribute to mp_code_state_t, allowing a stack trace to be obtained, without
//       enabling the expensive MICROPY_PY_SYS_SETTRACE flag, or reimplementing the parts of
//       MICROPY_PY_SYS_SETTRACE needed to get a stack trace
#define MICROPY_STACKLESS (1)
#define MICROPY_STACKLESS_STRICT (1) // Don't allow fallback to stack when OOM

#define MICROPY_HW_BOARD_NAME "trio-unknown"
#define MICROPY_HW_MCU_NAME "unknown-cpu"

extern const struct _mp_print_t mp_stderr_print;

#ifdef _MSC_VER
#define MICROPY_GCREGS_SETJMP       (1)
#define MICROPY_USE_INTERNAL_PRINTF (0)
#endif

#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF   (1)
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE     (256)
#define MICROPY_KBD_EXCEPTION       (1)

#define MICROPY_PY_TIME (1)

#define MICROPY_PERSISTENT_CODE_LOAD (1)
#define MICROPY_PY_FSTRINGS (1)

// type definitions for the specific machine

// #if defined(__MINGW32__) && defined(__LP64__)
// typedef long mp_int_t; // must be pointer size
// typedef unsigned long mp_uint_t; // must be pointer size
// #elif defined(__MINGW32__) && defined(_WIN64)
// #include <stdint.h>
// typedef __int64 mp_int_t;
// typedef unsigned __int64 mp_uint_t;
// #define MP_SSIZE_MAX __INT64_MAX__
// #elif defined(_MSC_VER) && defined(_WIN64)
// typedef __int64 mp_int_t;
// typedef unsigned __int64 mp_uint_t;
// #else
// // These are definitions for machines where sizeof(int) == sizeof(void*),
// // regardless for actual size.
// typedef int mp_int_t; // must be pointer size
// typedef unsigned int mp_uint_t; // must be pointer size
// #endif

// typedef long suseconds_t;

// Just assume Windows is little-endian - mingw32 gcc doesn't
// define standard endianness macros.
#define MP_ENDIANNESS_LITTLE (1)

// // Cannot include <sys/types.h>, as it may lead to symbol name clashes
// #if _FILE_OFFSET_BITS == 64 && !defined(__LP64__)
// typedef long long mp_off_t;
// #else
// typedef long mp_off_t;
// #endif

#define MP_STATE_PORT               MP_STATE_VM

#define MICROPY_MPHALPORT_H         "trio_mphal.h"

// We need to provide a declaration/definition of alloca()
// #include <malloc.h>

// #include "realpath.h"
// #include "init.h"

#ifdef __GNUC__
#define MP_NOINLINE __attribute__((noinline))
#endif

// MSVC specifics
#ifdef _MSC_VER

// Sanity check

#if (_MSC_VER < 1800)
    #error Can only build with Visual Studio 2013 toolset
#endif


// CL specific overrides from mpconfig

#define MP_NORETURN                 __declspec(noreturn)
#define MP_WEAK
#define MP_NOINLINE                 __declspec(noinline)
#define MP_ALWAYSINLINE             __forceinline
#define MP_LIKELY(x)                (x)
#define MP_UNLIKELY(x)              (x)
#define MICROPY_PORT_CONSTANTS      { MP_ROM_QSTR(MP_QSTR_dummy), MP_ROM_PTR(NULL) } // can't have zero-sized array
#ifdef _WIN64
#define MP_SSIZE_MAX                _I64_MAX
#else
#define MP_SSIZE_MAX                _I32_MAX
#endif

// VC++ 12.0 fixes
#if (_MSC_VER <= 1800)
#define MICROPY_PY_MATH_ATAN2_FIX_INFNAN (1)
#define MICROPY_PY_MATH_FMOD_FIX_INFNAN (1)
#ifdef _WIN64
#define MICROPY_PY_MATH_MODF_FIX_NEGZERO (1)
#else
#define MICROPY_PY_MATH_POW_FIX_NAN (1)
#endif
#endif

// VC++ 2017 fixes
#if (_MSC_VER < 1920)
#define MICROPY_PY_MATH_COPYSIGN_FIX_NAN (1)
#endif

// CL specific definitions

#ifndef __cplusplus
#define restrict
#define inline                      __inline
#define alignof(t)                  __alignof(t)
#endif
#define PATH_MAX                    MICROPY_ALLOC_PATH_MAX
#define S_ISREG(m)                  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)                  (((m) & S_IFMT) == S_IFDIR)
// #ifdef _WIN64
// #define SSIZE_MAX                   _I64_MAX
// typedef __int64 ssize_t;
// #else
// #define SSIZE_MAX                   _I32_MAX
// typedef int ssize_t;
// #endif
// typedef mp_off_t off_t;


// Put static/global variables in sections with a known name
// This used to be required for GC, not the case anymore but keep it as it makes the map file easier to inspect
// For this to work this header must be included by all sources, which is the case normally
// #define MICROPY_PORT_DATASECTION "upydata"
// #define MICROPY_PORT_BSSSECTION "upybss"
// #pragma data_seg(MICROPY_PORT_DATASECTION)
// #pragma bss_seg(MICROPY_PORT_BSSSECTION)


// System headers (needed e.g. for nlr.h)

#include <stddef.h> // for NULL
#include <assert.h> // for assert

#elif defined(__IAR_SYSTEMS_ICC__)
#define MP_NORETURN                 __noreturn
#define MP_WEAK                     
#define MP_NOINLINE                 
#define MP_ALWAYSINLINE             
#define MP_LIKELY(x)                (x)
#define MP_UNLIKELY(x)              (x)
#define MICROPY_PORT_CONSTANTS      { MP_ROM_QSTR(MP_QSTR_dummy), MP_ROM_PTR(NULL) } // can't have zero-sized array
//#define __arm__ (1)
#include <alloca.h>
#else
#error "Unsupported compiler - only IAR and MSVC currently supported"
#endif
