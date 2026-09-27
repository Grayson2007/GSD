#ifndef stdmem_h
#define stdmem_h
#include "stdtype.h"
void bset(u8* dest,u8 val,size n);
void wset(u16* dest,u16 val,size n);
void dset(u32* dest,u32 val,size n);
void qset(u64* dest,u64 val,size n);
void zeromem(void* dest,size n);

void* gsdmalloc(size bytes);
void* gsdralloc(void* ptr,size newsize);
void* gsdcalloc(size num,size blksz);
void gsdfree(void* ptr);







#endif