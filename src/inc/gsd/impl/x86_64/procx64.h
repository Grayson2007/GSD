#ifndef procx64_h
#define procx64_h
#include <gsd-common.h>

typedef struct {

} x64_thread_ctx;

typedef struct {
        gsdproc_common_header hdr;
        void* rsp;
        uintptr_t cr3;
        
} gsdprocx64;

#endif