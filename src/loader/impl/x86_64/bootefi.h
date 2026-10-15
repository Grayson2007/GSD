#ifndef bootefi_h
#define bootefi_h
#include <efi-headers/Include/Uefi.h>
#include <efi-headers/Include/Protocol/LoadedImage.h>
#include <efi-headers/Include/Protocol/GraphicsOutput.h>
#include <efi-headers/Include/Guid/FileInfo.h>
#include <efi-headers/Include/Protocol/SimpleFileSystem.h>
#include <efi-headers/Include/Guid/Acpi.h>
#include <gsd-memory.h>
#include <impl/x86_64/x86_64-boot-table.h>
#include <impl/x86_64/vmmap.h>
#include <impl/x86_64/pagingx64.h>
#include <elf.h>

extern EFI_SYSTEM_TABLE* ST;
extern EFI_HANDLE IMGH;
extern UINT64* KCR3;
extern gsdpage* GSD_PAGES;
extern uintptr_t MAX_PHYSICAL_ADDRESS;
extern size GSD_PAGES_ARRAY_SIZE;
extern EFI_LOADED_IMAGE* GSDBOOT_IMG;
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* IOVOL;
extern EFI_FILE_PROTOCOL* ROOTDIR;
extern EFI_FILE_HANDLE GSDBOOTDIR;
extern EFI_FILE_HANDLE OPTDIR;
extern struct s_gsdboot_memblock_table* ROOT_TABLE;  
extern int BOOT_OPT_COUNT;
extern int BOOT_OPT_SELECTED;
#define OPTIONS_PER_PAGE 4
extern NORETURN void ktramp(UINTN cr3,UINTN rip);

struct s_efimmap{
        EFI_MEMORY_DESCRIPTOR* BASE;
        UINTN DESC_SZ;
        UINTN KEY;
        UINTN MAPSZ;
        UINT32 VER;
};
extern struct s_efimmap EFIMMAP;

// helpers
void efi_printu64(u64 x,int base);
void efi_print(CHAR16* str);
size efi_STRLEN(CHAR16* str);
NORETURN void efi_PANIC(CHAR16* msg,EFI_STATUS code);
NORETURN void efi_STALL_PANIC(CHAR16* msg); 
void efi_Clean();
void efi_CarrageReturn();
void efi_ClearScreen();
// Memory
void efi_UpdateMemoryMap();
EFI_STATUS efi_AllocPool(size poolsz,void** addr);
void efi_FreePool(void* addr);
EFI_STATUS efi_AllocAnyPages(size count,EFI_PHYSICAL_ADDRESS* outptr);
EFI_STATUS efi_FreePages(EFI_PHYSICAL_ADDRESS addr,size count);
void Pmm_Init();
EFI_STATUS AllocateKernelPages(EFI_PHYSICAL_ADDRESS* outphys,size n,u64 attrib); // Allocates a physical block and marks it 
EFI_STATUS efi_KCR3_nmap(EFI_VIRTUAL_ADDRESS virt,EFI_PHYSICAL_ADDRESS phys,size n);
EFI_STATUS efi_KCR3_map(EFI_VIRTUAL_ADDRESS virt,EFI_PHYSICAL_ADDRESS phys);
void efi_FlushPmm();
// Allocate Kernel Memory Regions 
// Files 
EFI_STATUS efi_SetupFs();
EFI_STATUS efi_Finfo(EFI_FILE_HANDLE f,EFI_FILE_INFO** outinfo);
// Kernel Setups 

void SetupBootTable();
// Additional Setups
void efi_SetupGop();
// Boot Routines
NORETURN void Boot();
void efi_clear();
void efi_println(CHAR16* str);

#endif