#ifndef gsd_common_h
#define gsd_common_h
#include "gsd-config.h"
#include <stdarg.h>
#include <stdalign.h>
#include <float.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef uintmax_t gsd_procid;
typedef uintmax_t gsd_threadid;
typedef int status;
typedef uintptr_t index;
typedef size_t size;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


enum gsd_stdstatus {
        gsd_ok,
        gsd_forbidden,
        gsd_invalid_param,
        gsd_not_present,
        gsd_already_present,
        gsd_out_of_resources
};
#endif