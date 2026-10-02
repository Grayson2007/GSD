#include <impl/x86_64/dt.h>

void EncodeGdtEntry(GDT_ENTRY_INFO* info,u64* entryptr)
{
        u64 lim_low = info->limit & 0xFFFF;
        u64 base_low = (u64)(info->base & 0xFFFFFF) << 16;
        u64 ab = 0;
        ab |= (1ul << 47); // Present bit 
        ab |= (u64)(info->PL & 3) << 45;
        if(info->system) {
                ab |= (1ul << 44);
                switch(info->system_type) {
                        case gdt_ldt:
                                ab |= 0x2ul << 40;
                                break;
                        case gdt_tss_avl:
                                ab |= 0x9ul << 40;
                                break;
                        case gdt_tss_busy:
                                ab |= 0xBul << 40;
                }
        }
        else 
        {
                if(info->writable) {
                        ab |= (1ul << 41);
                }
                if(info->executable) {
                        ab |= (1ul << 43);
                }

        }
        // flags
        u64 flags = info->scale_4k ? 0b1010 : 0b0010;
        flags <<= 52;
        flags |= (u64)((u64)info->limit & 0xF0000 >> 16) << 48;
        entryptr[0] = flags | lim_low | base_low | ab | (((info->base >> 24) & 0xFF) << 56);
        entryptr[1] = ((info->base >> 32) & 0xFFFFFFFF);

}
void EncodeTSS(TSS_INFO* info,void* base) {
        u64* outptr = (u64*)base;

}

void EncodeIDT(IDT_INFO* info,u64* entryptr) {
        u64 offset_lowbits = (info->addr & 0xFFFF);
        u64 segment = (u64)info->segment << 16;
        u64 encoded_flags = 0;
        encoded_flags |= ((u64)(info->ist & 7) << 32);
        encoded_flags |= info->gate_type == idt_gt_trap ? ((u64)0xF << 40) : ((u64)0xE << 40);
        encoded_flags |= (u64)info->pl << 45;
        encoded_flags |= (1ul << 47);
        u64 offset_midbits = ((info->addr >> 16) & 0xFFFF) << 48;
        u64 offset_highbits = info->addr >> 32;
        entryptr[0] =  offset_lowbits | segment | encoded_flags | offset_midbits;
        entryptr[1] = offset_highbits;
}