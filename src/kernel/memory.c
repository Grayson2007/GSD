#include "gsd-memory.h"

static gsdpage* kmem_vregion_base;




#define mem_map_init_none 0
#define mem_map_init_stage1_done 1
#define mem_map_init_completed 2  
static int mem_map_init_status = mem_map_init_none;
static gsd_atomic freelist_mutex;
static gsd_atomic mem_map_mutex;
static gsdpage* freelist[SYS_MEMORY_ORDER];
static  gsdpage* mem_map = (gsdpage*)(MEM_MAP_ADDRESS);
static size syspagec;


static status gsd_locate_page(u64 attrib,int order,gsdpage** out) {
        gsdpage* pg = NULL;
        u64 capabilities = attrib & page_capability_mask;
        while(order < SYS_MEMORY_ORDER) {
                if(!freelist[order]) { order++; continue;} 
                pg = freelist[order];
                while(pg->flags & capabilities != capabilities) { // Find a free page in the current order
                        /*  If last page on this order set pg to NULL
                        * so the parent loop knows no page was found on this level to go up the freelist 
                        */ 
                        if(!pg->nextfree) { pg = NULL; order++; break;} 
                        pg = pg->nextfree; 
                }
                if(!pg) { continue;} 
                // We found one! 
                break;
        }
        *out = pg; // Page remains NULL if all else failed 
        return !pg ? gsd_out_of_resources : gsd_ok;
}

static void split_page(gsdpage* pg) {
        index pg_idx = pg-mem_map;
        index buddy_pg_idx = pg_idx+(1 << (pg->order-1));
        if(freelist[pg->order]  == pg) { // Update freelist root if this is the root of that order's freelist 
                freelist[pg->order] = pg->nextfree;
        }
        pg->order--;

        // setup the buddy page this page wont be added as it's going to be allocated 
        gsdpage* buddy = &mem_map[buddy_pg_idx];
        buddy->order = pg->order;
        buddy->flags &= ~(gsdpg_s_present); // disable present bit (Flags are setup by loader but just a percaution)
        buddy->nextfree = freelist[buddy->order]; 
        if(freelist[buddy->order]) {
                freelist[buddy->order]->prevfree = buddy;
        }
        freelist[buddy->order] = buddy;


        // Update freelist pointers 
        if(pg->nextfree) { 
                pg->nextfree->prevfree = pg->prevfree;
        }
        if(pg->prevfree) {
                pg->prevfree->nextfree = pg->nextfree;
        }
        pg->nextfree = NULL; // Clear out freelist pointers 
        pg->prevfree = NULL;

}

status GsdAllocatePage(u64 attribute,int order,gsdpage** outpage) {

        status s;
        gsdpage* found_page = NULL;
        u64 status_flags = attribute & page_status_mask;
        // find a free page that satisfys our requirements
        s = gsd_locate_page(attribute,order,&found_page);
        status_assert_ok(s) { return s;} // If no page exists this function will return gsd_out_of_resources from gsd_locate_page

        // Now to split and add buddies to freelist 
        while(found_page->order > order) {
                split_page(found_page);
        }


        found_page->flags = (found_page->flags & page_attribute_mask) | status_flags | gsdpg_s_present;
        *outpage = found_page;
        return gsd_ok;
}

// Tries to combine a page 
// If succeeded the pg pointer variable from the caller is updated 
// it will either stay the same or become the buddy page 
static bool combine_page(gsdpage** pg_ptr) {
        gsdpage* page = *pg_ptr;
        if(page->flags & gsdpg_s_reserved) {return false;} // was this marked as reserved? If so NOPE 
        if(page->order+1 >= SYS_MEMORY_ORDER) { return false;} // Already at max order 
        index pgi = page-mem_map;
        index buddy_index = (pgi ^ (1ul << page->order));
        gsdpage* buddy = &mem_map[buddy_index];
        if(buddy->flags & gsdpg_s_present) { return false; } // page is not free so we cannot combine 
        if(buddy_index < pgi) { // this is a buddy
                *pg_ptr = buddy;
                page->flags &= ~(page_status_mask); 
                page->order = 0;
                buddy->order++;
                buddy->flags &= ~(page_status_mask);
                return true;
        }
        page->flags &= ~(page_status_mask); 
        page->order++;
        buddy->order = 0;
        buddy->flags &= ~(page_status_mask);
        return true;
}


void GsdFreePage(gsdpage* page) {
        while(combine_page(&page));
        page->nextfree = freelist[page->order];
        if(freelist[page->order]) {
                freelist[page->order]->prevfree = page;
        }
        freelist[page->order] = page;
}


// Mark a address range as reserved 
void GsdMarkAddress(uintptr_t base_address,int order,u64 attrib) {

        if(mem_map_init_status != mem_map_init_stage1_done) { return;} // safety gate to prevent this being called after the memory manager was fully initalised
        index page_index = (base_address/SYS_PAGE_SIZE)/(1ul << order); // Setup an aligned down base address 
        index buddy_index = page_index ^ (1ul << order);
        if(page_index > syspagec) { return;}
        mem_map[page_index].order = order;
        mem_map[page_index].flags |= (attrib & page_status_mask) | gsdpg_s_present;
        // add the buddy to the freelist
        mem_map[buddy_index].order = order;
        mem_map[buddy_index].flags &= (attrib & page_status_mask);
        mem_map[buddy_index].flags |= gsdpg_s_present | gsdpg_s_reserved;
        if(freelist[order]) {
                freelist[order]->prevfree = &mem_map[buddy_index];
        }
        mem_map[buddy_index].nextfree = freelist[order];
        freelist[order] = &mem_map[buddy_index];
        
        for(index i = page_index+1; i < buddy_index; i++) {
                mem_map[page_index].flags |= gsdpg_s_reserved;
        }
}

// -- Memory Manager Initalisation -- // 

// Stage 1 Initalisation
// Sets all blocks to free at Order 0 to allow for Block Marking
// (Only really do this if you plan on like reserving certain physical addresses )
// Example: GSD on x86_64/x86 uses physical addresses 0x1000-0x100000 as a Idenity mapped legacy
// Space to Initalise Other Cores and marks the first megabyte as Used so it's not touched 


// Find max_order blocks 
// If block is fragmented place the slack pages on the order 0 freelist
// After that we will map a heap area for the kernel
// 





status GsdMemoryInit(uintptr_t max_paddr) {
        index max_pageidx = max_paddr/SYS_PAGE_SIZE;
        index max_order_pagec = (1 << SYS_MEMORY_ORDER);
        bool max_order_free;
        index max_order_blocks = max_pageidx/max_order_pagec;
        index slack_blocks = max_pageidx % max_order_pagec;
        
        for(index b = 0; b < max_order_blocks; b++)
        {
                max_order_free = true;
                index base_idx = b*max_order_pagec;
                for(index p = 0; p < max_order_pagec; p++)
                {
                        if(!vaddress_isvalid((uintptr_t)&mem_map[base_idx]+p)) { max_order_free = false; break;} 
                        if(!mem_map[base_idx+p].flags & gsdpg_a_valid
                        || mem_map[base_idx+p].flags & (gsdpg_a_readonly | gsdpg_a_readonly)
                        || mem_map[base_idx+p].flags & (gsdpg_s_reserved & gsdpg_s_present)) { max_order_free = false; break;}  
                }


                if(!max_order_free)
                {
                        for(index p = 0; p < max_order_pagec; p++)
                        {
                                if(!vaddress_isvalid((uintptr_t)&mem_map[base_idx]+p)) { continue;}
                                if(!mem_map[base_idx+p].flags & gsdpg_a_valid) { continue;}
                                if(mem_map[base_idx+p].flags & (gsdpg_a_readonly | gsdpg_a_readonly)) { continue;}
                                if(mem_map[base_idx+p].flags & (gsdpg_s_reserved & gsdpg_s_present)) { continue;}
                                if(freelist[0]) {
                                        freelist[0]->prevfree = &mem_map[base_idx+p];
                                }
                                mem_map[base_idx+p].nextfree = freelist[0];
                                freelist[0] = &mem_map[base_idx+p];
                        }
                }
                else 
                {
                        if(freelist[SYS_MEMORY_ORDER-1]) {
                                freelist[SYS_MEMORY_ORDER-1]->prevfree = &mem_map[base_idx];
                        } 
                        mem_map[base_idx].nextfree = freelist[SYS_MEMORY_ORDER-1];
                        freelist[SYS_MEMORY_ORDER-1] = &mem_map[base_idx];
                }
        }
        // Check and add slack pages at the end if they exist 
        index base_idx = max_order_blocks*max_order_pagec;
        for(index p = 0; p < slack_blocks; p++)
        {
                if(!vaddress_isvalid((uintptr_t)&mem_map[base_idx]+p)) { continue;}
                if(!mem_map[base_idx+p].flags & gsdpg_a_valid) { continue;}
                if(mem_map[base_idx+p].flags & (gsdpg_a_readonly | gsdpg_a_readonly)) { continue;}
                if(mem_map[base_idx+p].flags & (gsdpg_s_reserved & gsdpg_s_present)) { continue;}
                if(freelist[0]) {
                        freelist[0]->prevfree = &mem_map[base_idx+p];
                }
                mem_map[base_idx+p].nextfree = freelist[0];
                freelist[0] = &mem_map[base_idx+p];
        }
        return gsd_ok;
}

uintptr_t GsdGetPaddr(gsdpage* pg) {
        if(pg < mem_map) { return (uintptr_t)NULL;}
        uintptr_t out = ((uintptr_t)(pg-mem_map))*SYS_PAGE_SIZE;
        return out;
}