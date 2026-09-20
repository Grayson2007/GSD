section .boot2
pml4: equ 0xD000
pml3: equ 0xE000
pml2: equ 0xF000
bits 16
; Boot2 Grabs Necessarry Information Then transitions into long mode 
org 0x7E00
boot2_init:








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
lmrestore:


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


section .bss
align 256
vesa_info_block:
        resb 256
memory_descriptor_number:
        resd 1
memory_descriptor:
        resb 24