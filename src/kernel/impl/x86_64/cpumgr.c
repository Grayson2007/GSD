#include <gsd-common.h>
#include <impl/x86_64/cpux64.h>
#include <impl/x86_64/asm.h>
cpux64* CORE_LIST;
size NUM_CORES;


status x64_init_cores() {
        index n = 0;
        cpux64* curr = CORE_LIST;
        while(n < NUM_CORES) {
                if(curr->state == corestate_unusable)  {
                        curr = curr->hdr.next_core;
                        continue;
                }
                curr->IDTR.sz = 4095;
                gsdpage* pg;
                status s = gsd_allocate_page(gsdpg_c_gpmem,0,&pg,&pg);
                status_assert_ok(s) { return s;}
                s = vmm_alloc_page(&K_HEAP_VMM,pg,KERNEL_OBJECT_BASE,KERNEL_OBJECT_END);
                status_assert_ok(s) { return s;}
                curr->IDTR.base = (u64)pg->vaddr;
                curr = curr->hdr.next_core;
                continue;
        }
        return gsd_ok;
}