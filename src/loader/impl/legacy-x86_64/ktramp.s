section .text



; to kernel (uptr cr3,uptr rip,uptr rbp,uptr rsp)
to_kernel:
        mov rbp,r8
        mov rsp,r9
        mov cr3,rdi
        jmp rsi
