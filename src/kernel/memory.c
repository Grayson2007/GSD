#include "gsd-memory.h"
static gsdpage* freelist[GSD_MEM_ORDER];
static gsdpage* mem_map;



void InitPages(void* mem_map_base) {
                
}

void GsdQueryPageInfo(gsdpage** freelistptr,int* max_order) {
        *freelistptr = &freelist;
        *max_order = GSD_MEM_ORDER;
}


void GsdMarkPage(int attrib,int order,uintptr_t ptr); 



status GsdAllocatePage(int attrib,int order,gsdpage** outpage) {
        int curr_order = order;
        int capabilitys = attrib & gsdpg_capability_mask;
        while(!freelist[curr_order]) {
                curr_order++;
                if(curr_order >= GSD_MEM_ORDER) { return alloc_pages_no_mem;}
        }

        gsdpage* currpg = freelist[curr_order];
        // iterate through the freelists until we find a page of greater or equal order that satisfys our needed capabilities 
        while(currpg->attrib & gsdpg_capability_mask != capabilitys) {
                if(currpg->nextfree) 
                { 
                        currpg = currpg->nextfree; 
                        continue;
                } 
                curr_order++;
                if(curr_order >= GSD_MEM_ORDER) { return alloc_pages_no_mem;} // Max level then no memory is avalible 
        }
        index selfidx = currpg-mem_map;
        
        while(currpg->order > order) 
        {
                // Split the page until we reach the target ordfer 
                currpg->order--;
                index buddyidx = (selfidx ^ ((index)1 << currpg->order));
                gsdpage* buddypg = &mem_map[buddyidx];
                if(freelist[currpg->order]) {
                        freelist[currpg->order]->prevfree = buddypg;
                }
                buddypg->prevfree = NULL;
                buddypg->nextfree = freelist[currpg->order];
                freelist[currpg->order] = buddypg;
                buddypg->order = currpg->order;
        } 

        currpg->attrib |= gsdpg_active | attrib;
        *outpage = currpg;
        return gsd_ok;
}


status GsdFreePage(gsdpage* page) {
        if(!page->attrib & gsdpg_active) { return gsd_ok;} // page is free do nothing 
        index i = page-mem_map;
        while(1) {

        }
}