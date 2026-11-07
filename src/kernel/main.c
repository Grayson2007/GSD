#include <gsd-common.h>
#include <gsd-thread.h>


struct {
        struct rb_tree root;
        gsd_atomic lock;
} SYS_PROCESS;
struct {
        struct rb_tree root;
        gsd_atomic lock;
} SYS_THREADS;

struct {
        gsdcore* first;
        size count;
} SYS_CPUS;


struct s_gsd_registry_key* ROOT_KEY;
int gsdproc_rbcmp(const struct rb_node *node, const void *b) {
        gsdproc_common_header* hdr = (gsdproc_common_header*)node;
        gsd_procid id = *(gsd_procid*)b;
        if(hdr->id > id) { return 1;}
        if(hdr->id < id) { return -1;}
        return 0;
}



gsdproc_common_header* gsd_get_process(gsd_procid id) {
        spinlock_aquire(&SYS_PROCESS.lock);
        struct rb_node* proc = rb_tree_search(&SYS_PROCESS.root,&id,gsdproc_rbcmp);
        spinlock_release(&SYS_PROCESS.lock);
        return (gsdproc_common_header*)proc;
}

gsdthread_hdr* gsd_get_thread_global(gsd_threadid gid) 
{
        
}

gsdthread_hdr* gsd_get_thread_local(gsdproc_common_header* proc,gsd_threadid lid)
{

}


// stdkerncalls 


status inscall();

status tfork();
