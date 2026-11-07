#include <gsd-memory.h>
#include <gsd-thread.h>
static gsdpage* kmem_vregion_base;
static spinlock freelist_spins[SYS_MEMORY_ORDER+1]; 
static gsdpage* freelist[SYS_MEMORY_ORDER+1];
static gsdpage* mem_map = (gsdpage*)(MEM_MAP_ADDRESS);
static size syspagec;

static status gsd_locate_page(u64 attrib,int order,gsdpage** out) {
        bool found_page = false;
        gsdpage* curr_page;
        u64 capabilities = attrib & page_capability_mask;
        for(int o = order; order <= SYS_MEMORY_ORDER; o++) 
        {
                spinlock_aquire(&freelist_spins[o]);
                curr_page = freelist[o];
                while(curr_page) {
                        // test the page
                        if(curr_page->flags & capabilities != capabilities) {
                                curr_page = curr_page->nextfree;
                                continue;
                        }
                        found_page = true;
                        break;
                }
        
                if(found_page) 
                { 
                        // if we found our page 
                        // remove curr_page from it's freelist and update pointers
                        if(curr_page->prevfree) {
                                curr_page->prevfree->nextfree = curr_page->nextfree;
                        } else { // this is the head
                                freelist[o] = curr_page->nextfree;
                        }
                        if(curr_page->nextfree) {
                                curr_page->nextfree->prevfree = curr_page->prevfree;
                        }

                        *out = curr_page;
                        spinlock_release(&freelist_spins[o]);
                        return gsd_ok;
                }
                spinlock_release(&freelist_spins[o]); // otherwise release the spinlock 
                
        }
        *out = NULL;
        return gsd_out_of_resources;
}


static void split_page(gsdpage* pg) {
        if(!pg->order) { return;}
        int new_order = pg->order-1;
        spinlock_aquire(&freelist_spins[new_order]);
        index pgi = pg-mem_map;
        index buddyi = pgi ^ (1ul << new_order);
        gsdpage* buddy = &mem_map[buddyi];
        if(freelist[new_order]) {
                freelist[new_order]->prevfree = buddy;
        }
        buddy->nextfree = freelist[new_order];
        freelist[new_order] = buddy;
        buddy->order = new_order;
        pg->order = new_order;
        spinlock_release(&freelist_spins[new_order]);
}



status GsdAllocatePage(u64 attribute,int order,gsdpage** outpage) {
        status s = gsd_locate_page(attribute,order,outpage);
        status_assert_ok(s) { return s;}
        gsdpage* pg = *outpage;
        while(pg->order > order) {
                // split our page down 
                split_page(pg);
        }
        pg->rc = 1;
        return gsd_ok;
}

// Tries to combine a page 
// If succeeded the pg pointer variable from the caller is updated 
// it will either stay the same or become the buddy page 

#define non_combinable_flags gsdpg_s_present | gsdpg_a_reserved 
static bool combine_page(gsdpage** pg_ptr) {
        gsdpage* page = *pg_ptr;
        if(page->order >= SYS_MEMORY_ORDER) { return false;}
        if(page->flags & gsdpg_a_slack) { return false;} // slack pages cannot be merged 
        // otherwise aquire freelists of the current order and order+1
        spinlock_aquire(&freelist_spins[page->order]);
        index pgi = page-mem_map;
        index buddyi = pgi ^ (1ul << page->order);
        gsdpage* buddy = &mem_map[buddyi];
        if(buddy->flags & non_combinable_flags) {
                spinlock_release(&freelist_spins[page->order]); 
                return false;
        }
        if(buddyi < pgi) 
        {
                buddy->order = page->order+1;
                page->order = 0;
                page->flags &= ~(gsdpg_s_present);
                *pg_ptr = buddy;
                if(buddy->prevfree) {
                        buddy->prevfree->nextfree = buddy->nextfree;
                }
                else { // buddy is the head of the freelist
                        freelist[buddy->order-1] = buddy->nextfree;
                }
                if(buddy->nextfree) {
                        buddy->nextfree->prevfree = buddy->prevfree;
                }
                spinlock_release(&freelist_spins[buddy->order-1]);
                // now append our buddy to the 

                spinlock_aquire(&freelist_spins[buddy->order]);
                if(freelist[buddy->order]) {
                        freelist[buddy->order]->prevfree = buddy;
                }
                buddy->nextfree = freelist[buddy->order];
                freelist[buddy->order] = buddy;
                spinlock_release(&freelist_spins[buddy->order]);

   
        }
        else 
        {
                page->order++;
                buddy->flags &= ~(gsdpg_s_present);
                buddy->order = 0;
                if(page->prevfree) {
                        page->prevfree->nextfree = page->nextfree;
                } else {
                        freelist[page->order-1] = page->nextfree;
                }
                if(page->nextfree) {
                        page->nextfree->prevfree = page->prevfree;
                }
                spinlock_release(&freelist_spins[page->order-1]);
                spinlock_aquire(&freelist_spins[page->order]);
                if(freelist[page->order]) {
                        freelist[page->order]->prevfree = page;
                }
                page->nextfree = freelist[page->order];
                freelist[page->order] = page;
                spinlock_release(&freelist_spins[page->order]);
        }
        return true;
}



void GsdFreePage(gsdpage* page) {
        atomic_decrement(&page->rc);
        if(page->rc) { return;}
        while(combine_page(&page));
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

// mark all avalible slack pages 
static void _mark_slack_pages(index blkidx) {
        size order_pagec = (1ul << SYS_MEMORY_ORDER);
        u64 rsvd_flags = gsdpg_a_reserved | gsdpg_s_present;
        for(index i = 0; i < order_pagec; i++) {
                if(!vaddr_test((uintptr_t)&mem_map[blkidx+i])) { continue;}
                if(!(mem_map[blkidx+i].flags & gsdpg_a_valid)) { continue;}
                if(mem_map[blkidx+i].flags & rsvd_flags) { continue;}
                mem_map[blkidx+i].flags |= gsdpg_a_slack;
                mem_map[blkidx+i].nextfree = freelist[0];
                mem_map[blkidx+i].rc = 0;
                if(freelist[0]) {
                        freelist[0]->prevfree = &mem_map[blkidx+i];
                }
                freelist[0] = &mem_map[blkidx+i];


        }
}

status GsdMemoryInit(uintptr_t max_paddr) {
        // zero out freelist 
        for(index i = 0; i < SYS_MEMORY_ORDER+1; i++) {
                freelist[i] = (gsdpage*)NULL;
        }

        // iterate through every page and add blocks of max_order to the max order freelist 
        // and append all slack pages to the order 0 freelist 
        size pagec = max_paddr/SYS_PAGE_SIZE;
        size order_pagec = (1ul << SYS_MEMORY_ORDER);
        size order_block_count = pagec/order_pagec;
        index baseidx = 0;
        bool block_avalible = true;
        u64 rsvd_flags = gsdpg_a_reserved | gsdpg_s_present;
        for(index i = 0; i < order_block_count; i++)
        {
                for(index j = 0; j < order_pagec; j++)
                {
                
                        if(!vaddr_test((uintptr_t)&mem_map[baseidx+j])) { block_avalible = false; break;}
                        if(!(mem_map[baseidx+j].flags & gsdpg_a_valid)) { block_avalible = false; break;}
                        if(mem_map[baseidx+j].flags & rsvd_flags) { block_avalible = false; break;}
                }
                if(!block_avalible) {
                        _mark_slack_pages(baseidx);
                        block_avalible = true;
                }
                else {
                        mem_map[baseidx].order = SYS_MEMORY_ORDER;
                        mem_map[baseidx].nextfree = freelist[SYS_MEMORY_ORDER];
                        if(freelist[SYS_MEMORY_ORDER]) {
                                freelist[SYS_MEMORY_ORDER]->prevfree = &mem_map[baseidx];
                        }
                        freelist[SYS_MEMORY_ORDER] = &mem_map[baseidx];
                }
                baseidx += order_block_count;
        }
        

        for(index i = 0; i <= SYS_MEMORY_ORDER; i++) {
                freelist_spins[i] = 0;
        }

        return gsd_ok;
}

uintptr_t GsdGetPaddr(gsdpage* pg) {
        if(pg < mem_map) { return (uintptr_t)NULL;}
        uintptr_t out = ((uintptr_t)(pg-mem_map))*SYS_PAGE_SIZE;
        return out;
}

gsdpage* get_corrisponding_gsdpage(uintptr_t paddr) {
        index pgi = paddr/SYS_PAGE_SIZE;
        if(!vaddr_test((uintptr_t)&mem_map[pgi])) { return NULL;}
        return &mem_map[pgi];
}





status map_page(gsdpage* pg) {
        gsdcore* self = cself();
        gsd_procid selfid = self->current_thread->process->id;
        if(selfid && selfid != pg->owning_process) { 
                return gsd_invalid_param;
        }
        atomic_increment(&pg->rc);
        return mmu_map(pg);
}


status unmap_page(gsdpage* pg) {
        gsdcore* self = cself(); // Bounds check for the thread 
        gsd_procid selfid = self->current_thread->process->id;
        if(selfid && selfid != pg->owning_process) { 
                return gsd_invalid_param;
        }
        return mmu_unmap_page(pg);

}
