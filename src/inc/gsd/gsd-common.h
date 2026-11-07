#ifndef gsd_common_h
#define gsd_common_h
#include "gsd-config.h"
#include <gsdstd/stdtype.h>
#include <gsdstd/stdmem.h>
#include <gsdstd/arrlist.h>
#include <rb_tree.h>
#include <stdalign.h>
#include <float.h>
enum gsd_syscall_numbers {
        sysc_in, // resourceid resource,size max_bytes,void* buff 
        sysc_out,
        sysc_yeild,
        sysc_exit,
        sysc_open_resource, 
        sysc_close_resource,
        sysc_alloc_page,
        sysc_vmm_alloc,
        sysc_vmm_free,
        sysc_free_page,
        sysc_mmap,
        sysc_rqkey,
        sysc_rqkeys,
        sysc_rinskey,
        sysc_rdelkey,
        sysc_inscall,
        sysc_qcall, // returns gsd_ok if present, or gsd_not_present ; 
        sysc_delcall,
};





typedef int gsd_atomic;
typedef gsd_atomic mutex;
typedef uint gsd_uatomic;
typedef long gsd_latomic;
typedef ulong gsd_ulatomic;
typedef intptr_t gsd_patomic;
typedef uintptr_t gsd_upatomic;


typedef u32 gsd_userid;
typedef u32 gsd_groupid;
typedef u32 gsd_coreid;
typedef u32 gsd_procid;
typedef u32 gsd_threadid;
typedef u32 gsd_fileid; // A Resource ID is a number for anything that the 
// process is using 
typedef int64_t timestamp;
typedef uint64_t utimestamp;


timestamp now();
utimestamp now_u();




int gsd_try_aquire(gsd_atomic* atomic);
void gsd_release(gsd_atomic* atomic);


void _gsdsc_yeild();
void _gsdsc_exit(status s);
status _gsdsc_open(char* path,gsd_fileid* outid);




typedef struct s_gsd_permission_target {
        u32 crc32;
} permtarget;

enum gsd_reserved_permissions {
        gsdperm_r = 1,
        gsdperm_w = (1 << 1),
        gsdperm_x = (1 << 2), 
        gsdperm_query = (1 << 3),
        gsdperm_delete = (1 << 4)
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
        rkeyt_directory,
        rkeyt_string,
        rkeyt_buffer
};




enum rkey_attributes {
        rkey_attrib_persistant = (1 << 0)
};


typedef struct s_gsd_reg_symlink_node {
        struct s_gsd_registry_key* link;
        struct s_gsd_reg_symlink_node* next;
} gsd_reg_symlink_node;
#define GSD_REG_KEY_SIG 'RKEY'
#define REGKEY_MEM_BASE KERNEL_OBJECT_BASE
#define REGKEY_MEM_LIM KERNEL_OBJECT_END



typedef struct ALIGNED(sizeof(uintptr_t)) s_gsd_registry_key {
        u32 signature; // 'RKEY' 

        struct s_gsd_registry_key* par_next_child;
        struct s_gsd_registry_key* parent;

        gsd_atomic lock;

}  gsd_registry_key;

status rcreatekey(char* keyname,int keytype,permgate perms,gsd_registry_key** outptr,...);
status rinskey(gsd_registry_key* workingkey,gsd_registry_key* new);
status rnextkey(gsd_registry_key* curr_key,gsd_registry_key** outptr);  // next key in the directory sequence 
status rprevkey(gsd_registry_key* curr_key,gsd_registry_key** outptr);
status rdelkey(gsd_registry_key* key);


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

typedef struct {
        struct rb_node grb_link; // red black link to global tree
        struct rb_node lrb_link; // red black link to the processes local resource tree
        gsd_fileid lid;
        gsd_fileid gid;
        handle internal_handle;
        gsd_file_intrf* intrf;
} gsd_file;







u32 docrc32(void* base,size n);
bool gsd_pchk(permgate* gate,u16 perms); 




#endif