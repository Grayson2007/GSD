#include <impl/x86_64/acpi/acpi.h>



enum madt_entry_types {
        et_proc_lapic,
        et_ioapic,
        et_ioapic_iso,
        et_ioapic_nmisrc,
        et_lapic_nmi,
        et_lapic_addr_overide,
        et_proc_x2apic = 9
};

enum madt_flags {
        fl_default = 0,
        fl_active_high = 0b01,
        fl_active_low = 0b11,
        fl_edge_trigger = 0b0100,
        fl_level_trigger = 0b1100,
        fl_processor_enabled = 1,
        fl_online_capable = 2,
};


typedef struct {
        acpi_table_common_hdr common;
        u32 lapic_address;
        u32 legacy_pic_installed;
} madt_hdr;


void x64_install_cpu(u32 logical_id,u32 apic_id,u32 flags) {

}

void x64_install_ioapic(void* base,u32 id,u32 gsi_base) 
{

}

void load_cpu_namespace(madt_hdr* madt) {
        u8* endptr = (u8*)madt+(madt->common.length);
        u8* cptr = (u8*)madt+sizeof(madt_hdr);
        while(cptr < endptr) {
                u8 type = *cptr;
                cptr++;
                size entry_size = *cptr;
                cptr++;

                switch(type)
                {
                        case et_proc_lapic:
                                u8 logical_id = *cptr;
                                cptr++;
                                u8 lapic_id = *cptr;
                                u32 flags = *cptr;
                                x64_install_cpu((u32)logical_id,(u32)lapic_id,(u32)flags);
                                break;
                        case et_proc_x2apic:

                        case et_ioapic:

                        case et_ioapic_iso:

                        case et_ioapic_nmisrc:

                        case et_lapic_addr_overide:

                        case et_lapic_nmi:

                }
                cptr += entry_size-2;
        }
}       