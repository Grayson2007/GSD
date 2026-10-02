#include <gsdstd/stdmem.h>
void bset(u8* dest,u8 val,size n) {
        while(n--) {
                *dest++ = val;
        }
}
void wset(u16* dest,u16 val,size n) {
        while(n--) {
                *dest++ = val;
        }
}
void dset(u32* dest,u32 val,size n) {
        while(n--) {
                *dest++ = val;
        }
}
void qset(u64* dest,u64 val,size n) {
        while(n--) {
                *dest++ = val;
        }
}
void zeromem(void* dest,size n) {
        while(n--) {
                *(char*)dest = 0;
        }

}

void memset(void* dest,int value,size n) {
        while(n--) {
                *(char*)dest++ = (char)value;
        }
}
void memcpy(void* dest,void* src,size n) {
        while(n--) {
                *(char*)dest++ = *(char*)src++;
        }
}
void memmove(void* dest,void* src,size n) {
        void* newdest = dest+n;
        void* newsrc = src+n;
        while(newdest > dest) {
                *(char*)newdest = *(char*)newsrc;
                newdest--;
                newsrc--;
        }
}
int memcmp(void* dest,void* src,size n) {
        while(n--) {
                if(*(uchar*)dest > *(uchar*)src) { return 1;}
                if(*(uchar*)dest < *(uchar*)src) { return -1;}
                src++;
                dest++;
        }
        return 0;
}




