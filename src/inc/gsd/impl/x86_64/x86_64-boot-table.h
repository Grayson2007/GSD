#ifndef boottab_x86_64_h
#define boottab_x86_64_h
#include <gsd-memory.h>

enum x86_64_bootmode {
        bm_legacy,
        bm_efi
};

enum x86_64_memtype {
        mt_regular,
        mt_firmrsvd,
        mt_acpi,
        mt_mmio,
        mt_rom,
        mt_unusable,
        mt_loadercode,
        mt_loaderdata
};


#define boottab_signature 'BOOT'

typedef struct {
        u64 base;
        u64 size;
        u32 type;
        u32 atrib;
} e820_descriptor;


// Fuck this uefi piss 
typedef size efi_status;
#define efiapi __attribute__((ms_abi))
typedef uchar efi_bool;
typedef u16 efi_char16;
typedef struct {
        u32 type;
        uintptr_t pstart;
        uintptr_t vstart;
        u64 pagec;
        u64 attrib;
} efi_descriptor;


typedef struct {
        u16 yr;
        u8 mon;
        u8 day;
        u8 hour;
        u8  min;
        u8 sec;
        u8 pad0;
        u32 nanosec;
        u16 timezone;
        u8 daylight;
        u8 pad1;
} efi_time;

typedef struct {
        u32 rev;
        u32 accuracy;
        efi_bool sets_to_zero;
} efi_time_capabilties;
typedef struct {
        u64 sig;
        u32 rev;
        u32 tblesize;
        u32 crc32;
        u32 rsvd;
} efi_table_header;


typedef struct {
        u32 data1;
        u16 data2;
        u16 data3;
        u8 data4[8];
} efi_guid;
enum efi_reset_types {
        efi_reset_cold,
        efi_reset_warm,
        efi_reset_shutdown,
        efi_reset_platform
};
typedef struct {
        efi_table_header hdr;
        efi_status (*efiapi GetTime)(efi_time* out_time,efi_time_capabilties* out_capabilities);
        efi_status (*efiapi SetTime)(efi_time* Time);
        efi_status (*efiapi GetWakeTime)(efi_bool* out_enabled,efi_bool* out_pending,efi_time* out_time);
        efi_status (*efiapi SetWakeTime)(efi_bool enable,efi_time* time);
        uintptr_t garb0; // These wont be needed here 
        uintptr_t garb1;
        efi_status (*efiapi GetVar)(efi_char16* name,efi_guid* vendorid,u32* attrib,size* datsz,void* data);
        efi_status (*efiapi GetNextVarName)(size* namesz,efi_char16* name,efi_guid* vendor_id);
        efi_status (*efiapi SetVar)(efi_char16* name,efi_guid* vendor,u32 attrib,size datasz,void* data);
        efi_status (*efiapi NextMonoCount)(u32* count);
        efi_status (*efiapi ResetSystem)(enum efi_reset_types type,efi_status status,size datasz,void* data);
        uintptr_t garb2;
        uintptr_t garb3;  
        efi_status (*efiapi QueryVarInfo)(u32 attrib,u64* MaxVarStoreSz,u64* RemVarStoreSz,u64* MaxVarSz);
} efi_runtime_table;

#define efi_success 0
#define efi_load_error 1
#define efi_inv_param 2
#define efi_unsupported 3
#define efi_bad_buffer 4
#define efi_buff_too_small 5
#define efi_not_rdy 6
#define efi_dev_error 7
#define efi_wp 8
#define efi_no_resources 9
#define efi_corrupted 10
#define efi_full 11
#define efi_no_media 12
#define efi_media_changed 13
#define efi_not_found 14
#define efi_deined 15
#define efi_no_response 16
#define efi_no_mapping 17
#define efi_aborted 21
// end of the uefi piss
typedef struct 
{
        u32 signature;
        enum x86_64_bootmode mode;
        union 
        {
                struct {
                        efi_descriptor* base;
                        size desc_sz;
                        size map_sz;
                        u64 key;
                        u32 ver;
                } efi;
                e820_descriptor* e820;
        } mmap;
        size mmapdescriptorc;
        struct {
                void* pbase;
                size fbsz;
                u32 w;
                u32 h;
                u32 depth;
                u32 pitch;
                u32 ppsl;
                u32 rmask;
                u32 gmask;
                u32 bmask;

        } fb;
        struct {
                uintptr_t pbase;
                size sz;
                size pagec;
        } irfs;

        struct {
                uintptr_t pbase;
                size ksize;
                size kpagec;
        } kernel_bin_info;
        struct {
                uintptr_t base;
                size pagec;
        } loader;

        union {
                efi_runtime_table efirt;
        };
        uintptr_t max_physical_address; // Max address of the mem_map structure 
        void* rsdp;
} x86_64_BootTable;
#define KBT_POINTER ((x86_64_BootTable*)(BOOT_TABLE_ADDRESS))

// Early Memory Functions 
static status _EarlyGsd_FindFreePages();
static status _EarlyGsd_FreeMemBlock();
static status _EarlyGsd_SetupPmmStage2();
static status _EarlyGsd_Handoff();
#endif