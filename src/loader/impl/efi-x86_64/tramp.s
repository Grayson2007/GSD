section .text
global ktramp
; NORETURN void ktramp(UINTPTR cr3,UINTPTR rbp, UINTPTR rsp,UINTPTR rip)
ktramp:
        
section .bss
align 65536
boot_stack: ; 64k boot stack 
        resb 65536