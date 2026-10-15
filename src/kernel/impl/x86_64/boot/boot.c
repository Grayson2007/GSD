#include <impl/x86_64/x86_64-boot-table.h>
#include <impl/x86_64/dt.h>
#include <impl/x86_64/vmmap.h>
#include <impl/x86_64/cpux64.h>
#include <impl/x86_64/asm.h>
#include <impl/x86_64/asm_irq.h>
#include <gsd-memory.h>



NORETURN static void _early_kernel_panic() {
        KBT_POINTER->efirt.ResetSystem(efi_reset_cold,efi_aborted,0,NULL);
        for(;;);
}


// Initalise Memory Manager first 
void kinit() {

        // Allocate a fresh stack with the function we 'return'
        

        for(;;); // Just halt for now
}
void detect_cpu_cores() { // Detect system cores 

}
void setup_new_stack() { 

}
void setup_memory_map() {

}
void registry_init() {
        
}