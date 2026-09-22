section .text
pml4: equ 0xD000
pml3: equ 0xE000
pml2: equ 0xF000
bits 16
; Boot2 Grabs Necessarry Information Then transitions into long mode 6
org 0x7E00
boot2_init: 
        call setup_page_tables
        ; Disable IRQs
return_to_long_mode:
        mov al, 0xFF                      ; Out 0xFF to 0xA1 and 0x21 to disable all IRQs.
        out 0xA1, al
        out 0x21, al
        lidt [dummy_lm_idt]
        ; Enter long mode.
        mov eax, 10100000b                ; Set the PAE and PGE bit.
        mov cr4, eax
        
        mov edx, edi                      ; Point CR3 at the PML4.
        mov cr3, edx
        
        mov ecx, 0xC0000080               ; Read from the EFER MSR. 
        rdmsr    

        or eax, 0x00000100                ; Set the LME bit.
        wrmsr
        
        mov ebx, cr0                      ; Activate long mode -
        or ebx,0x80000001                 ; - by enabling paging and protection simultaneously.
        mov cr0, ebx             
        cli
        lgdt [gdt64.pointer]     
        jmp 0x8:lm_init 
setup_page_tables: ; idenity map the first 4MB (For Loading Data Etc...)
        mov eax,0x11003
        mov [pml4],eax
        mov eax,pml2
        mov [pml3],eax
        mov eax,0x83
        mov [pml2],eax
        mov eax,0x200083
        mov [pml2+8],eax
        ret
bits 64
lm_init:
        hlt
        jmp lm_init
to_real_mode:
        
section .data
gdt16:
        .null: dq 0
        .code:
                dw 0xFFFF
                dw 0
                db 0
                db 0x9A 
                db 0
                db 0
        .data:
                dw 0xFFFF
                dw 0
                db 0
                db 0x92
                db 0
                db 0
        .pointer:
                dw $-gdt16-1
                dd gdt16
gdt64:
        .null: dq 0,0
        .code: 
                dd 0
                db 0
                db 0x9A
                db 0x20
                db 0
                dq 0
        .data:
                dd 0
                db 0
                db 0x92
                db 0x20
                db 0
                dq 0
        .pointer:
                dw $-gdt64-1
                dq gdt64

dummy_lm_idt:
        dw 0
        dq 0
section .bss
align 256
vesa_info_block:
        resb 256
memory_descriptor_number:
        resd 1
memory_descriptor:
        resb 24