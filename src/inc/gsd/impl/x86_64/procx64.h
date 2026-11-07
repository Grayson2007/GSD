#ifndef procx64_h
#define procx64_h
#include <gsd-common.h>
#include <gsd-memory.h>
#include <gsd-thread.h>

typedef u16 thread_burst;
#define cycles_per_burst 5000
#define transition_latency 200
#define burst_max 500



typedef struct {
        gsdthread_hdr hdr;
        u16 attrib;
        thread_burst burst_unit;
        gsd_procid cpu;
        void* ctx_sp;
} x64_thread_ctx;

typedef struct {
        gsdproc_common_header hdr;
        uintptr_t cr3;
        void* ksp;
} gsdprocx64;


void SetThreadTimer(u32 cycles);
void CancelThreadTimer();


#endif