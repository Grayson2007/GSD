#ifndef gsd_memory_h
#define gsd_memory_h
#include "gsd-common.h"


#define SBA_BLOCK_COUNT SYS_PAGE_SIZE


enum gsdpage_status_flags : u64 {
        gsdpg_s_present = 1, // Clear means this page is free. 1 Means allocated 
        gsdpg_s_r = (1 << 1), // Readable bit 
        gsdpg_s_w = (1 << 2), // Writable bit 
        gsdpg_s_x = (1 << 3), // Executable bit 
        gsdpg_s_kernelpg = (1 << 4), // Page belongs to the kernel 
        gsdpg_s_page_table = (1 << 5), // Page is a page table 
        gsdpg_s_slab_allocator = (1 << 6), // Page is a slab allocator
        gsdpg_s_gsdpage = (1 << 7), // Page holds mem_map data 
        gsdpg_s_uc = (1 << 8), // Uncached 
        gsdpg_s_wc = (1 << 9), // Write Combine
        gsdpg_s_wt = (1 << 10), // Write Through 
        gsdpg_s_wb = (1 << 11), // Writeback
        gsdpg_s_combined = (1 << 12),
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
        gsdpg_a_valid = (1ul << 52), // If this bit is clear the page is ignored
        gsdpg_a_identmap = (1ul << 53), // If set this page MUST be idenity mapped when mapped in a address space 
        gsdpg_a_slack = (1ul << 54), // A Slack page is a special page that can only ever be Order 0
        gsdpg_a_reserved = (1ul << 55) // Page is reserved memory 
};



// if tj

#define gsd_normal_memory gsdpg_c_gpmem 
#define gsd_normal_low_memory gsdpg_a_low_mem | gsdpg_c_gpmem
#define page_capability_mask 0x0000FFFF00000000
#define page_attribute_mask 0xFFFF000000000000
#define page_status_mask 0x0000000FFFFFFFF

#define gsdpg_rw gsdpg_s_r | gsdpg_s_w
#define gsdpg_valid gsdpg_s_present | gsdpg_a_valid
#define gsdpg_slab_allocator gsdpg_rw | gsdpg_s_slab_allocator | gsdpg_s_kernelpg
#define gsdpage_kpgtable_attrib gsdpg_rw | gsdpg_s_page_table | gsdpg_s_kernelpg | gsdpg_valid
#define gsdpg_kpage_attrib gsdpg_s_kernelpg | gsdpg_rw
#define gsdpg_kcode_attrib gsdpg_s_kernelpg | gsdpg_rw | gsdpg_s_x
#define gsdpg_mem_map_attrib gsdpg_valid | gsdpg_a_reserved |gsdpg_s_kernelpg | gsdpg_rw


extern struct rb_tree K_HEAP_VMM;
extern struct rb_tree K_MODULE_VMM;


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
                        struct s_gsdpage* next_slab; // Allows chaining slab allocators toghether 
                        struct s_gsdpage* prev_slab;
                } slabinfo;
                struct {
                        struct s_gsdpage* prev_pt;
                        struct s_gsdpage* next_pt;
                        int lvl;
                } ptinfo;

                struct {
                        size slab_size;
                        struct s_gsdpage* sba_page;
                } sbainfo;
        };
        int rc;
        uintptr_t _pad[2]; // padding to make this a power of 2 so it's divisible 
}  gsdpage;


typedef struct {
        size prevsize;
        size blksz;
        u32 attrib;
        gsd_atomic spinlock; 
} heap_tag;

status kheap_create(size totalsz,heap_tag** outbase);
status kheap_destroy(void* heap_pbase);
void* kheap_malloc(heap_tag* heap_base,size bytes);









status GsdMemoryInit(uintptr_t max_paddr);
#define gsdpage_entrys_per_page SYS_PAGE_SIZE/sizeof(gsdpage)
gsdpage* get_process_memory_tree();
gsdpage* get_corrisponding_gsdpage(uintptr_t paddr);
uintptr_t get_paddr_by_gsdpage(gsdpage* pg);
status gsd_allocate_page(u64 attribute,int order,gsdpage** outpage);
void gsd_free_page(gsdpage* page);
status vmm_free_page(struct rb_tree* vmm,uintptr_t vaddr);
status vmm_alloc_page(struct rb_tree* vmm,gsdpage* page,uintptr_t vmin,uintptr_t vmax);
status vmm_get_page(struct rb_tree* vmm,uintptr_t vaddr,gsdpage** outpg);
status mmu_unmap_page(gsdpage* page);
status mmu_map(gsdpage* page);
void mmu_unmap(uintptr_t vaddr);
bool vaddr_test(uintptr_t virt);




status kpgalloc(int order,uintptr_t** outptr);
void kpgfree(uintptr_t addr);


void* kmalloc(size bytes);
void* kralloc(void* ptr,size bytes);
void kfree(void* ptr);

enum arenaflags {
        arena_flag_allocated = (1 << 0),
        arena_flag_mmapd = (1 << 1),
        arena_flag_spinlock = (1 << 2)
};

struct s_arena_tag {
        size blksz;
        size prevsz;
};

#endif