#ifndef stdtype_h
#define stdtype_h
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define NORETURN __attribute__((noreturn))
#define ALIGNED(x) __attribute__((aligned(x)))
#define SECTION(x) __attribute__((section(x)))
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intmax_t imax;
typedef uintmax_t umax;
typedef uintptr_t uptr;
typedef ptrdiff_t pdiff;
typedef void* handle;
typedef uptr index;
typedef size_t size;
typedef size offset;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef long long llong;
typedef unsigned char uchar;
typedef unsigned long long ullong;
typedef unsigned short ushort;
typedef _Float16 hfloat;
typedef _Bool boolean;
#define bool boolean
#define true 1
#define false 0

typedef int status;
enum gsd_stdstatus {
        gsd_ok,
        gsd_forbidden,
        gsd_invalid_param,
        gsd_not_present,
        gsd_already_present,
        gsd_out_of_resources,
        gsd_busy
};

#define status_assert(sv,code) if(sv != code) 
#define status_assert_ok(sv) if(sv != gsd_ok) 

#endif