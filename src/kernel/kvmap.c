#include <gsd-memory.h>
#include <gsd-thread.h>



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




status vmm_alloc_page(gsdpage* page,uintptr_t vmin,uintptr_t vmax) {
        
        page->vaddr = vmin; 
        size addr_size = vmax-vmin;
        if((1ul << (page->order+12)) > addr_size) { return gsd_out_of_resources;}
        uintptr_t max_possible_addr = vmax-(1ul << (page->order+12));
        gsdproc_common_header* proc = pself_structure();
        gsdpage* nodepg = (gsdpage*)rb_tree_search(&proc->vmm,&page,&rb_cmp);
        while(nodepg) {
                uintptr_t node_max = nodepg->vaddr+(1ul << (nodepg->order+12));
                if(max_possible_addr  <= node_max) { return gsd_out_of_resources;}
                page->vaddr = node_max;
                nodepg = (gsdpage*)rb_tree_search(&proc->vmm,&page->vaddr,&rb_cmp);
        }
        rb_tree_insert(&proc->vmm,(struct rb_node*)page,&rb_ndcmp);
        return gsd_ok;
}


status vmm_free_page(uintptr_t vaddr) {
        gsdproc_common_header* proc = pself_structure();
        gsdpage* pg = (gsdpage*)rb_tree_search(&proc->vmm,&vaddr,&rb_cmp);
        if(!pg) { return gsd_not_present;}
        rb_tree_remove(&proc->vmm,(struct rb_node*)pg);
        GsdFreePage(pg);
        return gsd_ok;
}

