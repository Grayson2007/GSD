#ifndef procx64_h
#define procx64_h
#include <gsd-common.h>
#include <gsd-memory.h>
typedef struct {
        
} x64_thread_ctx;

typedef struct {

        gsdpage* vaddr;
        uintptr_t cr3;
        void* ksp;

} gsdprocx64;

#endif