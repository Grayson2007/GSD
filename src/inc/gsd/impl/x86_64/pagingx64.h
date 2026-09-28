#ifndef pagingx64_h
#define pagingx64_h
#include <gsd/gsd-memory.h>
#define PML4_ADDRESS 0xFFFFFFFFFFFFF000
#define PGT_ADDRESS(L3,L2,L1) (511 << 39) | (L3 << 30) | (L2 << 21 ) | (L1 << 12)

enum x64_gsdpage_capabilities {
        gsdpg_xp = (1 << 10),
        gsdpg_wp = (1 << 11),
        gsdpg_rp = (1 << 12),
        gsdpg_uc = (1 << 13),
        gsdpg_wt = (1 << 14),
        gsdpg_wb = (1 << 15),
        gsdpg_wc = (1 << 16),
};


enum PageTableFlags {
        pgt_present = (1 << 0),
};




#endif