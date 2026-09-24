section .text
global ktramp
; NORETURN void ktramp(UINTPTR cr3,UINTPTR rip)

; rcx = cr3
; rdx = GDTR
; r8 = rip 

ktramp:
        mov cr3,rcx
        cli
        mov rsp,boot_stack_top
        mov rbp,boot_stack
        mov rax,gdt_pointer
        lgdt [rax]
        mov rax,0x8
        push rax
        lea rax,[rel reload_cs]
        push rax
        lretq
reload_cs:
        mov ax,0x10
        mov ds,ax
        mov es,ax
        mov fs,ax
        mov gs,ax
        mov ss,ax
        jmp r8 
section .rodata
; Generic GDT for the kernel 
align 4
gdt_start:
    ; 1. Null Descriptor (64 bits of zeros)
    dq 0x0000000000000000             

    ; 2. Kernel Code Descriptor (Index 1 -> Selector 0x08)
    ; Flags: Present(1), Ring 0(00), System(1), Executable(1), Readable(1) -> 0x9A
    ; Granularity: Long Mode flag (L=1) -> 0x20
    dq 0x00209A0000000000             

    ; 3. Kernel Data Descriptor (Index 2 -> Selector 0x10)
    ; Flags: Present(1), Ring 0(00), System(1), Writable(1) -> 0x92
    dq 0x0000920000000000             
gdt_end:

; GDT Pointer descriptor for the 'lgdt' instruction
align 4
gdt_pointer:
    dw gdt_end - gdt_start - 1        ; Size of GDT minus 1
    dq gdt_start                      ; Linear address of GDT

section .bss
align 4096
; Small 32K stack just to kickstart everything 
; Entire Bootloader will be idenity mapped 
; Once the Kernel has what it need these pages will be freed up 
boot_stack:
        resb 32768
boot_stack_top:

