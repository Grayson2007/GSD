#ifndef arrlist_h
#define arrlist_h
#include "stdtype.h"
typedef struct {
        handle* arr;
        size count;
        size capacity;
} arrlist;


status arrlist_push(arrlist* self,handle item);
status arrlist_insert(arrlist* self,handle item,index idx);
status arrlist_pop(arrlist* self,handle* out);
status arrlist_remove_at(arrlist* self,index idx);
status arrlist_find(arrlist* self,handle item,index* out);
status arrlist_remove_item(arrlist* self,handle item);


#endif