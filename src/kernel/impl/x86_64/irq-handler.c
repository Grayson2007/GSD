#include <gsd-common.h>
#include <impl/x86_64/asm_irq.h>
#include <impl/x86_64/dt.h>
#include <impl/x86_64/asm.h>
#include <gsd-stdcall.h>
static rbt* kerncall_tree;

void gsd_irq_c_handler(index irq_num) {
        
} 
void gsd_next_process() {

}
extern gsd_handle_termination() {

}

void gsd_delcall(kcallid function) {

}

status gsd_handle_kerncall(kcallid function,int argc,void** argarr)
{
        rbtnode* kerncall_node;
        status s = rbt_find(kerncall_tree,&function,&kerncall_node);
        status_assert_ok(s) {
                return s;
        }
        gsd_registry_key* kc_key = (gsd_registry_key*)kerncall_node->ref;
        if(!gsd_pchk(kc_key,gsdperm_x) ) { return gsd_forbidden;}
        kc_function func = *((kc_function*)&kc_key->data);
        s = func(argc,argarr);
        _wbinvd();
        return s;
}




