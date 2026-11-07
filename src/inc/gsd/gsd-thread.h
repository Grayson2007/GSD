#ifndef gsd_thread_h
#define gsd_thread_h
#include "gsd-common.h"
#include "gsd-memory.h"
#include "gsd-stdcall.h"

typedef struct {
        gsd_atomic arc;
        int state;
} trilock;




#define trilock_free 0
#define trilock_read 1
#define trilock_modify 2 // Only 1 thread can access this 
bool trilock_aquire(trilock* lock,int access); // Tries 
void trilock_release(trilock* lock);
void trilock_spin_aquire(trilock* lock,int access);



typedef gsd_atomic spinlock;
void spinlock_aquire(spinlock* lock);
void spinlock_release(spinlock* lock);


void atomic_increment(int* x);
void atomic_decrement(int* x);

typedef struct s_gsd_semaphore {
        gsd_atomic lock;
        u32 rc;
} gsd_semaphore;
enum gsd_threadstates {
        threadstate_disabled,
        threadstate_waking,
        threadstate_ready,
        threadstate_running,
        threadstate_sleeping
};


typedef struct s_gsdthread_common_header 
{
        struct rb_node global_rblink; // Global link for all system threads 
        struct rb_node local_rblink; // Local link for the threads specific processs
        struct rb_node priority_rblink;
        struct s_gsdproc_common_header* process;
        int priority_level;
        gsd_threadid global_tid;
        gsd_threadid local_tid;
        timestamp sleep_end; // timestamp for the sleeping of the thread
        timestamp thread_creation_stamp; // creation time for the thread
        timestamp burst_start; // start time for it's current burst 
        timestamp burst_end; // end time 
        
} gsdthread_hdr;
typedef struct s_gsdproc_common_header 
{
        struct rb_node link;
        gsd_procid id; // id for this process 
        size descriptor_size; // sizeof the descriptor 
        gsdpage* page_tables;
        struct rb_tree vmm; // rbt for the threads virtual memory manager
        struct rb_tree threads; // rbt for owned thread lookup 
        gsd_userid owner; // owner 
        gsd_groupid owning_group; // group user 
} gsdproc_common_header;

enum gsd_core_states {
        corestate_unusable,
        corestate_disabled,
        corestate_initalising,
        corstate_ready,
        corestate_running_thread,
        corestate_venv
};
typedef struct s_gsdcore_common_hdr {
        gsd_coreid id;
        int core_state;
        int current_priority;
        struct s_gsdcore_common_header* next_core; // only used during initalization and interation
        union {
                struct s_gsdthread_common_header* current_thread;
                handle venvctx;
        };
} gsdcore;

gsdthread_hdr* tself();
gsdproc_common_header* pself();
gsdcore* cself();


bool mutex_try_aquire(mutex* lock,uint tries); 
void mutex_release(mutex* lock);




#endif