#include "bootefi.h"
struct s_efimmap EFIMMAP;
u64* KCR3;
uintptr_t MAX_PHYSICAL_ADDRESS;

// Tracks physical addresses being used by certain resources 
struct s_page_tracker {
        size count;
        size capacity;
        struct s_page_tracker* next;
        uintptr_t addresses[1]; 

};
#define tracker_capacity = ((EFI_PAGE_SIZE-sizeof(struct s_page_tracker))/sizeof(void*))
// This is used for tracking pages used by mem_map quickly
// These pages are all freed up and marked as free in mem_map 

struct s_page_tracker* pt_root_tracker;
struct s_page_tracker* mmap_root_tracker;
static bool _page_mapped(uintptr_t ptr) {
        index pml3i = ptr >> 39 & 0x1FF;
        index pml2i = ptr >> 30 & 0x1FF;
        index pml1i = ptr >> 21 & 0x1FF;
        index pgi = ptr >> 12 & 0x1FF;
        if(!KCR3[pml3i] & 1) { return false;}
        u64* pml3 = (u64*)(KCR3[pml3i] & ~((uintptr_t)0xFFF));
        if(!pml3[pml2i] & 1) { return false; }
        u64* pml2 = (u64*)(pml3[pml2i] & ~((uintptr_t)0xFFF));
        if(!pml2[pml1i] & 1) { return false;}
        u64* pml1 = (u64*)(pml1[pml1i] & ~((uintptr_t)0xFFF));
        return pml1[pgi] & 1 ? true : false;
}


void efi_UpdateMemoryMap() {
        if(EFIMMAP.BASE) {
                size efimmap_pagec = (EFIMMAP.MAPSZ/4096) + ((EFIMMAP.MAPSZ % 4096) ? 1 : 0);
                ST->BootServices->FreePages(EFIMMAP.BASE,efimmap_pagec);
        }
        // Query the new size 
        EFIMMAP.BASE = NULL;
        EFIMMAP.MAPSZ = 0;
        EFIMMAP.DESC_SZ = 0;
        EFIMMAP.MAPSZ = 0;
        EFIMMAP.VER = 0;
        ST->BootServices->GetMemoryMap(&EFIMMAP.MAPSZ,NULL,&EFIMMAP.KEY,&EFIMMAP.DESC_SZ,&EFIMMAP.VER);

        EFIMMAP.MAPSZ += (2*EFIMMAP.DESC_SZ);
        size pagec = (EFIMMAP.MAPSZ/4096) + ((EFIMMAP.MAPSZ % 4096) ? 1 : 0);
        EFI_STATUS s = efi_AllocAnyPages(pagec,&EFIMMAP.BASE);
        if(EFI_ERROR(s)) {
                // This would fucking suck ass 
                efi_PANIC(u"Unable to update memory map",s);
        }
        ST->BootServices->GetMemoryMap(&EFIMMAP.MAPSZ,EFIMMAP.BASE,&EFIMMAP.KEY,&EFIMMAP.DESC_SZ,&EFIMMAP.VER);
        efi_println(u"Memory Map Updated!");
        efi_print(u"Map Base: 0x");
        efi_printu64((u64)EFIMMAP.BASE,16);
        efi_println(u"");
        efi_print(u"Map Size: ");
        efi_printu64((u64)EFIMMAP.MAPSZ,10);
        efi_println(u"Bytes");
        efi_print(u"Descriptor Size: ");
        efi_printu64((u64)EFIMMAP.DESC_SZ,10);
        efi_println(u"Bytes");
}

EFI_STATUS efi_AllocPool(size poolsz,void** addr) {
        return ST->BootServices->AllocatePool(EfiLoaderData,poolsz,addr);
}

void efi_FreePool(void* addr) {
        ST->BootServices->FreePool(addr);
}


EFI_STATUS efi_AllocAnyPages(size count,EFI_PHYSICAL_ADDRESS* outptr) {
        return ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,count,outptr);
}
EFI_STATUS efi_FreePages(EFI_PHYSICAL_ADDRESS addr,size count) {
        return ST->BootServices->FreePages(addr,count);
}


static const UINT32 usable_memtypes[] = {
        EfiLoaderCode,
        EfiLoaderData,
        EfiBootServicesCode,
        EfiBootServicesData,
        EfiConventionalMemory
};
//  
static const UINT32 firmware_rsvd_mem_types[] = {
        EfiACPIMemoryNVS,
        EfiACPIReclaimMemory,
        EfiRuntimeServicesCode,
        EfiRuntimeServicesData,

};

static inline bool memtype_usable(UINT32 type) {
        for(int i = 0; i < 5; i++) {
                if(type == usable_memtypes[i]) { return true;}
        }
        return false;
}


static inline bool mem_firmrsvd(UINT32 type) {
        for(int i = 0; i < 4; i++) {
                if(type == firmware_rsvd_mem_types[i]) { return true;}
        }
        return false;
}

void tracker_append(struct s_page_tracker* tracker,uintptr_t paddr) {
        while(tracker->count >= tracker->capacity)
        {
                if(!tracker->next) 
                {
                        EFI_STATUS s;
                        EFI_PHYSICAL_ADDRESS new_addr;
                        s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,1,&new_addr);
                        if(EFI_ERROR(s)) {
                                efi_PANIC(u"Unable to allocate tracker structure",s);
                        }
                        ST->BootServices->SetMem((void*)new_addr,EFI_PAGE_SIZE,0);
                        struct s_page_tracker* new = (struct s_page_tracker*)new_addr;
                        new->addresses[0] = new_addr;
                        new->capacity = ((EFI_PAGE_SIZE-sizeof(struct s_page_tracker))/sizeof(void*));
                        new->next = NULL;
                        tracker->next = new;
                        break;
                }
                tracker = tracker->next;
        }
        tracker->addresses[tracker->count] = paddr;
        tracker->count++;
}







u64* NewPageTable() {
        EFI_PHYSICAL_ADDRESS table;
        EFI_STATUS s = efi_AllocAnyPages(1,&table);
        if(EFI_ERROR(s)) { efi_PANIC(u"Page table allocation failed",s); }
        if(EFI_ERROR(s)) { ST->BootServices->FreePages(table,1); efi_PANIC(u"Page table allocation failed",s);}
        tracker_append(pt_root_tracker,(uintptr_t)table);
        return (u64*)table;
}





EFI_STATUS efi_KCR3_map(EFI_VIRTUAL_ADDRESS virt,EFI_PHYSICAL_ADDRESS phys) {
        u64* pml4 = KCR3;
        u64* pml3;
        u64* pml2;
        u64* pml1;
        index pml3i = virt >> 39 & 0x1FF;
        index pml2i = virt >> 30 & 0x1FF;
        index pml1i = virt >> 21 & 0x1FF;
        index pgi = virt >> 12 & 0x1FF;
        EFI_STATUS s;
        if(!pml4[pml3i] & 1)  { 
                u64 paddr = (u64)NewPageTable();
                pml4[pml3i] = paddr | 3;
        }
        pml3 = (u64*)(pml4[pml3i] & ~((u64)0xFFF));
        if(!pml3[pml2i] & 1) {
                u64 paddr = (u64)NewPageTable();
                pml3[pml2i] = paddr | 3;
        }
        pml2 = (u64*)(pml3[pml2i] & ~((u64)0xFFF));
        if(!pml2[pml1i] & 1) { 
                u64 paddr = (u64)NewPageTable();
                pml2[pml1i] = paddr | 3;
        }
        pml1 =  (u64*)(pml2[pml1i] & ~((u64)0xFFF));
        pml1[pgi] = phys | 3;
        return EFI_SUCCESS;
}
static uintptr_t get_paddr(uintptr_t vptr) {
        index pml3i = vptr >> 39 & 0x1FF;
        index pml2i = vptr >> 30 & 0x1FF;
        index pml1i = vptr >> 21 & 0x1FF;
        index pgi = vptr >> 12 & 0x1FF;
        if(!KCR3[pml3i] & 1) { return 0;}
        u64* pml3 = (u64*)(KCR3[pml3i] & ~((uintptr_t)0xFFF));
        if(!pml3[pml2i] & 1) { return 0; }
        u64* pml2 = (u64*)(pml3[pml2i] & ~((uintptr_t)0xFFF));
        if(!pml2[pml1i] & 1) { return 0;}
        u64* pml1 = (u64*)(pml1[pml1i] & ~((uintptr_t)0xFFF));
        return pml1[pgi] & 1 ? pml1[pgi] & ~((uintptr_t)0xFFF) : 0;
        
}


EFI_STATUS efi_KCR3_nmap(EFI_VIRTUAL_ADDRESS virt,EFI_PHYSICAL_ADDRESS phys,size n) {
        for(index i = 0; i < n; i++) {
                EFI_STATUS s;
                s = efi_KCR3_map(virt,phys);
                if(EFI_ERROR(s)) { return s;}
                virt += EFI_PAGE_SIZE;
                phys += EFI_PAGE_SIZE;
        }
}
static inline bool memory_valid(UINT32 T) {
        if(T == EfiUnusableMemory || T == EfiReservedMemoryType || T >= EfiMaxMemoryType) { return false;}
        return true;
}



u64 efi_GetGsdMemflags(EFI_MEMORY_DESCRIPTOR* desc) {
 
        u64 flags = memory_valid(desc->Type) ? gsdpg_a_valid : 0;
        flags |= desc->Attribute & EFI_MEMORY_UC ? gsdpg_c_uc : 0;
        flags |= desc->Attribute & EFI_MEMORY_WC ? gsdpg_c_wc : 0;
        flags |= desc->Attribute & EFI_MEMORY_WT ? gsdpg_c_wt : 0;
        flags |= desc->Attribute & EFI_MEMORY_WB ? gsdpg_c_wb : 0;
        flags |= desc->Attribute & EFI_MEMORY_WP ? gsdpg_c_wp : 0;
        flags |= desc->Attribute & EFI_MEMORY_RP ? gsdpg_c_rp : 0;
        flags |= desc->Attribute & EFI_MEMORY_XP ? gsdpg_c_xp : 0;
        flags |= desc->Attribute & EFI_MEMORY_RO ? gsdpg_a_readonly : 0;
        if(desc->PhysicalStart+(desc->NumberOfPages*EFI_PAGE_SIZE) <= 0xFFFFFFFF) { // Check for DMA and low memory 
                flags |= gsdpg_a_low_mem | gsdpg_c_dma;
        }
        return flags;
}





enum gsd_memtypes efi_ToGsdMemType(UINT32 efi_type) {
        if(memtype_usable(efi_type)){ return gsdmem_Conventional;}
        if(mem_firmrsvd(efi_type)) {return gsdmem_FirmwareRsvd;}
        return gsdmem_Invalid;
}



// Ma
static void mem_map_mark_page(index pgi,u64 new_attrib) {

        gsdpage* virtual_pointer_base = (gsdpage*)(MEM_MAP_ADDRESS);
        if(!_page_mapped(&virtual_pointer_base[pgi])) {
                EFI_PHYSICAL_ADDRESS paddr;
                EFI_STATUS s = efi_AllocAnyPages(1,&paddr);
                if(EFI_ERROR(s)) { efi_PANIC(u"Unable to allocate mem_map structure",s);}
                tracker_append(mmap_root_tracker,paddr);
                ST->BootServices->SetMem((void*)paddr,EFI_PAGE_SIZE,0);
                efi_KCR3_map((EFI_VIRTUAL_ADDRESS)&virtual_pointer_base[pgi],paddr);
        }
        index rel_index = pgi % (EFI_PAGE_SIZE/sizeof(gsdpage));
        gsdpage* pgarr = (gsdpage*)get_paddr((uintptr_t)&virtual_pointer_base[pgi]);
        pgarr[rel_index].flags = (pgarr[rel_index].flags & ~(page_status_mask)) | new_attrib;
}

EFI_STATUS AllocateKernelPages(EFI_PHYSICAL_ADDRESS* outphys,size n,u64 attrib) {
        
        EFI_STATUS s = efi_AllocAnyPages(n,outphys);
        if(EFI_ERROR(s)) { return s;}
        for(index i = 0; i < n; i++) {
                mem_map_mark_page((index)((*outphys/EFI_PAGE_SIZE)),attrib);
        }
        return EFI_SUCCESS;
}


void Tracker_Create(struct s_page_tracker** outptr) {
        EFI_PHYSICAL_ADDRESS tracker_paddr;
        EFI_STATUS s;
        s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,1,&tracker_paddr);
        if(EFI_ERROR(s)) {
                efi_PANIC(u"Page tracker init failed",s);
        }
        ST->BootServices->SetMem((void*)tracker_paddr,EFI_PAGE_SIZE,0);
        struct s_page_tracker* tracker = (struct s_page_tracker*)tracker_paddr;
        tracker->addresses[0] = tracker_paddr;
        tracker->count = 1;
        tracker->capacity = ((EFI_PAGE_SIZE-sizeof(struct s_page_tracker))/sizeof(void*));
        *outptr = tracker;
}


void Pmm_Init() {
        efi_println(u"Allocating Low Bootstrap Region");
        // Memory in 0x1000-0x4000 in GSD kernels are used for bootstrapping CPU cores
        EFI_STATUS s;
        EFI_PHYSICAL_ADDRESS low_base = 0x1000;
        s = ST->BootServices->AllocatePages(AllocateAddress,EfiLoaderData,3,&low_base);
        if(EFI_ERROR(s)) {
                efi_PANIC(u"Low Bootstrap Region Cannot be Allocated",s);
        }

        efi_UpdateMemoryMap();
        uintptr_t cptr = (uintptr_t)EFIMMAP.BASE;
        uintptr_t eptr = (uintptr_t)cptr+EFIMMAP.MAPSZ;
        efi_print(u"Memory map located at 0x");
        efi_printu64((u64)EFIMMAP.BASE,16);
        efi_println(u" ");
        // Allocate a Page table virtual pointer (PTVP)
        // Pass 1
        // Calculate Total Addressable Pages and allocate space 
        // For Potential Page tables 
        // (Will allocate Plenty of them) 
        efi_println(u"Initalising Memory Mangement Structures");
        efi_println(u"Setting up page table tracker");
        Tracker_Create(&pt_root_tracker);
        efi_println(u"Page table tracker setup sucessfully");
        efi_println(u"Setting up mem_map tracker");
        Tracker_Create(&mmap_root_tracker);
        efi_println(u"mem_map tracker setup sucessfully!");
        EFI_MEMORY_DESCRIPTOR* desc;
        uintptr_t max_ptr;
        while(desc < eptr) {
                desc = (EFI_MEMORY_DESCRIPTOR*)cptr;
                if(memory_valid(desc->Type)) 
                {
                        efi_print(u"Found Valid Memory at 0x");
                        efi_printu64((u64)desc->PhysicalStart,16);
                        efi_print(u" Num pages: ");
                        efi_printu64((u64)desc->NumberOfPages,10);
                        efi_println(u".");
                        uintptr_t endptr = (uintptr_t)(desc->PhysicalStart+(EFI_PAGE_SIZE*desc->NumberOfPages));
                        if(endptr >= max_ptr) {
                                max_ptr = endptr;
                        }
                        u64 flags = efi_GetGsdMemflags(desc->Attribute);
                        if(
                                desc->Type == EfiRuntimeServicesCode || 
                                desc->Type == EfiRuntimeServicesData ||
                                desc->Type == EfiACPIMemoryNVS ||
                                desc->Type == EfiACPIReclaimMemory
                        ) 
                        {
                                flags |= gsdpg_a_identmap;
                                flags |= gsdpg_s_reserved;
                        }
                        index start_phys_index = desc->PhysicalStart/EFI_PAGE_SIZE;
                        index end_index = start_phys_index+desc->NumberOfPages;
                        for(index i = start_phys_index; i < end_index; i++) {
                                mem_map_mark_page(i,flags);
                        }
                }
                cptr += EFIMMAP.MAPSZ;
        }
        efi_println(u"Query Completed!");
        MAX_PHYSICAL_ADDRESS = max_ptr;
        // Mark the mem_map pages 


}

void efi_MarkTrackedPages() {
        struct s_page_tracker* mem_map_tracker = mmap_root_tracker;
        struct s_page_tracker* pt_tracker = pt_root_tracker;

        while(1) {
                for(index i = 1; i < mem_map_tracker->count; i++) {
                        mem_map_mark_page((index)(mem_map_tracker->addresses[i]/EFI_PAGE_SIZE),gsdpg_mem_map_attrib);
                }
                if(!mem_map_tracker->next) { break;}
                mem_map_tracker = mem_map_tracker->next;
        }

        while(1) { // page tables 
                for(index i = 1; i < pt_tracker->count; i++) {
                        mem_map_mark_page((index)(pt_tracker->addresses[i]/EFI_PAGE_SIZE),gsdpage_kpgtable_attrib);
                }
                if(!pt_tracker->next) { break;}
                pt_tracker = pt_tracker->next;
  
        }

}


void efi_CleanTracker(struct s_page_tracker* root) {
        while(1) {
                EFI_PHYSICAL_ADDRESS addr = (uintptr_t)root->addresses[0];
                struct s_page_tracker* next = root->next;
                ST->BootServices->FreePages(addr,1);
                if(!next) { break; }
                root = root->next;
        }
}

void efi_FlushPmm() {
        efi_MarkTrackedPages();
        efi_CleanTracker(pt_root_tracker);
        efi_CleanTracker(mmap_root_tracker);
}