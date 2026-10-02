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

typedef struct {
        uintptr_t IST[7];
        uintptr_t RSP[3];
        u8* IOPB_BUFFER;
        u32 tss_size;
        u8* iopb_payload;
} TSS_INFO;

void EncodeTSS(TSS_INFO* info,void* base);


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