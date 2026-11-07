#ifndef cpux64_h
#define cpux64_h
#include <gsd-common.h>
#include <gsd-thread.h>
#include <impl/x86_64/dt.h>
enum CPU_states {
        CPU_Waking,
        CPU_Awating_job,
        CPU_In_ISR,
        CPU_Servicing_Kerncall,
        CPU_Flag_Enabled = (1 << 31),
        
};


typedef struct {
        gsdcore hdr;
        u16 apic_cluster;
        u16 apic_processor;
        int state;
        TSS* tss;
        struct __attribute__((packed)) s_IDT {
                u16 sz;
                u64 base;
        } IDTR;
} cpux64; 


#endif