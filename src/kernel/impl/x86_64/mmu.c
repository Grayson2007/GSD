#include <gsd-common.h>
#include <gsd-common.h>
#include <impl/x86_64/vmmap.h>
#include <impl/x86_64/pagingx64.h>
#include <impl/x86_64/asm.h>
#include <gsd-thread.h>
static bool pat_supported = false;

#define pt_attrib gsdpg_rw | gsdpg_s_page_table | gsdpg_valid_present | gsdpg_s_kernelpg
enum x64_pat_options {
        pat_uncacheable,
        pat_write_combine,
        pat_write_through = 4,
        pat_write_protect = 5,
        pat_writeback = 6,
        pat_unchached = 7
};

enum x64_pat_indexes : u64 {
        patidx_unchacheable,
        patidx_unchached,
        patidx_wt,
        patidx_wb,
        patidx_wp,
        patidx_wc

};
#define PAT_MSR 0x277
#define GSD_PAT 0x0007040605010000

enum x64_page_attributes {
        pga_present = (1 << 0),
        pga_writable = (1 << 1),
        pga_user_allow = (1 << 2),
        pga_patbit0 = (1 << 3),
        pga_patbit1 = (1 << 4),
        pga_accessed = (1 << 5),
        pga_dirty = (1 << 6),
        pga_ps = (1 << 7),
        pga_l1patbit2 = pga_ps,
        pga_global = (1 << 8),
        pga_valid = (1 << 9),
        pga_patbit2 = (1 << 12),
        pga_xd = (1ul << 63)
};

bool vaddress_isvalid(uintptr_t vaddr) {
        index pml3i = vaddr >> 39 & 0x1FF;
        index pml2i = vaddr >> 30 & 0x1FF;
        index pml1i = vaddr >> 21 & 0x1FF;
        index pgi = vaddr >> 12 & 0x1FF;

        
        u64* pml4 = (u64*)(PML4_ADDRESS);
        if(!pml4[pml3i] & 1) { return false;}
        u64* pml3 = PGT_ADDRESS(511,511,pml3i);
        if(!pml3[pml2i] & 1) { return false;}
        if(pml3[pml2i] & (1 << 7)) { return true;}
        u64* pml2 = PGT_ADDRESS(511,pml3i,pml2i);
        if(!pml2[pml1i] & 1) { return false;}
        if(pml2[pml1i] & (1 << 7)) { return true;}
        u64* pml1 = PGT_ADDRESS(pml3i,pml2i,pml1i);
        if(!pml1[pgi] & 1) { return false;}
        return true;
}

void free_page_table(uintptr_t paddr) {

}

status new_page_table(int lvl,uintptr_t* out) {
        gsdproc_common_header* proc = pself_structure();
        status s;
        gsdpage* pg;
        s = GsdAllocatePage(pt_attrib,0,&pg);
        status_assert_ok(s) {
                return s;
        }
        pg->ptinfo.lvl = lvl;
        if(proc->page_tables) {
                proc->page_tables->ptinfo.prev_pt = pg;
        }
        pg->ptinfo.next_pt = proc->page_tables;
        pg->prevfree->prevfree = NULL;
        proc->page_tables = pg;
        *out = GsdGetPaddr(pg);
        return gsd_ok;
}


static u64 _get_caching_idx(gsdpage* pg) {
        if(pg->flags & gsdpg_a_cd) { return patidx_unchacheable;}
        if(pg->flags & gsdpg_s_wb) { return patidx_wb;}
        if(pg->flags & gsdpg_s_wc) { return patidx_wc;}
        if(pg->flags & gsdpg_s_wt) { return patidx_wt;}
        return patidx_unchached; 
}

static u64 get_page_attributes(gsdpage* page) {
        u64 cache_idx = _get_caching_idx(page);
        u64 pg_attrib = 0;
        pg_attrib |= cache_idx & 1 ? pga_patbit0 : 0;
        pg_attrib |= cache_idx & 2 ? pga_patbit1 : 0;
        if(page->order < 9) {
                pg_attrib |= cache_idx & 4 ? pga_patbit2 : 0;
        }
        else {
                pg_attrib |= cache_idx & 4 ? pga_l1patbit2 : 0;
        }
        pg_attrib |= page->flags & gsdpg_s_r ? pga_present : 0;
        pg_attrib |= page->flags & gsdpg_s_present ? pga_valid : 0;
        pg_attrib |= !(page->flags & gsdpg_s_kernelpg) ? pga_user_allow : 0;
        pg_attrib |= page->flags & gsdpg_s_w ? pga_writable : 0;
        pg_attrib |= !(page->flags & gsdpg_s_x) ? pga_xd : 0;
        return pg_attrib;
}

static status _mmu_mark2m(uintptr_t vaddr,uintptr_t phys,u64 attrib) {
        index pml3i = vaddr >> 39 & 0x1FF;
        index pml2i = vaddr >> 30 & 0x1FF;
        index pml1i = vaddr >> 21 & 0x1FF;
        status s;
        uintptr_t phys_l3 = 0;
        uintptr_t phys_l2 = 0;
        u64* pml4 = (u64*)(PML4_ADDRESS);
        if(!pml4[pml3i] & 1) { 
                s = new_page_table(3,&phys_l3);
                status_assert_ok(s) {
                        return s;
                }
                pml4[pml3i] = phys_l3 | 3;
        }
        u64* pml3 = PGT_ADDRESS(511,511,pml3i);
        if(!pml3[pml2i] & 1) {
                s = new_page_table(2,&phys_l2);
                status_assert_ok(s) {
                        if(phys_l3) { free_page_table(phys_l3); }
                        return s;
                }
                pml3[pml2i] = phys_l2 | 3;
        }
        u64* pml2 = PGT_ADDRESS(511,pml3i,pml2i);
        pml2[pml1i] = phys | attrib | (1ul << 7);
        return gsd_ok;
}

static status _mmu_mark4k(uintptr_t vaddr,uintptr_t phys,u64 attrib) {
        index pml3i = vaddr >> 39 & 0x1FF;
        index pml2i = vaddr >> 30 & 0x1FF;
        index pml1i = vaddr >> 21 & 0x1FF;
        index pgi = vaddr >> 12 & 0x1FF;
        status s;

        uintptr_t phys_l3 = 0;
        uintptr_t phys_l2 = 0;
        uintptr_t phys_l1 = 0;
        u64* pml4 = (u64*)(PML4_ADDRESS);
        if(!pml4[pml3i] & 1) { 
                s = new_page_table(3,&phys_l3);
                status_assert_ok(s) {
                        return s;
                }
                pml4[pml3i] = phys_l3 | 3;
        }
        u64* pml3 = PGT_ADDRESS(511,511,pml3i);
        if(!pml3[pml2i] & 1) {
                s = new_page_table(2,&phys_l2);
                status_assert_ok(s) {
                        if(phys_l3) { free_page_table(phys_l3); }
                        return s;
                }
                pml3[pml2i] = phys_l2 | 3;
        }
        u64* pml2 = PGT_ADDRESS(511,pml3i,pml2i);
        if(!pml2[pml1i] & 1) { 
                s = new_page_table(1,&phys_l1);
                status_assert_ok(s) {
                        if(phys_l3) { free_page_table(phys_l3);}
                        if(phys_l2) { free_page_table(phys_l2);}
                        return s;
                }
                pml2[pml1i] = phys_l1 | 3;
        }
        u64* pml1 = PGT_ADDRESS(pml3i,pml2i,pml1i);
        pml1[pgi] = phys | attrib;
        return gsd_ok;
}


static status _mmu_map_2m(gsdpage* page,u64 attrib) {
        index num_blocks = (1 << (page->order-9));
        bool failed = false;
        status s;
        index failed_idx = 0;
        uintptr_t pbase = GsdGetPaddr(page);
        for(index i = 0; i < num_blocks; i++) {
                status s = _mmu_mark2m(page->vaddr+(i << 21),pbase+(i << 21),attrib);
                status_assert_ok(s) { return s;}
        }
        return gsd_ok;
}




status mmu_map(gsdpage* page) {
        if(!page->vaddr) { return gsd_invalid_param;}
        u64 attrib = get_page_attributes(page);
        uintptr_t phys = GsdGetPaddr(page);
        status s;
        if(page->order >= 9) 
        {
                return _mmu_map_2m(page,attrib);
        }
        index num_pages = (1ul << page->order);
        for(index i = 0; i < num_pages; i++) {
               s = _mmu_mark4k(page->vaddr+(i << 12),phys+(i << 12),attrib);
               status_assert_ok(s) { return s;}
        }
        return gsd_ok;
}

void mmu_unmap(uintptr_t vaddr ) {
        index pml3i = vaddr >> 39 & 0x1FF;
        index pml2i = vaddr >> 30 & 0x1FF;
        index pml1i = vaddr >> 21 & 0x1FF;
        index pgi = vaddr >> 12 & 0x1FF;

        
        u64* pml4 = (u64*)(PML4_ADDRESS);
        if(!pml4[pml3i] & 1) { return;}
        u64* pml3 = PGT_ADDRESS(511,511,pml3i);
        if(!pml3[pml2i] & 1) { return;}
        if(pml3[pml2i] & (1 << 7)) { pml3[pml2i] = 0; return;}
        u64* pml2 = PGT_ADDRESS(511,pml3i,pml2i);
        if(!pml2[pml1i] & 1) { return;}
        if(pml2[pml1i] & (1 << 7)) { pml2[pml1i] = 0; return;}
        u64* pml1 = PGT_ADDRESS(pml3i,pml2i,pml1i);
        pml1[pgi] = 0;

}


void mmu_init() {
        u32 _cpuid_arr[4];
        _cpuid(1,(u32*)&_cpuid_arr);
        if(_cpuid_arr[3] & (1 << 16)) {
               _wrmsr(PAT_MSR,GSD_PAT); // setup PAT 
               pat_supported = true;
        }

}       


