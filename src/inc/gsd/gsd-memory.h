#ifndef gsd_memory_h
#define gsd_memory_h
#include "gsd-common.h"


#define gsdpg_capability_mask ~((1 << 10)-1)

enum gsdpage_attrib {
        gsdpg_valid = 1,
        gsdpg_active = (1 << 1),
        gsdpg_read = (1 << 2),
        gsdpg_write = (1 << 3),
        gsdpg_exec = (1 << 4),
        gsdpg_kernel = (1 << 5),
        gsdpg_pgtable = (1 << 6),
        gsdpg_page_structure = (1 << 7),
        gsdpg_firmware_memmap = (1 << 8),
        gsdpg_dma_capable = (1 << 9)


        
};

enum gsd_vmem_attributes {
        vmem_read = (1 << 1),
        vmem_write = (1 << 2),
        vmem_execute = (1 << 3),
        vmem_anonymous = (1 << 4),
        vmem_share = (1 << 5),
}; 


typedef struct {

} gsd_vmrange;



// GSD's memtree is Inspired by the maple tree structure in the Linux kernel 
#define MEMTREE_SLOTS 4

typedef struct s_gsd_vmemtree {
        struct s_gsd_vmemtree* parent;
        uintptr_t ranges[MEMTREE_SLOTS*2];
        void* slots[MEMTREE_SLOTS]; // e
} gsd_vmemtree;


typedef struct s_gsdpage {
        uintptr_t addr;
        gsd_procid owning_process;
        int order;
        int attrib;
        struct s_gsdpage* nextfree;
        struct s_gsdpage* prevfree;
} gsdpage;








enum gsd_allocate_pages_status {
        alloc_pages_no_mem = 1,
        alloc_pages_forbidden = 2
};

status GsdAllocatePage(int attrib,int order,gsdpage** outpage);
status GsdFreePage(gsdpage* page);
status KVaddrMap(gsdpage* pg,uintptr_t* out);
status KVaddrFree(void* virt);
status KVaddrToPage(void* virt,gsdpage** outpg);
status KPageAlloc(size pagec,void* outbase);
status KPageFree(void* base);
void GsdQueryPageInfo(gsdpage** freelistptr,int* max_order);
void GsdMapPages(gsdpage* pg);







#endif