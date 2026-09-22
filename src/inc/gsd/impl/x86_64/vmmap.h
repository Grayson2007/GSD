#ifndef gsd_vmmap_h
#define gsd_vmmap_h
#define HIGHMEM_MASK 0xFFFF000000000000 
#define KERNEL_SPACE_BASE 0xFFFF000000000000 | (500 << 39)
#define MEM_MAP_ADDRESS KERNEL_SPACE_BASE
#define BOOT_TABLE_ADDRESS HIGHMEM_MASK | (510 << 39) | (511 << 39) | (511 << 39) | (511 << 39)


#endif