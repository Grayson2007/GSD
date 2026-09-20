org 0x7C00    ; Sets the starting offset for bootloaders
section .mbr
bits 16
global pml4
global pml3
global pml2
global gdt16
global gdt64

global _boot
_boot:
        cli ; disable irq's
        mov sp,0x7000
        mov bp,0x1000
        ; setup stack
        call setup_page_tables
a20_line: 
        call a20_test
        jc .bios
        clc        
        ret
        .bios:
                

        .fast:


        .manual:


        .failed:


; (Taken from: wiki.osdev.org/A20_Line)
a20_test:
    pushf
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax ; ax = 0
    mov es, ax

    not ax ; ax = 0xFFFF
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax

    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al

    pop ax
    mov byte [es:di], al

    mov ax, 0
    je check_a20__exit

    mov ax, 1

check_a20__exit:
    pop si
    pop di
    pop es
    pop ds
    popf

    ret



times 446 - ($-$$) db 0
dw 0xAA55

