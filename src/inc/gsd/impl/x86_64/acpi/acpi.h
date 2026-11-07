#ifndef acpi_h
#define acpi_h
#include <gsd/gsd-common.h>

typedef struct __attribute__((packed)) s_acpi_table_common_hdr{
        char signature[4];
        u32 length;
        u8 revision;
        u8 checksum;
        u8 oemid[6];
        u8 oem_table_id[8];
        u32 oem_revision;
        u32 creator_id;
        u32 creator_revision;
} acpi_table_common_hdr;




#endif