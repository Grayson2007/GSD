#include <efi-headers/Include/Uefi.h>
#include <efi-headers/Include/Protocol/LoadedImage.h>
#include <efi-headers/Include/Protocol/GraphicsOutput.h>
#include <efi-headers/Include/Guid/FileInfo.h>
#include <efi-headers/Include/Protocol/SimpleFileSystem.h>
#include <gsd-memory.h>
#include <impl/x86_64/x86_64-boot-table.h>
#include <impl/x86_64/vmmap.h>
#include <impl/x86_64/pagingx64.h>
#include <elf.h>
static EFI_SYSTEM_TABLE* ST;
static EFI_HANDLE IMGH;
static UINT64* KCR3;
static gsdpage* GSD_PAGES;
static size total_pages;
static size gsdpg_array_pagec;
static size gsdpg_array_size;
static x86_64_BootTable* GSD_BOOT_TABLE;
static EFI_LOADED_IMAGE* gsdboot_img;
static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* iovol;
static EFI_FILE_PROTOCOL* rootdir;
extern NORETURN void ktramp(UINTN cr3,UINTN rip);
static struct {
        EFI_MEMORY_DESCRIPTOR* BASE;
        UINTN DESC_SZ;
        UINTN KEY;
        UINTN MAPSZ;
        UINT32 VER;
} efimmap;

size _STRLEN(CHAR16* str) {
        size n = 0;
        while(*str++) { n++;}
        return n;
}

void CleanCr3() {
        if(!KCR3) { return;}
        for(index l3 = 0; l3 < 512; l3++) {
                if(!KCR3[l3] & 1) { continue;}
                u64* pml3 = KCR3[l3] & 0xFFFFFFFFF000;
                for(index l2 = 0; l2 < 512; l2++)
                {
                        if(!pml3[l2] & 1) { continue;}
                        u64* pml2 = pml3[l2] & 0xFFFFFFFFF000;
                        for(index l1 = 0; l1 < 512; l1++)
                        {
                                if(!pml2[l1] & 1) { continue;}
                                u64* pml1 = pml2[l1] & 0xFFFFFFFFF000;
                                for(index pg = 0; pg < 512; pg++)
                                {
                                        if(!pml1[pg] & 1) {
                                                continue;
                                        }
                                        ST->BootServices->FreePages(pml1[pg] & 0xFFFFFFFFF000,1);
                                }
                                ST->BootServices->FreePages(pml1,1);
                        }
                        ST->BootServices->FreePages(pml2,1);
                }
                ST->BootServices->FreePages(pml3,1);
        }
        ST->BootServices->FreePages(KCR3,1);
}

void Clean() {
        if(GSD_PAGES) {
                ST->BootServices->FreePages(GSD_PAGES,gsdpg_array_pagec);
        }
        if(GSD_BOOT_TABLE) {
                ST->BootServices->FreePages(GSD_BOOT_TABLE,1);
        }
        CleanCr3();
        if(efimmap.BASE) {
                size mmap_pgc = (efimmap.MAPSZ >> 12) + (efimmap.MAPSZ & 0xFFF) ? 1 : 0;
                ST->BootServices->FreePages(efimmap.BASE,mmap_pgc);
        }


        

}

NORETURN void panic(EFI_STATUS code,CHAR16* msg) {
        ST->ConOut->ClearScreen(ST->ConOut);
        ST->ConOut->SetCursorPosition(ST->ConOut,0,0);
        ST->ConOut->OutputString(ST->ConOut,msg);
        Clean(); // Cleans everything 
        ST->BootServices->Exit(IMGH,code,_STRLEN(msg),msg);
        for(;;); // unlikely but stalls forever if exit fails 
}

void* AllocatePool(size memsz) {
        void* out;
        EFI_STATUS s = ST->BootServices->AllocatePool(EfiLoaderData,memsz,&out);
        if(EFI_ERROR(s)) { panic(s,"PANIC: Allocate Pool failed"); }
        return out;
}

void FreePool(void* ptr) {
        ST->BootServices->FreePool(ptr);
}

void UpdateMemoryMap() {
        EFI_MEMORY_DESCRIPTOR* oldbase = efimmap.BASE;
        UINTN oldsize = efimmap.MAPSZ;
        size old_mmap_pages_needed = (efimmap.MAPSZ >> 12) + ((efimmap.MAPSZ & 0xFFF) ? 1 : 0);
        ST->BootServices->FreePages((EFI_PHYSICAL_ADDRESS)efimmap.BASE,old_mmap_pages_needed);
        EFI_STATUS s = ST->BootServices->GetMemoryMap(&efimmap.MAPSZ,NULL,&efimmap.KEY,&efimmap.DESC_SZ,&efimmap.VER);
        if(s != EFI_BUFFER_TOO_SMALL) {
                panic(s,u"PANIC: Failed to Query Memory map");
        }
        efimmap.MAPSZ += 2*efimmap.DESC_SZ; 
        size mmap_pages_needed = (efimmap.MAPSZ >> 12) + ((efimmap.MAPSZ & 0xFFF) ? 1 : 0);
        s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,mmap_pages_needed,&efimmap.BASE);
        if(EFI_ERROR(s)) { panic(s,u"PANIC: Memory Map Allocation failed"); }
        s = ST->BootServices->GetMemoryMap(&efimmap.MAPSZ,&efimmap.BASE,&efimmap.KEY,&efimmap.DESC_SZ,&efimmap.VER);
        if(EFI_ERROR(s)) { panic(s,u"PANIC: Failed to Query Memory map"); }
}


static const UINT32 mtype_valid[] = {
        EfiConventionalMemory,
        EfiLoaderCode,
        EfiLoaderData,
        EfiBootServicesCode,
        EfiBootServicesData,
        
};
#define num_valid_types 5

bool Memory_Is_Valid(UINT32 mt) {
        for(int i = 0; i < num_valid_types; i++) {
                if(mt == mtype_valid[i]) { return true;}
        }
        return false;
}

// Sets up the kernels physical memory structures

void InitGsdPages() {
        efimmap.MAPSZ = 0;
        efimmap.BASE = NULL;

        EFI_STATUS s = ST->BootServices->GetMemoryMap(&efimmap.MAPSZ,NULL,&efimmap.KEY,&efimmap.DESC_SZ,&efimmap.VER);
        if(s != EFI_BUFFER_TOO_SMALL) {
                panic(s,u"PANIC: Failed to Query Memory map");
        }
        efimmap.MAPSZ += 2*efimmap.DESC_SZ; 

        size mmap_pages_needed = (efimmap.MAPSZ >> 12) + ((efimmap.MAPSZ & 0xFFF) ? 1 : 0);
        s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,mmap_pages_needed,&efimmap.BASE);
        if(EFI_ERROR(s)) { panic(s,u"PANIC: Memory Map Allocation failed"); }
        s = ST->BootServices->GetMemoryMap(&efimmap.MAPSZ,&efimmap.BASE,&efimmap.KEY,&efimmap.DESC_SZ,&efimmap.VER);
        if(EFI_ERROR(s)) { panic(s,u"PANIC: Failed to Query Memory map"); }

        // Query Total Pages 
        total_pages = 0;
        void* desc_ptr = (void*)efimmap.BASE;
        void* endptr = desc_ptr+efimmap.MAPSZ;
        while(desc_ptr  < endptr)
        {
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)desc_ptr;
                total_pages += desc->NumberOfPages;
                desc_ptr += efimmap.DESC_SZ;
        }
        // 
        size gsd_page_memory_needed = total_pages*sizeof(gsdpage);
        size gsd_page_pages_needed = (gsd_page_memory_needed >> 12) + (gsd_page_pages_needed & 0xFFF) ? 1 : 0;
        gsdpg_array_size = gsd_page_memory_needed;
        gsdpg_array_pagec = gsd_page_pages_needed; 
        s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,gsd_page_pages_needed,&GSD_PAGES);
        if(EFI_ERROR(s)) { panic(s,u"PANIC: Failed to Allocate Page Structures"); } 
        desc_ptr = (void*)efimmap.BASE;
        // Initalise Memory Map To pass to GSD 
        // GSD will handle buddy related stuff after boot 
        // For now set eatch page to order 0 
        while(desc_ptr < endptr) {
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)desc_ptr;
                index base_idx = ((index)desc->PhysicalStart) >> 12;
                int attrib = Memory_Is_Valid(desc->Type) ? gsdpg_valid : 0;
                for(index i = base_idx; i < base_idx+desc->NumberOfPages; i++)
                {
                        GSD_PAGES[i].addr = (uintptr_t)(i << 12);
                        GSD_PAGES[i].attrib = attrib;
                        GSD_PAGES[i].order = 0;
                        GSD_PAGES[i].nextfree = NULL;
                        GSD_PAGES[i].prevfree = NULL;
                        GSD_PAGES[i].owning_process = 0;
                }
                desc_ptr += efimmap.DESC_SZ;
        }

        // Allocate the pages that the gsd_page structures take up 
        index page_map_base = (index)GSD_PAGES >> 12;
        for(index i = page_map_base; i < gsd_page_pages_needed; i++) {
                GSD_PAGES[i].attrib = gsdpg_valid | gsdpg_active | gsdpg_page_structure;
                GSD_PAGES[i].order = 0;
                GSD_PAGES[i].owning_process = 0;
                GSD_PAGES[i].prevfree = NULL;
                GSD_PAGES[i].nextfree = NULL;
        }
}

EFI_PHYSICAL_ADDRESS allocate_new_page_table() {
        EFI_PHYSICAL_ADDRESS out;
        EFI_STATUS s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,1,&out);
        if(EFI_ERROR(s)) { panic(s,"PANIC: Page table allocation failed"); }
        index addridx = (out >> 12);
        GSD_PAGES[addridx].attrib |= gsdpg_active | gsdpg_pgtable;
        return out;
}





void EfiMarkPage(EFI_VIRTUAL_ADDRESS va,EFI_PHYSICAL_ADDRESS pa) {
        UINTN l3idx = va >> 39 & 0x1FF;
        UINTN l2idx = va >> 30 & 0x1FF;
        UINTN l1idx = va >> 21 & 0x1FF;
        UINTN pgidx = va >> 12 & 0x1FF;
        u64* pml3;
        u64* pml2;
        u64* pml1;
        u64* pgt;
        if(!KCR3[l3idx] & 1) {
                EFI_PHYSICAL_ADDRESS p = allocate_new_page_table();
                KCR3[l3idx] = p | 3;
                ST->BootServices->SetMem((void*)p,EFI_PAGE_SIZE,0);
        }
        pml3 = (u64*)(KCR3[l3idx] & 0xFFFFFFFFF000);
        if(!pml3[l2idx] & 1) {
                EFI_PHYSICAL_ADDRESS p = allocate_new_page_table();
                pml3[l2idx] = p | 3;
                ST->BootServices->SetMem((void*)p,EFI_PAGE_SIZE,0);
        }
        pml2 = (u64*)(pml3[l2idx] & 0xFFFFFFFFF000);
        if(!pml2[l1idx] & 1) {
                EFI_PHYSICAL_ADDRESS p = allocate_new_page_table();
                pml2[l1idx] = p | 3;
                ST->BootServices->SetMem((void*)p,EFI_PAGE_SIZE,0);
        }       
        pml1 = pml2[l1idx] & 0xFFFFFFFFF000;
        pml1[pgidx] = pa | 3;

}

EFI_FILE_INFO* Finfo(EFI_FILE_HANDLE f) {
        EFI_STATUS s;
        EFI_GUID info_id = EFI_FILE_INFO_ID;
        UINTN finfo_sz = sizeof(EFI_FILE_INFO)+64;
        EFI_FILE_INFO* ptr = NULL;
        while(1)
        {
                ptr = AllocatePool(finfo_sz);
                s = f->GetInfo(f,&info_id,&finfo_sz,ptr);   
                if(s == EFI_BUFFER_TOO_SMALL) {
                        FreePool(ptr);
                        finfo_sz += 64;
                        continue;
                }
                break;
        }
        if(EFI_ERROR(s)) { panic(s,"PANIC: Failed to get file info"); }
        return ptr;
}

EFI_STATUS Fopen(EFI_FILE_HANDLE dir,CHAR16* path,UINT64 mode,UINTN attrib,EFI_FILE_HANDLE* out) {
        return dir->Open(dir,out,path,mode,attrib);
}

void GetVol() {
        EFI_GUID fspid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
        ST->BootServices->HandleProtocol(gsdboot_img->DeviceHandle,&fspid,&iovol);
        iovol->OpenVolume(iovol,&rootdir);
}

void LoadKernel(Elf64_Ehdr* elf)
{
        // verify elf
}

void LoadIniramfs() {

}

NORETURN void Boot(uintptr_t RIP) {
        ktramp(KCR3,RIP);
}

void GetImage() {
        EFI_GUID lipid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
        EFI_STATUS s = ST->BootServices->HandleProtocol(IMGH,&lipid,&gsdboot_img);
        if(EFI_ERROR(s)) { panic(s,"PANIC: Failed to get gsdboot image");}

}


void InitPageTable() {
        EFI_STATUS s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,1,&KCR3);
        if(EFI_ERROR(s)) { panic(s,"PANIC: Failed to initalise kernel page table"); }
        ST->BootServices->SetMem((void*)KCR3,EFI_PAGE_SIZE,0);
        index pgidx = (index)KCR3 >> 12;
        GSD_PAGES[pgidx].attrib |= gsdpg_pgtable | gsdpg_active | gsdpg_valid;
}

void InitBootTable() {
        EFI_STATUS s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,1,&GSD_BOOT_TABLE);
        if(EFI_ERROR(s)) { panic(s,u"PANIC: Boot table allocation failed");}
        GSD_BOOT_TABLE->signature = boottab_signature;
        GSD_BOOT_TABLE->mode = bm_efi;
        EfiMarkPage(BOOT_TABLE_ADDRESS,GSD_BOOT_TABLE);       
}
void MarkLoader() {
        size loaderpgc = (gsdboot_img->ImageSize >> 12) + (gsdboot_img->ImageSize & 0xFFF) ? 1 : 0;
        EFI_PHYSICAL_ADDRESS addr = (EFI_PHYSICAL_ADDRESS)gsdboot_img->ImageBase;
        for(index i = 0; i < loaderpgc; i++)
        
        {
                EfiMarkPage(addr,addr);
                addr += EFI_PAGE_SIZE;
        } 
}

EFI_STATUS efi_main(EFI_HANDLE imghandle,EFI_SYSTEM_TABLE* tab) {
        ST = tab;
        IMGH = imghandle;
        InitGsdPages();
        InitPageTable();
        GetImage();
        GetVol();
        InitBootTable();
        MarkLoader();
        Menu();

        return EFI_ABORTED;
}
