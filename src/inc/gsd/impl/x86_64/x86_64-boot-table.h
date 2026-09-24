#ifndef boottab_x86_64_h
#define boottab_x86_64_h

enum x86_64_bootmode {
        bm_legacy,
        bm_efi
};
#define boottab_signature 'BOOT'

typedef struct {

} e820_style_memory_map;
typedef struct 
{
        u32 signature;
        enum x86_64_bootmode mode;

        union {

        } memmap;
        struct {
                void* pbase;
                u32 w;
                u32 h;
                u32 depth;
                u32 pitch;
                u32 ppsl;
                u32 rmask;
                u32 gmask;
                u32 bmask;
        } fb;
        union {

        } firm_info;
        uintptr_t gsdpage_base;
        size syspagec;
        void* rsdt;
} x86_64_BootTable;

#endif