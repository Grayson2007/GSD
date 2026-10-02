#ifndef cpux64_h
#define cpux64_h
#include <gsd-common.h>
enum CPU_states {
        CPU_Waking,
        CPU_Awating_job,
        CPU_In_ISR,
        CPU_Servicing_Kerncall,
        CPU_Flag_Enabled = (1 << 31),
        
};


typedef struct {
        gsd_coreid system_id;
        u16 apic_cluster;
        u16 apic_processor;
        gsd_threadid executing_thread;
        int state;
        void* tss;
        struct __attribute__((packed,aligned(16))) s_IDT {
                u16 sz;
                u64 base;
        } IDTR;
} cpux64; 

#endif