#ifndef gsd_common_h
#define gsd_common_h
#include "gsd-config.h"
#include <gsdstd/stdtype.h>
#include <gsdstd/stdmem.h>
#include <gsdstd/rbtree.h>
#include <gsdstd/arrlist.h>
#include <stdalign.h>
#include <float.h>
typedef int gsd_atomic;
typedef long gsd_latomic;
typedef u32 gsd_procid;
typedef u32 gsd_threadid;
typedef u32 gsd_userid;
typedef u32 gsd_groupid;
typedef u32 gsd_coreid;
typedef u32 gsd_resourceid; // A Resource ID is a number for anything that the 
// process is using 

int gsd_try_aquire(gsd_atomic* atomic);
void gsd_release(gsd_atomic* atomic);
// I don't trust the Compiler's '_Atomic' 
// keyword so I use helpers from assembly 
void atomic_increment(gsd_atomic* value);
void atomic_decrement(gsd_atomic* value);

void latomic_increment(gsd_latomic* value);
void latomic_decrement(gsd_latomic* value);


typedef struct s_gsd_permission_target {
        u32 crc32;
} permtarget;

enum gsd_reserved_permissions {
        gsdperm_r = 1,
        gsdperm_w = (1 << 1),
        gsdperm_x = (1 << 2)
};

typedef struct permgate {
        gsd_userid usr;
        gsd_groupid grp;
        u16 flags_usr;
        u16 flags_grp;
        u16 flags_other;
} permgate;


enum rkey_types {
        rkeyt_none,
        rkeyt_i32,
        rkeyt_u32,
        rkeyt_i64,
        rkeyt_u64,
        rkeyt_string,
        rkeyt_strings,
        rkeyt_handle,
        rkeyt_boolean,
        rkeyt_blob,
};



typedef struct s_gsd_reg_symlink_node {
        struct s_gsd_registry_key* link;
        struct s_gsd_reg_symlink_node* next;
} gsd_reg_symlink_node;

typedef struct ALIGNED(sizeof(uintptr_t)) s_gsd_registry_key {
        char keyname[32]; // 32 Character Keynames 
        struct s_gsd_registry_key* parent;
        struct s_gsd_registry_key* par_next_child;
        struct s_gsd_registry_key* this_first_child;
        gsd_reg_symlink_node* active_symlinks; 
        gsd_atomic mutex;
        int keytype;
        permgate permissions;
        union {
                i32 _i32;
                i64 _i64;
                u32 _u32;
                u64 _u64;
                char* _str;
                handle _handle;
                struct {
                        char* _str;
                        size count;
                } _strs;
                struct {
                        handle* _handle;
                        size count;
                } _handles;
                struct {
                        void* base;
                        size sz;
                } blob;
                struct s_gsd_registry_key* link;
                bool _boolean;
        } ALIGNED(sizeof(uintptr_t)) value;
}  gsd_registry_key;


typedef struct {

        u32 crc32; // checksum for this structure (For security)
        gsd_procid id; // id for this process 
        size descriptor_size; // sizeof the descriptor 
        rbt thread_tree; // rbt structure for thread tree 
        rbt children_tree; // child processes tree
        rbt memlayout_tree; // virtual memory layout 
        gsd_userid owner; // owner 
        gsd_groupid owning_group; // group user 
} gsdproc_common_header;



typedef struct {
        status(*close)(handle self);
        status(*read)(handle self,void* destbuff,size num_blocks,size block_size);
        status(*write)(handle self,void* srcbuff,size num_blocks,size block_size);
        status(*get_current_offset)(handle self,offset* out);
        status(*seek_to)(handle self,offset new_offset);
        status(*seek_forward)(handle self,offset count);
        status(*seek_backward)(handle self,offset count);
        status(*rewind)(handle self);
        status(*fast_forward)(handle self);
        status(*get_size)(handle self);
} gsd_file_intrf;



typedef struct s_gsd_open_file {
        gsd_resourceid id;
        char* filename;
        gsd_file_intrf* intf;
        handle self;
} gsd_open_file;


u32 docrc32(void* base,size n);


bool gsd_pchk(permgate* gate,u16 perms); 
handle gsd_get_cpuself_node();
#endif