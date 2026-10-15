#include <gsd-common.h>
#include <impl/x86_64/asm_irq.h>
#include <impl/x86_64/dt.h>
#include <impl/x86_64/asm.h>
#include <rb_tree/rb_tree.h>
#include <gsd-stdcall.h>


static struct rb_tree* kerncall_tree;

void gsd_irq_c_handler(index irq_num) {
        
} 
void gsd_next_process() {

}

void gsd_handle_kerncall() {

}
extern void gsd_handle_termination() {

}

void gsd_delcall(kcallid function) {

}
