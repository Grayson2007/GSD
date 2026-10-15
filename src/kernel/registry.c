#include <gsd-common.h>
#include <gsd-memory.h>
#include <gsd-thread.h>

#define keys_per_page (SYS_PAGE_SIZE/sizeof(gsd_registry_key))-1
struct s_keyslab_hdr {
        struct s_keyslab* next;
        size num_free;
        gsd_atomic mutex;
};
struct s_keyslab {
        struct s_keyslab_hdr hdr;
        u8 pad[sizeof(gsd_registry_key)-(sizeof(struct s_keyslab_hdr))];
        gsd_registry_key keys[(keys_per_page)];
};




struct s_keyslab* keyslab_root;
gsd_registry_key* key_r; // root key (path would be @r/)
gsd_registry_key* reg_mounts; 



void alloc_key() {
        struct s_keyslab* curr_slab = keyslab_root;
        
}

