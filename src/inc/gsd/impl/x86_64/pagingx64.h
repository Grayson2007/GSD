#ifndef pagingx64_h
#define pagingx64_h

#define PML4_ADDRESS 0xFFFFFFFFFFFFF000
#define PGT_ADDRESS(L3,L2,L1) (511 << 39) | (L3 << 30) | (L2 << 21 ) | (L1 << 12)



#endif