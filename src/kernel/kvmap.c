#include <gsd-memory.h>
#include <gsd-thread.h>
struct rb_tree KVMM;
static spinlock KVMM_SPIN;


static int rb_ndcmp(const struct rb_node* a,const struct rb_node* b) {
        gsdpage* apg = (gsdpage*)a;
        gsdpage* bpg = (gsdpage*)b;

        uintptr_t a_base = apg->vaddr;
        uintptr_t a_lim = apg->vaddr+(1ul << (apg->order+12));
        uintptr_t b_base = bpg->vaddr;
        uintptr_t b_lim = bpg->vaddr+(1ul << (bpg->order+12));

        if(a_base >= b_base && a_lim <= b_lim) { return 0;}
        if(a_base < b_base) { return -1;}
        if(a_lim > b_lim) { return 1;}
        return 0;
}

static int rb_cmp(const struct rb_node* a, const void*b) {
        uintptr_t addr = *(uintptr_t*)b;
        gsdpage* pg = (gsdpage*)a;

        uintptr_t vbase = pg->vaddr;
        uintptr_t vlim = vbase+(1ul << (pg->order+12));
        if(addr >= vbase && addr <= vlim) { return 0;}
        if(addr < vbase) { return -1;}
        return 1;
}




status vmm_alloc_page(struct rb_tree* vmm,gsdpage* page,uintptr_t vmin,uintptr_t vmax) {
        
        page->vaddr = vmin; 
        size addr_size = vmax-vmin;
        if((1ul << (page->order+12)) > addr_size) { return gsd_out_of_resources;}
        uintptr_t max_possible_addr = vmax-(1ul << (page->order+12));
        gsdpage* nodepg = (gsdpage*)rb_tree_search(vmm,&page,&rb_cmp);
        while(nodepg) {
                gsdpage* next = (gsdpage*)rb_node_next((struct rb_node*)&nodepg->rb_link);
                if(next && (next->vaddr+(1ul << next->order)*SYS_PAGE_SIZE) >= vmax) 
                {
                        return gsd_out_of_resources;
                }

                if(!next || (next->vaddr-nodepg->vaddr) >= addr_size) { 
                        page->vaddr = nodepg->vaddr+(1ul << (nodepg->order))*SYS_PAGE_SIZE;
                        break;
                }
        
        }
        rb_tree_insert(vmm,(struct rb_node*)&page->rb_link,&rb_ndcmp);
        return gsd_ok;
}


status vmm_free_page(struct rb_tree* vmm,uintptr_t vaddr) {
        gsdpage* pg = (gsdpage*)rb_tree_search(vmm,&vaddr,&rb_cmp);
        if(!pg) { return gsd_not_present;}
        mmu_unmap(vaddr);
        rb_tree_remove(vmm,(struct rb_node*)pg);
        gsd_free_page(pg);
        return gsd_ok;
}


status vmm_get_page(struct rb_tree* vmm,uintptr_t vaddr,gsdpage** outpg) {
        gsdpage* pg = (gsdpage*)rb_tree_search(vmm,&vaddr,&rb_cmp);
        if(!pg) { return gsd_not_present;}
        *outpg = pg;
        return gsd_ok;

}



status kvmm_alloc(gsdpage* page) {
        spinlock_aquire(&KVMM_SPIN);
        status s = vmm_alloc_page(&KVMM,page,(uintptr_t)KERNEL_OBJECT_BASE,(uintptr_t)KERNEL_OBJECT_END);
        spinlock_release(&KVMM_SPIN);
        return s;
}


