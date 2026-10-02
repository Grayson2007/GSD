#include "gsd-common.h"
#include "gsd-stdcall.h"
#include "gsd-memory.h"
#define KEYS_PER_PAGE (SYS_PAGE_SIZE/sizeof(gsd_registry_key))-1// Free up one key slot for the tag data 
#define KEYSLAB_BITMAP_LENGTH ((KEYS_PER_PAGE)/8) + ((KEYS_PER_PAGE) % 8 ? 1 : 0)
struct s_registry_slab_tag{
        struct s_registry_slab_tag* next;
        struct s_registry_slab_tag* prev;
        volatile gsd_atomic count;
        volatile gsd_atomic trilock; // 
        u8 bitmap[KEYSLAB_BITMAP_LENGTH];
};

// doubly linked list for registry keys

struct s_registry_slab_tag* keyslabs;

// Registry Keys will use there own internel slab allocator getting memory from the 
// page manager
// buffers they store also will be allocated by the page manager 
static gsd_registry_key* SYS_ROOT_KEY;

static status NewKeySlab(struct s_registry_slab_tag* parent,struct s_gsd_registry_key** out) {
        status s;
        gsdpage* outpg;
        s = GsdAllocatePage(gsdpg_kernel | gsdperm_r | gsdpg_write,0,&outpg);
        status_assert_ok(s) { 
                return s;
        }
        uintptr_t pg_base;
        s = KVaddrMap(outpg,&pg_base);
        status_assert_ok(s) {
                GsdFreePage(outpg);
                return s;
        }
        zeromem((void*)pg_base,SYS_PAGE_SIZE);
        struct s_gsd_registry_key* keybase = (struct s_gsd_registry_key*)pg_base;
        struct s_registry_slab_tag* tagptr = (struct s_registry_slab_tag*)&keybase[KEYS_PER_PAGE];
        tagptr->count = KEYS_PER_PAGE;
        tagptr->prev = parent;
        tagptr->next = NULL;
        *out = tagptr;
        return gsd_ok;
}

status NextFreeKey(gsd_registry_key** outkey) {
        struct s_registry_slab_tag* tag = keyslabs;
        gsd_registry_key* keybase = (gsd_registry_key*)((uintptr_t)tag & ~((uintptr_t)SYS_PAGE_SIZE -1));
        status s;
        while(!tag->count) {
                if(!tag->next) {
                        s = NewKeySlab(tag,&keybase);
                        status_assert_ok(s) { return s;}
                        tag = (struct s_registry_slab_tag*)&keybase[KEYS_PER_PAGE];
                        break;
                }
                tag = tag->next;
                keybase = (gsd_registry_key*)((uintptr_t)tag & ~((uintptr_t)SYS_PAGE_SIZE -1));
        }

        for(index i = 0; i < KEYS_PER_PAGE; i++) {
                index byte = i >> 3;
                index bit = i % 8;
                if(!tag->bitmap[byte] & (1 << bit) ) {
                        *outkey = &keybase[i];
                        tag->bitmap[byte] |= (1 << bit);
                        return gsd_ok;
                }
        }
        // Shouldn't be likey to be here
        return gsd_not_present;
}

void GsdFreeKey(gsd_registry_key* key) {
        gsd_registry_key* base_ptr = ((uintptr_t)key & ~((uintptr_t)SYS_PAGE_SIZE-1));
        index keyidx = key-base_ptr;
        struct s_registry_slab_tag* tag = (struct s_registry_slab_tag*)&base_ptr[KEYS_PER_PAGE];
        tag->bitmap[keyidx >> 3] &= ~(1 << (keyidx % 8));
}       

status GsdInsertKey(gsd_registry_key* root,char* keyname,int keytype,...)
{
        if(!root) { root = SYS_ROOT_KEY;}
        if(!gsd_try_aquire(&root->mutex)) {
                return gsd_busy;
        }
        if(!gsd_pchk(&root->permissions,gsdperm_w)) {
                
                gsd_release(&root->mutex);
                return gsd_forbidden;
        }
        gsd_registry_key* new_key;
        status s;
        s = NextFreeKey(&new_key);
        status_assert_ok(s) {

                return s;
        }
        va_list argv;
        va_start(argv,keytype);
        switch(keytype)
        {
                case rkeyt_boolean:
                        bool _bool = va_arg(argv,bool);
                        new_key->value._boolean = _bool;
                        break;
                case rkeyt_string:
                        break;
                case rkeyt_strings:
                        break;
                case rkeyt_u32:
                        u32 _u32 = va_arg(argv,u32);
                        new_key->value._u32 = _u32;
                        break;
                case rkeyt_u64:
                        u64 _u64 = va_arg(argv,u32);
                        new_key->value._u64 = _u64;
                        break;
                case rkeyt_i32:
                        i32 _i32 = va_arg(argv,u32);
                        new_key->value._i32 = _i32;
                        break;
                case rkeyt_i64:
                        i64 _i64 = va_arg(argv,i64);
                        new_key->value._i64 = _i64;
                        break;
                case rkeyt_handle:
                        handle _handle = va_arg(argv,handle);
                        new_key->value._handle = _handle;
                        break;
                case rkeyt_blob:
                        u8* root_base = va_arg(argv,u8*);
                        size blobsz = va_arg(argv,size);
                        size pagec = (blobsz/SYS_PAGE_SIZE) + (blobsz % SYS_PAGE_SIZE ? 1 : 0);
                        void* blobkeybuff;
                        s = KPageAlloc(pagec,&blobkeybuff);
                        status_assert_ok(s) { 
                                GsdFreeKey(new_key);
                                gsd_release(&root->mutex);
                                return s;
                        }
                        memcpy(blobkeybuff,(void*)root_base,blobsz);
                        new_key->value.blob.base = blobkeybuff;
                        new_key->value.blob.sz = blobsz;
                        break;
        }
        

        return gsd_ok;
}

status GsdDeleteKey() 
{
        
}

status GsdFindKey()
{

}