bits 16
; Bootstrap real mode code to enter long mode for Application Cores 
; (Run this on )
section .realmbs

global _transition_cr3 
global _transition_stack_top
global _transition_stack_bottom
; Reserved Memory
_transition_cr3 equ 0x2000 ; Contains page table mappings & a idenity page of this bootstrap and initstack 
_transition_stack_top equ 0x3000
_transition_stack_bottom equ 0x4000

ap_start: equ 0x1000

        ; Enter long mode.
    mov eax, 10100000b                ; Set the PAE and PGE bit.
    mov cr4, eax
      
    mov edx, _transition_cr3          ; Point CR3 at the PML4.
    mov cr3, edx
      
    mov ecx, 0xC0000080               ; Read from the EFER MSR. 
    rdmsr    

    or eax, 0x00000100                ; Set the LME bit.
    wrmsr
      
    mov ebx, cr0                      ; Activate long mode -
    or ebx,0x80000001                 ; - by enabling paging and protection simultaneously.
    mov cr0, ebx                    

    cli
    lgdt [_transition_GDT.Pointer]                ; Load GDT.Pointer defined below.
      
    jmp 0x8:LM_INIT 
bits  64
LM_INIT: 
        mov ax,0x8
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        mov rax,application_core_jmp_vector
        mov rsp,_transition_stack_bottom
        mov rbp,_transition_stack_top
        jmp [rax]
        

; GDT is taken from Osdev.org: wiki.osdev.org/Entering_Long_Mode_Directly
_transition_GDT: 
        .Null:
                dq 0x0000000000000000             ; Null Descriptor - should be present.

        .Code:
                dq 0x00209A0000000000             ; 64-bit code descriptor (exec/read).
                dq 0x0000920000000000             ; 64-bit data descriptor (read/write). 
align 4
        dw 0                              ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary
        .Pointer:
        dw $ - _transition_GDT - 1                    ; 16-bit Size (Limit) of GDT.
        dd _transition_GDT                           ; 32-bit Base Address of GDT. (CPU will zero extend to 64-bit)
application_core_jmp_vector:
        dq 0
times 4096-($-$$) db 0 
; 0x9000-0xA000 = Stack 
