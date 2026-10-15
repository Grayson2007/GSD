#ifndef pagingx64_h
#define pagingx64_h
#include <gsd/gsd-memory.h>
#define PML4_ADDRESS 0xFFFFFFFFFFFFF000
#define PGT_ADDRESS(L3,L2,L1) (u64*)((511 << 39) | (L3 << 30) | (L2 << 21 ) | (L1 << 12))

enum x64_gsdpage_capabilities {
        gsdpg_xp = (1 << 13),
        gsdpg_wp = (1 << 14),
        gsdpg_rp = (1 << 15),
        gsdpg_uc = (1 << 16),
        gsdpg_wt = (1 << 17),
        gsdpg_wb = (1 << 18),
        gsdpg_wc = (1 << 19)
};


enum PageTableFlags {
        pgt_present = (1 << 0),
};




#endif