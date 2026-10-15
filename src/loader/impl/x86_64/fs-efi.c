#include "bootefi.h"
EFI_LOADED_IMAGE* GSDBOOT_IMG;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* IOVOL;
EFI_FILE_PROTOCOL* ROOTDIR;
EFI_FILE_HANDLE GSDBOOTDIR;
EFI_FILE_HANDLE OPTDIR;


EFI_STATUS efi_SetupFs() {
        EFI_STATUS s;
        EFI_GUID lim_id = EFI_LOADED_IMAGE_PROTOCOL_GUID;
        s = ST->BootServices->HandleProtocol(IMGH,&lim_id,&GSDBOOT_IMG);
        if(EFI_ERROR(s)) { return s;}
        EFI_GUID sfs_id = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
        s = ST->BootServices->HandleProtocol(GSDBOOT_IMG->DeviceHandle,&sfs_id,&IOVOL);
        if(EFI_ERROR(s)) { return s;}
        s = IOVOL->OpenVolume(IOVOL,&ROOTDIR);
        if(EFI_ERROR(s)) { return s;}
        s = ROOTDIR->Open(ROOTDIR,&GSDBOOTDIR,u"gsdboot",EFI_FILE_MODE_READ,EFI_FILE_DIRECTORY | EFI_FILE_SYSTEM | EFI_FILE_HIDDEN);
        if(EFI_ERROR(s)) { efi_PANIC(u"gsdboot directory not found",s); }
        return EFI_SUCCESS;
}
EFI_STATUS efi_Finfo(EFI_FILE_HANDLE f,EFI_FILE_INFO** outinfo) {
        UINTN sz = 0;
        void* buffer = NULL;
        EFI_GUID info_id = EFI_FILE_INFO_ID;
        EFI_STATUS s = f->GetInfo(f,&info_id,&sz,NULL);
        if(s != EFI_BUFFER_TOO_SMALL) { return s;}
        while(1) {
                s = efi_AllocPool(sz,&buffer);
                if(EFI_ERROR(s)) { return s;}
                s = f->GetInfo(f,&info_id,&sz,buffer);
                if(s == EFI_BUFFER_TOO_SMALL) {
                        efi_FreePool(buffer);
                        sz += 32;
                        continue;
                }
                if(EFI_ERROR(s)) {
                        efi_FreePool(buffer);
                        return s;
                }
                break;
        }
        *outinfo = (EFI_FILE_INFO*)buffer;
        return EFI_SUCCESS;
}



