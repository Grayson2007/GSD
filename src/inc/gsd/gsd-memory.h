#ifndef gsd_memory_h
#define gsd_memory_h
#include "gsd-common.h"


enum gsdpage_status_flags : u64 {
        gsdpg_s_present = 1, // Clear means this page is free. 
        gsdpg_s_r = (1 << 1), // Read bit 
        gsdpg_s_w = (1 << 2), // Writable bit 
        gsdpg_s_x = (1 << 3), // Executable bit 
        gsdpg_s_kernelpg = (1 << 4), // Page belongs to the kernel 
        gsdpg_s_page_table = (1 << 5), // Page is a page table 
        gsdpg_s_slab_allocator = (1 << 6), // Page is being used as a slab allocator
        gsdpg_s_valid = (1 << 7), // Is this a valid page? 
        gsdpg_s_gsdpage = (1 << 8), // Does this page hold gsdpage descriptors
        gsdpg_s_uc = (1 << 9),
        gsdpg_s_wc = (1 << 10),
        gsdpg_s_wt = (1 << 11),
        gsdpg_s_wb = (1 << 12),
        gsdpg_s_reserved = (1ul << 31), // This is an extra protection bit (Mainly used for pages that hold the mem_map and base kernel code/data)
        // Capability Bits
        gsdpg_c_gpmem = (1ul << 32), // Page can be used as normal memory
        gsdpg_c_uc = (1ul << 33), // Page can be Uncached 
        gsdpg_c_wc = (1ul << 34), // Page can be Cached with Write Combine
        gsdpg_c_wt = (1ul << 35), // Page can be Cached with Write Through
        gsdpg_c_wb = (1ul  << 36), // Page can be Cached with Write Back
        gsdpg_c_wp = (1ul << 37), // Page can be Write Protected
        gsdpg_c_rp = (1ul << 38), // Page can be Read Protected
        gsdpg_c_xp = (1ul << 39), // Page can be Execute Protected 
        gsdpg_c_nv = (1ul << 40), // Page can be Non Volatile 
        gsdpg_c_more_reliable = (1ul << 41),  // Page can be more Relliable
        gsdpg_c_dma = (1ul << 42), // Page can be used in DMA Operations 
        // Attribute Bits 
        gsdpg_a_readonly = (1ul << 48), // Page MUST be readonly
        gsdpg_a_cd = (1ul << 49), // Page MUST be non Cached 
        gsdpg_a_rp = (1ul << 50), // Page MUST be read protected
        gsdpg_a_low_mem = (1ul << 51), // Page is Low memory
        gsdpg_a_valid = (1ul << 52), // If this bit is clear the page is ignored (Only used in Dummmy Slack entries leftover from the bootloader from allocating the page stuctures)
        gsdpg_a_identmap = (1ul << 53) // If set this page MUST be idenity mapped when mapped in a address space 

};

enum gsd_memtypes {
        gsdmem_Invalid,
        gsdmem_Conventional,
        gsdmem_FirmwareRsvd
};


// if tj
#define page_invalid 0 
#define gsd_normal_memory gsdpg_c_gpmem 
#define gsd_normal_low_memory gsdpg_a_low_mem | gsdpg_c_gpmem
#define page_capability_mask 0x0000FFFF00000000
#define page_attribute_mask 0xFFFF000000000000
#define page_status_mask 0x00000007FFFFFFF

#define gsdpg_rw gsdpg_s_r | gsdpg_s_w
#define gsdpg_valid_present gsdpg_s_valid | gsdpg_s_gsdpage
#define gsdpage_kpgtable_attrib gsdpg_rw | gsdpg_s_page_table | gsdpg_s_kernelpg | gsdpg_valid_present
#define gsdpg_kpage_attrib gsdpg_s_kernelpg | gsdpg_valid_present | gsdpg_rw
#define gsdpg_kcode_attrib gsdpg_s_kernelpg | gsdpg_valid_present | gsdpg_rw | gsdpg_s_x
#define gsdpg_mem_map_attrib gsdpg_valid_present | gsdpg_s_reserved |gsdpg_s_kernelpg | gsdpg_rw

typedef struct s_gsdpage 
{
        struct rb_node rb_link;
        gsd_procid owning_process;
        u32 order;
        u64 flags;
        uintptr_t vaddr;
        struct s_gsdpage* nextfree;
        struct s_gsdpage* prevfree;
        union { // paramters based on page states
                struct { // state while it's being used as a simple slab allocator. 
                        size slab_size;
                        size slabs_free;
                        index freelist_start;
                } slabinfo;

                struct {
                        struct s_gsdpage* prev_pt;
                        struct s_gsdpage* next_pt;
                        int lvl;
                } ptinfo;
        };
        uintptr_t _pad[5]; // padding to make this a power of 2 so it's divisible 
}  gsdpage;





#define gsdb_memblock_ident_attrib gsdpg_a_identmap | gsdpg_s_kernelpg | gsdpg_s_present | gsdpg_s_w | gsdpg_s_valid 


#define gsdpage_entrys_per_page SYS_PAGE_SIZE/sizeof(gsdpage)
gsdpage* get_process_memory_tree();
uintptr_t GsdGetPaddr(gsdpage* pg);
status GsdAllocatePage(u64 attribute,int order,gsdpage** outpage);
void GsdFreePage(gsdpage* page);
status vmm_free_page(uintptr_t vaddr);
status vmm_alloc_page(gsdpage* page,uintptr_t vmin,uintptr_t vmax);
status mmu_map(gsdpage* page);
void mmu_unmap(uintptr_t vaddr);
bool vaddress_isvalid(uintptr_t virt);

status kpgalloc(int order,uintptr_t** outptr);
void kpgfree(uintptr_t addr);
// @r:/dev/nv
#endif