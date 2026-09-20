#ifndef gsd_memory_h
#define gsd_memory_h
#include "gsd-common.h"

#define gsdpg_capability_mask ~((1 << 5)-1)
enum gsdpage_attrib {
        gsdpg_active = 1,
        gsdpg_read = (1 << 1),
        gsdpg_write = (1 << 2),
        gsdpg_exec = (1 << 3),
        gsdpg_kernel = (1 << 4),
        gsdpg_pgtable = (1 << 5),
        gsdpg_dma_capable = (1 << 6)
};

typedef struct s_gsdpage {
        uintptr_t addr;
        gsd_procid owning_process;
        int order;
        int attrib;
        struct s_gsdpage* nextfree;
        struct s_gsdpage* prevfree;
} gsdpage;


typedef struct s_memtree_entry 
{

        uintptr_t minptr;
        uintptr_t maxptr;
        uintptr_t priority;
        gsdpage* page;
        struct s_memtree_entry* low;
        struct s_memtree_entry* high;
        struct s_memtree_entry* parent;
} memtree_entry;


typedef struct s_memtree {
        memtree_entry* root;
} memtree;

status memtree_insert(memtree* self,uintptr_t minptr,uintptr_t maxptr,gsdpage* page);
status memtree_delete(memtree* self,uintptr_t ptr);
status memtree_find(memtree* self,uintptr_t ptr,memtree_entry** outptr);

enum gsd_allocate_pages_status {
        alloc_pages_no_mem = 1,
        alloc_pages_forbidden = 2
};

status GsdAllocatePage(int attrib,int order,gsdpage** outpage);
void GsdFreePage(gsdpage* page); 



void GsdQueryPageInfo(gsdpage** freelistptr,int* max_order);



#define GSD_MEM_ORDER 12
#endif