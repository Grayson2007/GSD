#include <impl/x86_64/x86_64-boot-table.h>
#include <impl/x86_64/dt.h>
#include <impl/x86_64/vmmap.h>
#include <impl/x86_64/cpux64.h>
#include <impl/x86_64/asm.h>
#include <impl/x86_64/asm_irq.h>
#include <gsd-memory.h>
// 
static struct __attribute__((packed)) gdtr {
        u16 lim;
        u64 base;
};

static struct __attribute__((packed)) idtr {
        u16 lim;
        u64 base;
};




NORETURN static void _early_kernel_panic() {
        KBT_POINTER->efirt.ResetSystem(efi_reset_cold,efi_aborted,0,NULL);
        for(;;);
}

NORETURN void clean_bootloader() {
        size loader_page_count = KBT_POINTER->loader.pagec;
        uintptr_t paddr = KBT_POINTER->loader.base;
        // Append Free
}


// Initalise Memory Manager first 
NORETURN void kinit() {
        GsdMemoryInit(KBT_POINTER->max_physical_address);
        // Allocate a fresh stack 
        status s; 
        gsdpage* newstack;
        s = gsd_allocate_page(gsdpg_c_wb,4,&newstack); // order 4; 16 pages or 64k stack  used for kernel operations 
        K_HEAP_VMM.root = (struct rb_node*)newstack;
        newstack->vaddr = KERNEL_OBJECT_BASE;
        status_assert_ok(s) {
                _early_kernel_panic();
        }
        newstack->owning_process = 0;
        newstack->rb_link.left = NULL;
        newstack->rb_link.right = NULL;
        newstack->rb_link.parent = 0;
        _asm_flush_stack((newstack->vaddr+((SYS_PAGE_SIZE*16))),newstack->vaddr,(uintptr_t)clean_bootloader);
        for(;;); // Shouldn't come here just stall for now if that happens 
}
void detect_cpu_cores() { // Detect system cores 

}

void core_wake() {

}