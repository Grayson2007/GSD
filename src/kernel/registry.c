#include <gsd-common.h>
#include <gsd-memory.h>
#include <gsd-thread.h>



#define keys_per_page (SYS_PAGE_SIZE/sizeof(gsd_registry_key))-1




status GsdRegInsKey(gsd_registry_key* workingkey,char* keyname,enum rkey_types type,...) 
{
        // todo

}

status GsdRegDelKey(gsd_registry_key* key) {
        // todo


}

status GsdRegGetKey(gsd_registry_key* workingkey,char* path) {
        // todo
}

