#include "bootefi.h"
EFI_SYSTEM_TABLE* ST;
EFI_HANDLE IMGH;
x86_64_BootTable* BT;

#define ui_OPTIONS_PER_SCREEN 8


static struct {
        EFI_FILE_INFO** arr;
        size count;
        size capacity;
        index selected;
} boot_options;


size efi_STRLEN(CHAR16* str) {
        size out = 0;
        while(*str++ ) { out++;}
        return out;
}
NORETURN void efi_PANIC(CHAR16* msg,EFI_STATUS code) {
        ST->ConOut->ClearScreen(ST->ConOut);
        efi_println(u"gsdboot has panicked!");
        efi_print(msg);
        efi_print(u"code: ");
        efi_printu64((u64)code,10);
        efi_println(u" ");
        efi_println(u"press any key to reboot system");
        EFI_STATUS    Status;
        UINTN         Index;
        EFI_INPUT_KEY Key;
        ST->BootServices->WaitForEvent(1,&ST->ConIn->WaitForKey,&Index);
        ST->RuntimeServices->ResetSystem(EfiResetCold,code,0,NULL);
        efi_println(u"If your system did not reboot");
        efi_println( u"force shut it down by holding down the power button");
        for(;;); 
}

void efi_Cleanup() {
        for(index i = 0; i < boot_options.count; i++) {
                efi_FreePool(boot_options.arr[i]);
        }
        efi_FreePool(boot_options.arr); // Free Up Bullshit 


        GSDBOOTDIR->Close(GSDBOOTDIR);
}



NORETURN void efi_STALL_AutoReset(CHAR16* msg,EFI_STATUS code) {
     
        UINTN _msgsz = efi_STRLEN(msg)*sizeof(CHAR16);
        ST->RuntimeServices->ResetSystem(EfiResetCold,code,_msgsz,(void*)msg);
        for(;;);
}
void efi_println(CHAR16* str) {
        ST->ConOut->OutputString(ST->ConOut,str);
        ST->ConOut->SetCursorPosition(ST->ConOut,0,ST->ConOut->Mode->CursorRow+1);
}
void efi_print(CHAR16* str) {
        ST->ConOut->OutputString(ST->ConOut,str);
}

void efi_CarrageReturn() {
        ST->ConOut->SetCursorPosition(ST->ConOut,0,ST->ConOut->Mode->CursorRow);
}

void efi_ClearScreen() {
        ST->ConOut->ClearScreen(ST->ConOut);
        ST->ConOut->SetCursorPosition(ST->ConOut,0,0);
}
CHAR16* didgits = u"0123456789ABCDEF";
void efi_printu64(u64 x,int base) {
        if(base > 16) { return;}
        if(!x) { efi_print(u"0"); }
        CHAR16 buffer[21];
        index len = 1;

        while(x > 0) {
                for(index i = len; i > 0; i--) {
                        buffer[i] = buffer[i-1];  
                }
                buffer[0] = didgits[x % base];
                x /= base;
                len++;
        }       
        buffer[len] = u'\0';
        efi_print(&buffer);

}
// My favorite boot service is ExitBootServices so we can say fuck you to this
// buggy as shit  overcomplicated NT'ified insult that somehow barely works
// Dont forget UEFIcanhazbufferoverflow you filthy Bastards 


// gsdb/default/ [kernel.elf irfs.img] 


EFI_STATUS ScanKernels() {
        boot_options.capacity = 8;
        boot_options.count = 0;
        boot_options.selected = 0;
        ST->BootServices->AllocatePool(EfiLoaderData,sizeof(EFI_FILE_INFO*)*8,&boot_options.arr); 
        GSDBOOTDIR->SetPosition(GSDBOOTDIR,0);
        while(1) 
        {
                // EFI_FILE_INFO
                EFI_FILE_INFO* inf;
                UINTN infsize = 0;
                EFI_STATUS s;
        
                s = GSDBOOTDIR->Read(GSDBOOTDIR,&infsize,NULL);
                if(EFI_ERROR(s) && s != EFI_BUFFER_TOO_SMALL) { efi_PANIC(u"Unable to scan kernels",s);}
                if(!infsize) { return EFI_SUCCESS;}
                efi_AllocPool(infsize,&inf);
                s = GSDBOOTDIR->Read(GSDBOOTDIR,&infsize,inf);
                if(EFI_ERROR(s)) {
                        efi_FreePool(inf);
                        return s;
                }
                if(!(inf->Attribute & EFI_FILE_DIRECTORY)) { 
                        efi_FreePool(inf);
                        continue;
                }
                if(inf->FileName[0] == u'.') { efi_FreePool(inf); continue;}
                boot_options.count += 1;
                if(boot_options.count > boot_options.capacity) {
                        size oldcap = boot_options.capacity;
                        boot_options.capacity += 8;
                        EFI_FILE_INFO** old_array = boot_options.arr;
                        EFI_FILE_INFO** new_array;
                        efi_AllocPool(boot_options.capacity*sizeof(EFI_FILE_INFO*),(void**)&new_array);
                        ST->BootServices->CopyMem((void*)new_array,(void*)old_array,sizeof(EFI_FILE_INFO*)*oldcap);
                        efi_FreePool(old_array);
                        boot_options.arr = new_array;
                }
                boot_options.arr[boot_options.count-1] = inf;
              

        }
}


static void LoadKernelElf(Elf64_Ehdr* elf_base,uintptr_t* out_entry_point) {
        // Verify the elf 
        u32* elf_signature = (u32*)(&elf_base->e_ident[0]);
        if(*elf_signature != '\x7FELF') { 
                efi_PANIC(u"Bad kernel binary detected",EFI_ABORTED);
        }
        if(elf_base->e_ident[EI_CLASS]!= ELFCLASS64) {
                efi_PANIC(u"Wrong kernel binary bitwidth",EFI_ABORTED);
        }
        if(elf_base->e_machine != EM_X86_64) {
                efi_PANIC(u"Kernel binary is not for x86_64",EFI_ABORTED);
        }
        if(elf_base->e_type != ET_EXEC) {
                efi_PANIC(u"Kernel binary is not Exec",EFI_ABORTED);
        }
        // If all these pass this is a valid binary
        // Load the kernel 
        size num_phdr = elf_base->e_phnum;
        uintptr_t phdr_base = (uintptr_t)(((uintptr_t)elf_base)+elf_base->e_phoff);
        size phdr_size = elf_base->e_phentsize;
        uintptr_t phdr_pointer = phdr_base;
        index phdr_index = 0;
        EFI_STATUS s;
        while(phdr_index < num_phdr)
        {
                Elf64_Phdr* phdr = (Elf64_Phdr*)phdr_pointer;
                if(phdr->p_type == PT_LOAD)
                {
                        u64 flags = phdr->p_flags & PF_X ? gsdpg_kcode_attrib : gsdpg_kpage_attrib;
                        uintptr_t vbase_address = phdr->p_vaddr;
                        size copysz = phdr->p_filesz;
                        size fullsz = phdr->p_memsz;
                        size zero_size = fullsz-copysz;
                        size num_pages = ((fullsz/EFI_PAGE_SIZE)+((fullsz % EFI_PAGE_SIZE) ? 1 : 0));
                        EFI_PHYSICAL_ADDRESS phys;
                        s = AllocateKernelPages(&phys,num_pages,flags);
                        if(EFI_ERROR(s)) {
                                efi_PANIC(u"Memory issue ocurred when loading kernel binary",s);
                        }
                        uintptr_t copysrc = ((uintptr_t)elf_base)+phdr->p_offset;
                        ST->BootServices->CopyMem((void*)phys,(void*)copysrc,copysz);
                        ST->BootServices->SetMem((void*)(phys+copysz),zero_size,0);
                        efi_KCR3_nmap((EFI_VIRTUAL_ADDRESS)vbase_address,phys,num_pages);
                }
                phdr_pointer += phdr_size;
        }
        *out_entry_point = (uintptr_t)elf_base->e_entry;



}
// Idenity Map the Runtime Services 
void ImapRuntimeServices() {
        uintptr_t mmap_cptr = (uintptr_t)EFIMMAP.BASE;
        uintptr_t mmap_eptr = (uintptr_t)mmap_cptr+EFIMMAP.MAPSZ;

        while(mmap_cptr < mmap_eptr) {
                EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)mmap_cptr;
                if(desc->Type == EfiRuntimeServicesCode || desc->Type == EfiRuntimeServicesData)
                {
                        efi_KCR3_nmap((EFI_VIRTUAL_ADDRESS)desc->PhysicalStart,desc->PhysicalStart,desc->NumberOfPages);
                }
                mmap_cptr += EFIMMAP.DESC_SZ;
        }

}
// Idenity Map the Bootloader Space
void ImapLoader() {
        uintptr_t loader_base = (uintptr_t)GSDBOOT_IMG->ImageBase;
        size loader_sz = (size)GSDBOOT_IMG->ImageSize;
        size loader_pgc = ((loader_sz/EFI_PAGE_SIZE) + ((loader_sz % EFI_PAGE_SIZE) ? 1 : 0));
        EFI_STATUS s = efi_KCR3_nmap((EFI_VIRTUAL_ADDRESS)loader_base,(EFI_PHYSICAL_ADDRESS)loader_base,loader_pgc);
        if(EFI_ERROR(s)) { efi_PANIC(u"Failed to map loader",s); }

}

static int CheckGUID(EFI_GUID* a,EFI_GUID* b) {
        if(a->Data1 != b->Data1) { return 0;}
        if(a->Data2 != b->Data2) { return 0;}
        if(a->Data3 != b->Data3) { return 0;}
        for(int i = 0; i < 8; i++) {
                if(a->Data4[i] != b->Data4[i]) { return 0;}
        }
        return 1;
}
void FindRsdp() {
        size tablec = ST->NumberOfTableEntries;
        EFI_GUID RSDPID = EFI_ACPI_20_TABLE_GUID;
        for(index i = 0; i < tablec; i++) {
                if(CheckGUID(&RSDPID,&ST->ConfigurationTable[i].VendorGuid)) {
                        BT->rsdp = ST->ConfigurationTable[i].VendorTable;
                        return;
                }
        }
        efi_PANIC(u"RSDP CANNOT BE FOUND",EFI_ABORTED);
}

void LoadKernel(uintptr_t* kernel_entry_point) {
        EFI_FILE_HANDLE dir;
        EFI_STATUS s = GSDBOOTDIR->Open(GSDBOOTDIR,&dir,&boot_options.arr[boot_options.selected]->FileName,EFI_FILE_MODE_READ,EFI_FILE_DIRECTORY | EFI_FILE_SYSTEM);
        if(EFI_ERROR(s)) { efi_PANIC(u"Unable to open kernel directory",s);}
        EFI_FILE_HANDLE f_kernel_elf;
        EFI_FILE_HANDLE f_irfs_img;
        s = dir->Open(dir,&f_kernel_elf,u"kernel.elf",EFI_FILE_MODE_READ,EFI_FILE_SYSTEM | EFI_FILE_HIDDEN);
        if(EFI_ERROR(s)) { 
                efi_PANIC(u"failed to open kernel.elf",s);
        }
        s = dir->Open(dir,&f_irfs_img,u"irfs.img",EFI_FILE_MODE_READ,EFI_FILE_SYSTEM | EFI_FILE_HIDDEN);
        if(EFI_ERROR(s)) {
                efi_PANIC(u"failed to open irgs.img",s);
        }

        EFI_FILE_INFO* kernel_info;
        EFI_FILE_INFO* irfs_info;
        s = efi_Finfo(f_kernel_elf,&kernel_info);
        if(EFI_ERROR(s)) {
                efi_PANIC(u"Unable to get kernel info",s);
        }
        s = efi_Finfo(f_irfs_img,&irfs_info);
        if(EFI_ERROR(s)) {
                efi_PANIC(u"Unable to get irfs info",s);
        }
        size kernel_size = kernel_info->FileSize;
        size irfs_size = irfs_info->FileSize;
        efi_FreePool(kernel_info);
        efi_FreePool(irfs_info);

        size kernel_pages = ((kernel_size/EFI_PAGE_SIZE) + ((kernel_size % EFI_PAGE_SIZE) ? 1 : 0));
        size irfs_pages = ((irfs_size/EFI_PAGE_SIZE) + ((irfs_size % EFI_PAGE_SIZE) ? 1 : 0));
        // Start with the kernel elf 
        EFI_PHYSICAL_ADDRESS irfs_img_base;
        EFI_PHYSICAL_ADDRESS kernel_elf_base;
        s = AllocateKernelPages(&kernel_elf_base,kernel_pages,gsdpg_kpage_attrib);
        if(EFI_ERROR(s)) { efi_PANIC(u"unable to allocate memory for kernel elf",s);}
        // Load the kernel elf
        s= f_kernel_elf->Read(f_kernel_elf,(UINTN*)&kernel_size,(void*)kernel_elf_base);
        LoadKernelElf((Elf64_Ehdr*)kernel_elf_base,kernel_entry_point);
        s = AllocateKernelPages(&irfs_img_base,irfs_pages,gsdpg_kpage_attrib);
        if(EFI_ERROR(s)) { efi_PANIC(u"unable to allocate memory for irfs_img",s );}
        s = f_irfs_img->Read(f_irfs_img,&irfs_size,(void*)irfs_img_base);
        if(EFI_ERROR(s)) { efi_PANIC(u"irfs read failed",s);}
        
        BT->irfs.pagec = irfs_pages;
        BT->irfs.pbase = irfs_img_base;
        BT->irfs.sz = irfs_size;

        BT->kernel_bin_info.pbase = kernel_elf_base;
        BT->kernel_bin_info.ksize = kernel_size;
        BT->kernel_bin_info.kpagec = kernel_pages;

        // Idenity map irfs and kernel elfr
        s = efi_KCR3_nmap((EFI_VIRTUAL_ADDRESS)irfs_img_base,irfs_img_base,irfs_pages);
        if(EFI_ERROR(s)) { efi_PANIC(u"Unable to map irfs",s);}
        s = efi_KCR3_nmap((EFI_VIRTUAL_ADDRESS)kernel_elf_base,kernel_elf_base,kernel_pages);
        if(EFI_ERROR(s)) { efi_PANIC(u"Unable to map kernel",s);}
        f_irfs_img->Close(f_irfs_img);
        f_kernel_elf->Close(f_kernel_elf);
}

static void InitaliseGop() {
        EFI_GUID gop_id = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
        EFI_STATUS s = ST->BootServices->LocateProtocol(&gop_id,NULL,&gop);
        if(EFI_ERROR(s)) {
                efi_PANIC(u"Gop is unavalible",s);
        }
        UINTN native_mode = gop->Mode->Mode;
        UINTN max_mode = gop->Mode->MaxMode;
        gop->SetMode(gop,native_mode); // Reset to the native mode 
        
        UINTN best_mode = 0;
        UINTN best_res = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* inf;
        UINTN infsz = 0;
        for(index i = 0; i < max_mode; i++) {
                gop->QueryMode(gop,i,&infsz,&inf);
                if(inf->PixelFormat != PixelRedGreenBlueReserved8BitPerColor) { continue;}
                index res = (inf->HorizontalResolution*inf->VerticalResolution);
                if(res > best_res) {
                        best_mode = i;
                }
        }
        s = gop->SetMode(gop,best_mode);
        if(EFI_ERROR(s)) { 
                efi_STALL_AutoReset(u"Gop Mode change error",s);
        }
        BT->fb.pbase = (void*)gop->Mode->FrameBufferBase;
        BT->fb.fbsz = gop->Mode->FrameBufferSize;
        BT->fb.depth = 32;
        BT->fb.gmask = gop->Mode->Info->PixelInformation.GreenMask;
        BT->fb.bmask = gop->Mode->Info->PixelInformation.BlueMask;
        BT->fb.rmask = gop->Mode->Info->PixelInformation.RedMask;
        BT->fb.ppsl = gop->Mode->Info->PixelsPerScanLine;
        BT->fb.pitch = 4*gop->Mode->Info->PixelsPerScanLine;
        BT->fb.w = gop->Mode->Info->HorizontalResolution;
        BT->fb.h = gop->Mode->Info->VerticalResolution;
}






void Menu_Redraw() {
        efi_ClearScreen();
        efi_println(u"  Select Boot Option  ");
        efi_println(u"------------------------");

        index pagestart = (boot_options.selected - (boot_options.selected % ui_OPTIONS_PER_SCREEN));
        index pageend = pagestart + ui_OPTIONS_PER_SCREEN;
        for(index i = pagestart; i < pageend; i++)
        {
                if(i >= boot_options.count) { break;}
                if(i == boot_options.selected) {
                        efi_print(u" >> ");
                } else { efi_print(u" ");}
                efi_println(&boot_options.arr[i]->FileName);
        }
        efi_println(u"------------------------");
        efi_println(u" Use UP/DOWN arrow keys to select.");
        efi_println(u" Press enter to boot kernel");
}
NORETURN void ToKernel(uintptr_t entry_point) {
        efi_FlushPmm(); // Flush the Final state of the PMM to the mem_map 
        efi_UpdateMemoryMap(); // Update the memory map 
        BT->mmap.efi.base = EFIMMAP.BASE;
        BT->mmap.efi.desc_sz = EFIMMAP.DESC_SZ;
        BT->mmap.efi.map_sz = EFIMMAP.MAPSZ;
        BT->mmap.efi.ver = EFIMMAP.VER;
        ST->BootServices->ExitBootServices(IMGH,EFIMMAP.KEY);
        ktramp((UINTN)KCR3,(UINTN)entry_point);
        for(;;);
}

static void StartBootProcess() {
        uintptr_t kernel_entry;
        LoadKernel(&kernel_entry);
        FindRsdp();
        ImapLoader();
        ImapRuntimeServices();
        GSDBOOTDIR->Close(GSDBOOTDIR);
        ROOTDIR->Close(ROOTDIR);
        InitaliseGop();
        ToKernel(kernel_entry);
        for(;;);
}

void Menu() {
        Menu_Redraw();
        EFI_STATUS    s;
        EFI_INPUT_KEY Key;
        while(1) {
                while ((s = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
                if(EFI_ERROR(s)) { efi_PANIC(u"Key input has panicked",s);}
                if(Key.ScanCode == SCAN_UP) {
                        if(boot_options.selected) {
                                boot_options.selected--;
                        } else { boot_options.selected = boot_options.count-1;}
                        Menu_Redraw();
                }
                else if(Key.ScanCode == SCAN_DOWN) 
                {
                        if(boot_options.selected < (boot_options.count-1))
                        {
                                boot_options.selected++;
                        }
                        else  { boot_options.selected = 0;}
                        Menu_Redraw();
                }
                else if(Key.UnicodeChar == u'\n' || Key.UnicodeChar == u'\r')
                {
                        efi_ClearScreen();
                        StartBootProcess();
                }
        }

}




EFI_STATUS efi_main(EFI_HANDLE _imgh,EFI_SYSTEM_TABLE* _st) {
        ST = _st;
        IMGH = _imgh;
        EFIMMAP.BASE = NULL;
        EFIMMAP.MAPSZ = 0;
        efi_println(u"Welcome to GSDBOOT!");
        efi_println(u"Doing Initial Setup please wait...");
        EFI_STATUS s;
        s = efi_SetupFs();
        if(EFI_ERROR(s)) {
                efi_PANIC(u"Fs Setup Failed",s);
        }
       
        efi_print(u"DBG: Image Base: 0x");
        efi_printu64((u64)GSDBOOT_IMG->ImageBase,16);


        Pmm_Init();
        efi_println(u"Setting up Boot table");
        s = AllocateKernelPages(&BT,1,gsdpg_kpage_attrib);
        if(EFI_ERROR(s)) { efi_PANIC(u"Boot Table setup Failed",s); }
        s = efi_KCR3_map((EFI_VIRTUAL_ADDRESS)(BOOT_TABLE_ADDRESS),(EFI_PHYSICAL_ADDRESS)BT);
        if(EFI_ERROR(s)) { efi_PANIC(u"Boot Table mapping Failed",s); }
        
        BT->mode = bm_efi;
        BT->signature = boottab_signature;
        BT->max_physical_address = MAX_PHYSICAL_ADDRESS;
        ST->BootServices->CopyMem((void*)&BT->efirt,(void*)ST->RuntimeServices,sizeof(EFI_RUNTIME_SERVICES)); 
        BT->loader.base = GSDBOOT_IMG->ImageBase;
        BT->loader.pagec = ((GSDBOOT_IMG->ImageSize/EFI_PAGE_SIZE)+((GSDBOOT_IMG->ImageSize % EFI_PAGE_SIZE) ? 1 : 0));
        efi_println(u"Boot table setup sucessfully");
        efi_println(u"Scanning for kernels...");
        ScanKernels();
        Menu();
        for(;;);
}


