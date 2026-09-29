#ifndef boottab_x86_64_h
#define boottab_x86_64_h
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

typedef struct {
        u32 type;
        uintptr_t pstart;
        uintptr_t vstart;
        u64 pagec;
        u64 attrib;
} efi_descriptor;
typedef struct 
{
        u32 signature;
        enum x86_64_bootmode mode;
        union {
                e820_descriptor* e820;
                efi_descriptor* efi;
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
        uintptr_t gsdpage_base;
        size syspagec;

        void* rsdp;
} x86_64_BootTable;

#endif