#include "gsd-memory.h"



static gsdpage* freelist[SYS_MEMORY_ORDER];
static  gsdpage* mem_map = (gsdpage*)(MEM_MAP_ADDRESS);
static size syspagec;
void GsdQueryPageInfo(gsdpage** freelistptr,int* max_order) {
        *freelistptr = &freelist;
        *max_order = SYS_MEMORY_ORDER;
}

static void Freelist_Push(int lvl,gsdpage* pg) {
        if(freelist[lvl]) {
                freelist[lvl]->prevfree = pg;
        }
        pg->prevfree = NULL;
        freelist[lvl] = pg;
}

static status Freelist_Pop(int lvl,gsdpage** outpg) {
        if(!freelist[lvl]) { return gsd_not_present;}
        *outpg = freelist[lvl];
        if(freelist[lvl]->nextfree) {
                freelist[lvl] = freelist[lvl]->nextfree;
                freelist[lvl]->prevfree = NULL;
        }
        else { freelist[lvl] = NULL;}
        return gsd_ok;
}

static void SplitPage(gsdpage* pg) {
        if(freelist[pg->order] == pg) {
                freelist[pg->order] = pg->nextfree;
        }
        if(pg->nextfree) {
                pg->nextfree->prevfree = pg->prevfree;
        }
        pg->order--;
        if(pg->prevfree) {
                pg->prevfree->nextfree = pg->nextfree;
        }
        pg->nextfree =NULL;
        pg->prevfree = NULL;
        index pgi = pg-mem_map;
        index buddyidx = (pgi) ^ (1 << pg->order);
        mem_map[buddyidx].order = pg->order;
        Freelist_Push(pg->order,&mem_map[buddyidx]);
}


static void CombinePage(gsdpage* pg,gsdpage** outpg) {
        index pgi = pg-mem_map;
        while(pg->order < SYS_MEMORY_ORDER)
        {
                index buddyidx = pgi ^ ((uptr)1 << pg->order);
                if(buddyidx >= syspagec) { break;} // 
                if(mem_map[buddyidx].attrib & gsdpg_active) { break;}

                // Update freelist 
                if(mem_map[buddyidx].prevfree) {
                        mem_map[buddyidx].prevfree->nextfree = mem_map[buddyidx].nextfree;
                }
                if(mem_map[buddyidx].nextfree) {
                        mem_map[buddyidx].nextfree->prevfree = mem_map[buddyidx].prevfree;
                }
                if(freelist[pg->order] == &mem_map[buddyidx]) {
                        freelist[pg->order] = mem_map[buddyidx].nextfree;
                }

                // If the buddy is before this page we will set our current to the buddy as we're getting absorbed 
                if(buddyidx < pgi) {
                        pg->order = 0;
                        pgi = buddyidx;
                        pg = &mem_map[pgi];
                }
                pg->order++;

        }
        *outpg = pg; // Page may change so update 
}

status GsdAllocatePage(int attrib,int order,gsdpage** outpage) {
        int curr_order = order;
        int capabilitys = attrib & gsdpg_capability_mask;
        while(!freelist[curr_order]) { // Find the first free page with order >= to the requested order 
                order++;
                if(order > SYS_MEMORY_ORDER) { return gsd_not_present;}
        }
        gsdpage* curr = freelist[curr_order];
                
        while(curr) // Locate a page in this order or Orders above that satisfy the required capabilities
        {
                if((curr->attrib & gsdpg_capability_mask) & capabilitys == capabilitys) {
                        break; // We found our Page! 
                }


                if(!curr->nextfree) 
                { 
                        curr_order++; 
                        if(curr_order > SYS_MEMORY_ORDER) { return gsd_not_present; /* no mem :(*/}
                        curr = freelist[curr_order];
                        continue;
                }

                curr = curr->nextfree;
        }

        while(curr_order > order) { // Split the page and mark as used 
                SplitPage(curr);
                curr_order--;
        }
        
        curr->attrib |= gsdpg_active;
        *outpage = curr;
        curr->owning_process = gsd_pself();
        return gsd_ok;
}


status GsdFreePage(gsdpage* page) {
        page->attrib &= ~(gsdpg_active);
        page->owning_process = 0;
        CombinePage(page,&page);
        if(freelist[page->order]) {
                freelist[page->order]->prevfree = page;
        }
        freelist[page->order] = page;
        return gsd_ok;
}





status GsdMemoryInit(size _Syspgc) 
{
        syspagec = _Syspgc;
        index max_order_block_pages = ((index)1 << SYS_MEMORY_ORDER);
        index max_order_blkc = syspagec >> SYS_MEMORY_ORDER;
        gsdpage* pg = &mem_map[0];
        bool block_free = true;
        for(index i = 0; i < max_order_blkc; i++)
        {
                block_free = true;
                // test 
                for(index p = 1; p < max_order_block_pages; p++)
                {
                        if(block_free) // scan the memory block for used pages
                        {
                                if(pg[p].attrib & gsdpg_active) {  // if a used page is found break 
                                        block_free = false;
                                        break;
                                }
                               
                        }         
                        
                }
                if(block_free) // No used page add this block to the freelist
                {
                        if(freelist[SYS_MEMORY_ORDER]) {
                                freelist[SYS_MEMORY_ORDER]->prevfree = pg;
                        }
                        pg->nextfree = freelist[SYS_MEMORY_ORDER];
                        freelist[SYS_MEMORY_ORDER] = pg;
                        pg += max_order_block_pages;
                        continue;
                }
                else 
                {       
                        // Otherwise we're going to add each free page in the memory block to the single page freelist 
                        for(index p = 0; p < max_order_block_pages; p++) {
                                if(pg[p].attrib & gsdpg_active) { 
                                        continue;
                                }
                                if(freelist[0]) {
                                        freelist[0]->prevfree = &pg[p];
                                }
                                pg[p].nextfree = freelist[0];
                                pg[p].prevfree = NULL;
                                freelist[0] = &pg[p];
                        }    
                        pg += max_order_block_pages;       
                }
        }
        return gsd_ok;
}


