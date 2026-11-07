#ifndef dt_h
#define dt_h
#include <gsd-common.h>
enum syst {
        gdt_ldt,
        gdt_tss_busy,
        gdt_tss_avl
};

typedef struct {
        uintptr_t base;
        u32 limit;
        int PL;
        int system_type;
        bool system;
        bool executable;
        bool writable;
        bool scale_4k;
} GDT_ENTRY_INFO;

void EncodeGdtEntry(GDT_ENTRY_INFO* info,u64* entryptr);

typedef  struct {
        u32 _r0;
        struct __attribute__((packed)) {
                u32 lo; 
                u32 hi;
        }rsp[3];
        u32 _r1[2];
        struct __attribute__((packed)) {
                u32 lo;
                u32 hi;      
        } IST[7];
        u32 _r2[2];
        u16 _r3;
        u16 iopb; // always 104
} __attribute__((packed)) TSS;

void EncodeTSS(TSS* tssptr,u64* gdtptr);


enum gate_types {
        idt_gt_trap,
        idt_gt_int
};
typedef struct {
        uintptr_t addr;
        u16 segment;
        int ist;
        int gate_type;
        int pl;
} IDT_INFO;

void EncodeIDT(IDT_INFO* info,u64* entryptr);



#endif