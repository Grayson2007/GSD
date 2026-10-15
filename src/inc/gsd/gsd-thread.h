#ifndef gsd_thread_h
#define gsd_thread_h
#include "gsd-common.h"
#include "gsd-memory.h"
#include "gsd-stdcall.h"

typedef gsd_atomic mutex;

typedef struct s_gsd_semaphore {
        u32 lock;
        u32 rc;
} gsd_semaphore;

typedef struct s_gsdthread_common_header 
{
        struct rb_node global_rblink; // Global link for all system threads 
        struct rb_node local_rblink; // Local link for the threads specific process
        struct s_gsdproc_common_header* process;
        gsd_semaphore lock;
        gsd_threadid global_tid;
} gsdthread_hdr;


typedef struct s_gsdproc_common_header 
{
        struct rb_node rb_link; // red black link 
        gsd_procid id; // id for this process 
        size descriptor_size; // sizeof the descriptor 
        gsdpage* page_tables;
        struct rb_tree vmm; // rbt for the threads virtual memory manager
        struct rb_tree threads; // rbt for owned thread lookup 
        gsd_userid owner; // owner 
        gsd_groupid owning_group; // group user 
} gsdproc_common_header;


gsdthread_hdr* tself_structure();
gsd_threadid tself();
gsdproc_common_header* pself_structure();
gsd_procid pself();


bool mutex_try_aquire(mutex* lock,uint tries); 
void mutex_release(mutex* lock);

#endif