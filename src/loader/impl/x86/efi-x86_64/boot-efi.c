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
static EFI_FILE_HANDLE gsdbdir;
static EFI_FILE_HANDLE optdir;
static int BOOT_OPT_COUNT;
static int BOOT_OPT_SELECTED;
#define OPTIONS_PER_PAGE 4
extern NORETURN void ktramp(UINTN cr3,UINTN rip);

struct s_bootopt {
        CHAR16* kernal_name;
        struct s_bootopt* next;
        struct s_bootopt* prev;
};

struct s_bootopt* root_opt;
struct s_bootopt* selected_opt;
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
        if(root_opt) {
                struct s_bootopt* curr_opt = root_opt;
                struct s_bootopt* next_opt;
                while(curr_opt) {
                        next_opt = curr_opt->next;
                        ST->BootServices->FreePool(curr_opt->kernal_name);
                        ST->BootServices->FreePool(curr_opt);
                        curr_opt = next_opt;
                }
        }

}


NORETURN void stall_panic(CHAR16* msg) {
        ST->ConOut->ClearScreen(ST->ConOut);
        ST->ConOut->SetCursorPosition(ST->ConOut,0,0);
        ST->ConOut->OutputString(ST->ConOut,msg);
        ST->ConOut->SetCursorPosition(ST->ConOut,0,1);
        ST->ConOut->OutputString(ST->ConOut,u"Your system has now stalled. Please do a manual reboot");
        for(;;);
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
// Updates the memory map 
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
#define _SET_ATTRIB(EFIA,GSDA) attrib |= desc->Attribute & EFIA ? GSDA : 0
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
                int attrib = Memory_Is_Valid(desc->Type) ? gsdpg_valid | gsdpg_dma_capable : 0;
                _SET_ATTRIB(EFI_MEMORY_XP,gsdpg_xp);
                _SET_ATTRIB(EFI_MEMORY_RP,gsdpg_rp);
                _SET_ATTRIB(EFI_MEMORY_WP,gsdpg_wp);
                _SET_ATTRIB(EFI_MEMORY_WB,gsdpg_wb);
                _SET_ATTRIB(EFI_MEMORY_WT,gsdpg_wt);
                _SET_ATTRIB(EFI_MEMORY_WC,gsdpg_wc);
                _SET_ATTRIB(EFI_MEMORY_UC,gsdpg_uc);
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
                GSD_PAGES[i].attrib |= gsdpg_valid | gsdpg_active | gsdpg_page_structure;
                GSD_PAGES[i].order = 0;
                GSD_PAGES[i].owning_process = 0;
                GSD_PAGES[i].prevfree = NULL;
                GSD_PAGES[i].nextfree = NULL;
        }
}

EFI_PHYSICAL_ADDRESS allocate_new_page_table() {
        EFI_PHYSICAL_ADDRESS out;
        EFI_STATUS s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,1,&out);
        if(EFI_ERROR(s)) { stall_panic(u"PANIC: Page table allocation failed"); }
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



// Exit Boot services and make the jump to the early stage of the kernel
NORETURN void Boot(uintptr_t RIP) {
        ST->BootServices->ExitBootServices(IMGH,efimmap.KEY);
        ktramp(KCR3,RIP);
}

void GetImage() {
        EFI_GUID lipid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
        EFI_STATUS s = ST->BootServices->HandleProtocol(IMGH,&lipid,&gsdboot_img);
        if(EFI_ERROR(s)) { panic(s,"PANIC: Failed to get gsdboot image");}

}

void SetupGop() {
        EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
        EFI_STATUS s = ST->BootServices->LocateProtocol(&gop_guid,NULL,(void**)&gop);
        if(EFI_ERROR(s)) {
                panic(s,u"PANIC: Unable to locate GOP");
        }
        UINTN modeinf_size;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* inf;
        s = gop->QueryMode(gop,gop->Mode == NULL ? 0 : gop->Mode->Mode,&modeinf_size,&inf);
        if(s == EFI_NOT_STARTED) {
                s = gop->SetMode(gop,0);
        }
        if(EFI_ERROR(s)) {
                panic(s,u"PANIC: Gop init failed");
        }
        UINTN num_modes = gop->Mode->MaxMode;
        UINTN native_mode = gop->Mode->Mode;

        UINTN best_dims = 0;
        UINTN best_mode = 0;
        for(UINT32 i = 0; i < num_modes; i++)
        {
                gop->QueryMode(gop,i,&modeinf_size,&inf);
                if(inf->PixelFormat != PixelRedGreenBlueReserved8BitPerColor) { continue;}
                if(inf->HorizontalResolution*inf->VerticalResolution > best_dims) {
                        best_mode = i;
                }
        }
        gop->SetMode(gop,best_mode);
        GSD_BOOT_TABLE->fb.depth = 32;
        GSD_BOOT_TABLE->fb.pbase = gop->Mode->FrameBufferBase;
        GSD_BOOT_TABLE->fb.fbsz = gop->Mode->FrameBufferSize;
        GSD_BOOT_TABLE->fb.w = gop->Mode->Info->HorizontalResolution;
        GSD_BOOT_TABLE->fb.h = gop->Mode->Info->VerticalResolution;
        GSD_BOOT_TABLE->fb.rmask = gop->Mode->Info->PixelInformation.RedMask;
        GSD_BOOT_TABLE->fb.bmask = gop->Mode->Info->PixelInformation.BlueMask;
        GSD_BOOT_TABLE->fb.gmask = gop->Mode->Info->PixelInformation.GreenMask;
        GSD_BOOT_TABLE->fb.ppsl = gop->Mode->Info->PixelsPerScanLine;
        GSD_BOOT_TABLE->fb.pitch = 4*gop->Mode->Info->PixelsPerScanLine;

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

static void Load_Kernel(Elf64_Ehdr* ehdr) {
        void* baseptr = (void*)ehdr;
        u32* magicptr = (u32*)ehdr;
        if(*magicptr != '\x7FELF') { panic(EFI_INVALID_PARAMETER,u"PANIC: Kernel is not an elf");}
        if(ehdr->e_machine != EM_X86_64) { panic(EFI_INVALID_PARAMETER,u"PANIC: Kernel is not for x86_64");}
        
        size phcount = ehdr->e_phnum;
        size phsz = ehdr->e_phentsize;

        Elf64_Phdr* phdrs = (Elf64_Phdr*)(baseptr+ehdr->e_phoff);
        void* cptr = (void*)phdrs;
        for(index i = 0; i < phcount; i++) {
                Elf64_Phdr* phdr = (Elf64_Phdr*)cptr;
                if(phdr->p_type == PT_LOAD)
                {
                        size memsz = phdr->p_memsz;
                        void* copy_base = (void*)(baseptr+phdr->p_offset);
                        size copysz = phdr->p_filesz;
                        size pagec = (memsz >> 12) + (memsz & 0xFFF) ? 1 : 0;
                        EFI_PHYSICAL_ADDRESS pbase;
                        EFI_STATUS s = ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,pagec,&pbase);
                        if(EFI_ERROR(s)) {
                                stall_panic(u"Unable to allocate Memory for Kernel Binary");
                        }
                        ST->BootServices->CopyMem((void*)pbase,copy_base,copysz);
                        size zero_count = memsz-copysz;
                        ST->BootServices->SetMem((void*)pbase+copysz,zero_count,0);
                        // Map the kernel
                        EFI_VIRTUAL_ADDRESS vbase = (EFI_VIRTUAL_ADDRESS)phdr->p_vaddr;
                        for(index p = 0; p < pagec; p++) {
                                EfiMarkPage(vbase,pbase);
                                vbase += EFI_PAGE_SIZE;
                                pbase += EFI_PAGE_SIZE;
                        }
                }
        }
}

NORETURN void BootSelected() {
        EFI_FILE_HANDLE kernelbin;
        EFI_FILE_HANDLE irfsbin;

        // Load the Kernel 
        optdir->Open(optdir,&kernelbin,u"kernel.elf",EFI_FILE_MODE_READ,EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
        EFI_FILE_INFO* kernelbin_info = Finfo(kernelbin);
        size kernsz = kernelbin_info->FileSize;
        ST->BootServices->FreePool(kernelbin_info);
        size kernbinpgc = (kernsz >> 12) + (kernsz & 0xFFF) ? 1 : 0;
        Elf64_Ehdr* kbuff;
        ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,kernbinpgc,(EFI_PHYSICAL_ADDRESS*)&kbuff);
        kernelbin->Read(kernelbin,&kernsz,kbuff);
        uintptr_t entry = kbuff->e_entry;
        Load_Kernel((Elf64_Ehdr*)kbuff);
        ST->BootServices->FreePages(kbuff,kernbinpgc);
        kernelbin->Close(kernelbin);


        // Load the Initramfs Image 
        optdir->Open(optdir,&irfsbin,u"irfs.img",EFI_FILE_MODE_READ,EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
        EFI_FILE_INFO* irfsinf = Finfo(irfsbin);
        size irfssz - irfsbin.


        // Finalise EFI Stage  by setting up GOP and getting the final Memory Map 
        SetupGop();
        UpdateMemoryMap();
        GSD_BOOT_TABLE->mmap.efi = efimmap.BASE;
        GSD_BOOT_TABLE->mmapdescriptorc = efimmap.MAPSZ/efimmap.DESC_SZ;
        Boot(entry);
}


void UpdateMenu() {
        ST->ConOut->ClearScreen(ST->ConOut);
        ST->ConOut->SetCursorPosition(ST->ConOut,1,1);
        ST->ConOut->OutputString(ST->ConOut,"Select Kernel");
        ST->ConOut->SetCursorPosition(ST->ConOut,4,1);
        index page = BOOT_OPT_SELECTED/OPTIONS_PER_PAGE;
        index Selected_index = BOOT_OPT_SELECTED % OPTIONS_PER_PAGE;

        struct s_bootopt* inital_option = root_opt;
        for(index i = 0; i < page; i++) {
                for(index j = 0; j < OPTIONS_PER_PAGE; j++) {
                        inital_option = inital_option->next;
                }
        }


        struct s_bootopt* curr_opt = inital_option;
        for(index i = 0; i < OPTIONS_PER_PAGE; i++) {
                if(i == Selected_index) {
                        ST->ConOut->OutputString(ST->ConOut,u" >>> ");
                        selected_opt = curr_opt;
                }
                ST->ConOut->OutputString(ST->ConOut,curr_opt->kernal_name);
                curr_opt = curr_opt->next;
        }
}

void Menu() {
        UINTN conin_idx;
        EFI_INPUT_KEY key;
        while(1) {
                ST->BootServices->WaitForEvent(1,&ST->ConIn->WaitForKey,&conin_idx);
                ST->ConIn->ReadKeyStroke(ST->ConIn,&key);
                UpdateMenu();
                if(key.UnicodeChar == u'\n') { BootSelected();}
                switch(key.ScanCode) 
                {
                        case SCAN_UP:
                                BOOT_OPT_SELECTED--;
                                if(BOOT_OPT_SELECTED <= 0) { BOOT_OPT_SELECTED = BOOT_OPT_COUNT-1;}
                                break;
                        case SCAN_DOWN:
                                BOOT_OPT_SELECTED++;
                                if(BOOT_OPT_SELECTED >= BOOT_OPT_COUNT) {
                                        BOOT_OPT_SELECTED = 0;
                                }
                                break;
                }
        
        }
}



void OpenGsdbDirectory() {
        EFI_STATUS s = Fopen(rootdir,u"gsdb",EFI_FILE_MODE_READ,EFI_FILE_DIRECTORY,&gsdbdir);
        if(EFI_ERROR(s)) {
                panic(s,u"Failed to open gsdb Directory");
        }

}

void LoadBootOptions() {
        size buffsz = sizeof(EFI_FILE_INFO)+512;
        EFI_FILE_INFO* inf = AllocatePool(buffsz);
        EFI_STATUS s;
        root_opt = AllocatePool(sizeof(struct s_bootopt));
        struct s_bootopt* last_opt = NULL;
        struct s_bootopt* curropt = root_opt;
        BOOT_OPT_COUNT = 1;
        while(1) {
                s = gsdbdir->Read(gsdbdir,&buffsz,inf);
                if(EFI_ERROR(s) || buffsz == 0) {
                        break;
                }

                if(inf->Attribute & EFI_FILE_DIRECTORY)
                {
                        if(inf->FileName[0] == '.') { continue;}
                        size namesz = _STRLEN(&inf->FileName);
                        curropt->kernal_name = AllocatePool(sizeof(CHAR16)*(namesz+1));
                        ST->BootServices->CopyMem(curropt->kernal_name,&inf->FileName,(namesz+1)*sizeof(CHAR16));
                        curropt->prev = last_opt;
                        if(last_opt) { last_opt->next = curropt;}
                        last_opt = curropt;
                }

        }
        FreePool(inf);
}



int GUID_EQU(EFI_GUID* a,EFI_GUID* b) {
        if(a->Data1 != b->Data1) { return 0;}
        if(a->Data2 != b->Data2) { return 0;}
        if(a->Data3 != b->Data3) { return 0;}
        for(int i = 0; i < 8; i++) {
                if(a->Data4[i] != b->Data4[i]) { return 0;}
        }
        return 1;
}

#define ACPI_TABLE_GUID { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } };
#define ACPI_20_TABLE_GUID { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } };

void FindRSDP() {
        EFI_GUID rsdp_20_id = ACPI_20_TABLE_GUID;
        for(index i = 0; i < ST->NumberOfTableEntries; i++) {
                if(GUID_EQU(&ST->ConfigurationTable[i].VendorGuid,&rsdp_20_id)) { 
                        GSD_BOOT_TABLE->rsdp = ST->ConfigurationTable[i].VendorTable;
                        return;
                }
        }
        EFI_GUID rsdp_id = ACPI_TABLE_GUID;
        for(index i = 0; i < ST->NumberOfTableEntries; i++) {
                if(GUID_EQU(&ST->ConfigurationTable[i].VendorGuid,&rsdp_id)) { 
                        GSD_BOOT_TABLE->rsdp = ST->ConfigurationTable[i].VendorTable;
                        return;
                }
        }
        GSD_BOOT_TABLE->rsdp = NULL;
}

EFI_STATUS efi_main(EFI_HANDLE imghandle,EFI_SYSTEM_TABLE* tab) {
        ST = tab;
        IMGH = imghandle;
        BOOT_OPT_COUNT  = 0;
        BOOT_OPT_SELECTED = 0;
        root_opt = NULL;
        KCR3 = NULL;
        GSD_BOOT_TABLE = NULL;
        GSD_PAGES = NULL;
        efimmap.BASE = NULL;
        efimmap.MAPSZ = 0;
        InitGsdPages();
        InitPageTable();
        GetImage();
        GetVol();
        InitBootTable();
        FindRSDP();
        MarkLoader();
        OpenGsdbDirectory();
        LoadBootOptions();
        Menu();
        return EFI_ABORTED;
}