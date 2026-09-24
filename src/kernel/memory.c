#include "gsd-memory.h"
#include "impl/x86_64/vmmap.h"
static gsdpage* freelist[GSD_MEM_ORDER];
static gsdpage* mem_map = (gsdpage*)(MEM_MAP_ADDRESS);

void GsdQueryPageInfo(gsdpage** freelistptr,int* max_order) {
        *freelistptr = &freelist;
        *max_order = GSD_MEM_ORDER;
}




status GsdAllocatePage(int attrib,int order,gsdpage** outpage) {
        int curr_order = order;
        int capabilitys = attrib & gsdpg_capability_mask;
        if(curr_order >= GSD_MEM_ORDER) {return alloc_pages_no_mem;}
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
                buddypg->prevfree = 0;
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
        while(page->order < GSD_MEM_ORDER) {
                index buddyidx = (i ^ ((index)1 << page->order));
                gsdpage* buddy = &mem_map[buddyidx];
                if(buddy->attrib & gsdpg_active) { 
                        break;
                }       
                if(buddyidx < i) {
                        page->order = 0;
                        page = buddy;
                        page->order++;
                        continue;
                }
                else 
                {
                        buddy->order = 0;
                        page->order++;
                        continue;
                }
        }
        if(freelist[page->order]) {
                freelist[page->order]->prevfree = page;
        }
        page->prevfree = NULL;
        freelist[page->order] = page;
        return gsd_ok;
}