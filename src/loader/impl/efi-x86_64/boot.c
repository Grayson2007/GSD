#include <efi-headers/Include/Uefi.h>
#include <efi-headers/Include/Protocol/LoadedImage.h>
#include <efi-headers/Include/Protocol/GraphicsOutput.h>
#include <gsd-memory.h>
#include <impl/x86_64/x86_64-boot-table.h>
#include <impl/x86_64/vmmap.h>

static EFI_SYSTEM_TABLE* ST;
static EFI_HANDLE IMGH;
static UINT64* KCR3;
static gsdpage* GSD_PAGES;
static struct {
        EFI_MEMORY_DESCRIPTOR* BASE;
        UINTN DESC_SZ;
        UINTN KEY;
        UINTN MAPSZ;
        UINT32 VER;
} efimmap;
void UpdateMemoryMap() {
        EFI_MEMORY_DESCRIPTOR* oldbase = efimmap.BASE;
        UINTN oldsize = efimmap.MAPSZ;

}

NORETURN void panic(CHAR16* msg) {
        ST->ConOut->ClearScreen(ST->ConOut);
        ST->ConOut->SetCursorPosition(ST->ConOut,0,0);
        ST->ConOut->OutputString(ST->ConOut,msg);
        for(;;);
}

void InitGsdPages() {
        efimmap.MAPSZ = 0;
        efimmap.BASE = NULL;

        EFI_STATUS s = ST->BootServices->GetMemoryMap(&efimmap.MAPSZ,NULL,&efimmap.KEY,&efimmap.DESC_SZ,&efimmap.VER);
        if(s != EFI_BUFFER_TOO_SMALL) {
                panic(u"PANIC: Failed to query memory map");
        }
        efimmap.MAPSZ += 2*efimmap.DESC_SZ; 

        size mmap_pages_needed = (efimmap.MAPSZ >> 12) + ((efimmap.MAPSZ & 0xFFF) ? 1 : 0);
        ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,mmap_pages_needed,&efimmap.BASE);
        ST->BootServices->GetMemoryMap(&efimmap.MAPSZ,&efimmap.BASE,&efimmap.KEY,&efimmap.DESC_SZ,&efimmap.VER);


        // Query Total Pages 
        size total_pages = 0;
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
        ST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,gsd_page_pages_needed,&GSD_PAGES);
        desc_ptr = (void*)efimmap.BASE;
        // Initalise Memory Map 

}

void allocate_new_page() {

}

void mark_page(EFI_VIRTUAL_ADDRESS va,EFI_PHYSICAL_ADDRESS pa) {
        UINTN l3idx = va >> 39 & 0x1FF;
        UINTN l2idx = va >> 30 & 0x1FF;
        UINTN l1idx = va >> 21 & 0x1FF;
        UINTN pgidx = va >> 12 & 0x1FF;

        if(!KCR3[l3idx] & 1) {

        }
}


EFI_STATUS efi_main(EFI_HANDLE imghandle,EFI_SYSTEM_TABLE* tab) {
        ST = tab;
        IMGH = imghandle;
        
}
