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
    mp_uint_t __dummy[1024 / sizeof(mp_uint_t)];
} upy_ctx;

typedef struct {
   void* start;
   void* end;
} heap_def;

extern mp_uint_t MpHalStdoutTxStrnTrio(const char* str, size_t len);
extern mp_uint_t MpHalTicksUsTrio(void);
extern mp_uint_t MpHalTicksCpuTrio(void);
extern void MpHalDelayMsTrio(mp_uint_t delay);
extern upy_ctx* GetMpStateCtxTrio(void);
extern heap_def GetMpHeapDefTrio(void);

extern void OnLineChange(const char* fileName, int lineNo, void* code_state);

typedef enum {
   upy_import_stat_dir,
   upy_import_stat_file,
   upy_import_stat_no_exist,
} upy_import_stat_t;

extern upy_import_stat_t GetImportStatTrio(const char* path);

extern mp_int_t MicropythonReadPythonFileLength(const char* filename);
extern mp_uint_t MicropythonReadPythonFileName(const char* filename, char* buf);

//int start_upy();
int start_upy_single(const char* file_name, const char *src, bool debug);

void interrupt_upy(upy_ctx *ctx);

#endif