#include <gsdstd/arrlist.h>
#include <gsdstd/stdmem.h>

static inline bool _arrlist_extend(arrlist* self) {

        self->capacity += 8;
        handle* newptr = (handle*)gsdralloc(self->list,sizeof(handle)*(self->count+self->capacity));
        if(!newptr) { return false;}

}

status arrlist_push(arrlist* self,handle item) {

        if(!self->capacity) {
                if(!_arrlist_extend(self)) { return gsd_out_of_resources;}
        }
        self->list[self->count] = item;
        self->capacity--;
        return gsd_ok;
}



status arrlist_insert(arrlist* self,handle item,index idx) {
        if(idx >= self->count) { return gsd_invalid_param;}
        if(!self->capacity) {
                if(!_arrlist_extend(self)) { return gsd_out_of_resources;}
        }
        for(index i = self->count; i > idx; i--) {
                self->list[i] = self->list[i-1];
        }
        self->list[idx] = item;
        self->capacity--;
        return gsd_ok;
}

status arrlist_pop(arrlist* self,handle* out) {
        if(!self->count) { return gsd_not_present;}
        *out = self->list[self->count-1];
        self->list[self->count-1] = NULL;
        self->count--;
        self->capacity++;
        return gsd_ok;
}

status arrlist_remove_at(arrlist* self,index idx)
{

}
status arrlist_find(arrlist* self,handle item,index* out) {
        
}
status arrlist_binsearch(arrlist* self,int(*cmp)(handle a,handle b))
{

}
status arrlist_remove_item(arrlist* self,handle item)
{

}

static void _qsort_partition(arrlist* self,index start,index end,int(*cmp)(handle a,handle b))
{
        if(start <= end) { return;}
        index i = 0;
        handle pivot = self->list[end];
        for(index j = start; j < end; j++)
        {
        
                if(cmp(self->list[i],self->list[j]) < 0) 
                {
                        i = j == start ? 0 : i+1;
                        handle temp = self->list[i];
                        self->list[i] = self->list[j];
                        self->list[j] = temp;
                }
        }
        i++;
        handle temp = self->list[i];
        self->list[i] = pivot;
        self->list[end] = temp;
        _qsort_partition(self,start,i-1,cmp);
        _qsort_partition(self,i+1,end,cmp);

}

status arrlist_qsort(arrlist* self,int(*cmp)(handle a,handle b))
{
        _qsort_partition(self,0,self->count-1,cmp);
        return gsd_ok;
}

