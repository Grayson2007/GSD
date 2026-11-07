
#include <gsd-memory.h>


// GSD offsers slabs of powers of 2 from 2^4-2^10 (16 bytes-)


status gsd_slab_init(int order,int slabpow,gsdpage** outslab) {
        gsdpage* slab;
        status s = gsd_allocate_page(gsdpg_slab_allocator,order,&slab); 
        status_assert_ok(s) { return s;}
        size slabsz = (1ul << slabpow);
        size blocksz = (1ul << slab->order);
        slab->slabinfo.freelist_start = 0;
        slab->slabinfo.slab_size = slabsz;
        slab->slabinfo.slabs_free = blocksz/slabsz;
        s = vmm_alloc_page(&K_HEAP_VMM,slab,KERNEL_OBJECT_BASE,KERNEL_OBJECT_END);
        status_assert_ok(s) {
                gsd_free_page(slab);
                return s;
        }
        return mmu_map(slab);
}

void* slab_alloc(gsdpage* slab) {
        if(!slab->slabinfo.slabs_free) {
                return NULL;
        }
        u8* slab_base = (u8*)slab->vaddr;
        index* slab_ptr = (index*)slab_base+(slab->slabinfo.slab_size*slab->slabinfo.freelist_start);
        slab->slabinfo.freelist_start = *slab_ptr;
        memset((void*)slab_ptr,0,slab->slabinfo.slab_size);
        return (void*)slab_ptr;
}

void slab_free(gsdpage* slab,index slabidx) {
        slab->slabinfo.slabs_free++;
        u8* slab_base = (u8*)slab->vaddr;
        index* newptr = (index*)slab_base+(slabidx*slab->slabinfo.slab_size);
        *newptr = slab->slabinfo.freelist_start;
        slab->slabinfo.freelist_start = slabidx;
}



void SlabVMM_init() {
        
}