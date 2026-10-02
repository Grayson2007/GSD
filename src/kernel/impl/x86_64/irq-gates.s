section .text
bits 64
 
extern gsd_irq_c_handler 
extern gsd_next_process
extern gsd_handle_termination
extern gsd_handle_kerncall
global _asm_goto_burst
global _asm_on_burst_end
%macro ctxsav 0
push rbp
push r15
push r14
push r13
push r12
push r11
push r10
push r9
push r8
push rdi
push rsi
push rdx
push rcx
push rbx
push rax
sub rsp,512
fxsave [rsp]
%endmacro

%macro ctxrstor 0
fxrstor [rsp]
add rsp,512
pop rax
pop rbx
pop rcx
pop rdx
pop rsi
pop rdi
pop r8
pop r9
pop r10
pop r11
pop r12
pop r13
pop r14
pop r15
pop rbp
%endmacro

%assign i 0
%rep 250
global _asm_onirq_#i
_asm_onirq_%+i:
        ctxsav 
        mov rdi,i
        jmp gsd_irq_c_handler
%assign i i+1
%endrep




; IRQ 251
_asm_on_terminate:
        jmp gsd_handle_termination
; IRQ 0xFC
; edi function 
; esi argc 
; r8 argarr 
_asm_irq_kerncall:
        ctxsav
        call gsd_handle_kerncall
        ctxrstor
        iretq


_asm_on_burst_end:
        ctxsav
        mov rdi,rsp
        jmp gsd_next_process

; NORETURN void _asm_goto_burst(void* rsp)
_asm_goto_burst:
        mov rsp,rdi
        ctxrstor
        iretq
