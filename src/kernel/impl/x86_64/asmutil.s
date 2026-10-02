section .text
bits 64
global _inb
global _inw
global _ind

global _outb
global _outw
global _outd

global _insb
global _insw
global _insd

global _outsb
global _outsw
global _outsd

global _getcr3
global _setcr3

global _rdrand
global _rdseed

global _rdmsr
global _wrmsr

global _cpuid

global _rdtsc

global _cli
global _sti

global _wbinvd
global _invlpg
global gsd_try_aquire
_cli:
        cli 
        ret
_sti:
        sti
        ret
; u8 _inb(u16 port)
_inb:
        mov dx,di
        in al,dx
        ret
; u16 _inw(u16 port)
_inw:
        mov dx,di
        in ax,dx
        ret
; u32 _ind(u16 port)
_ind:
        mov dx,di
        in eax,dx
        ret
; outb(u16 dest,u8 data)
_outb:
        mov dx,di
        mov al,sil
        out dx,al
        ret
_outw:
        mov dx,di
        mov ax,si
        out dx,ax
        ret
_outd:
        mov dx,di
        mov eax,esi
        out dx,eax
        ret



_rdrand:
        rdrand rax 
        ret
_rdseed:
        rdseed rax
        ret
_getcr3:
        mov rax,cr3
        ret
_setcr3:
        mov cr3,rdi
        ret
_rdtsc:
        xor rax,rax
        rdtsc
        shl rdx,32
        or rax,rdx
        ret
_cpuid:
        mov eax,esi
        mov ecx,edi 
        cpuid
        mov [r8],eax
        mov [r8+4],ebx
        mov [r8+8],ecx
        mov [r8+12],edx
        ret
_rdmsr:
        mov ecx,edi
        xor rax,rax
        rdmsr 
        shl rdx,32
        or rax,rdx
        ret
_wrmsr:
        mov ecx,edi
        mov eax,esi
        shr rsi,32
        mov edx,esi
        wrmsr
        ret

_wbinvd:
        wbinvd
        ret
_invlpg:
        invlpg [rdi]
        ret
; gsd_try_aquire(gsd_atomic* atomic)
gsd_try_aquire:
        mov eax,0
        mov ecx,1
        lock cmpxchg [rdi],ecx
        jz .ok
        mov eax,0
        ret
        .ok:
                mov eax,1
                ret
gsd_release:
        mov dword [rdi],0
        ret


global atomic_increment
global atomic_decrement
; void atomic_increment(gsd_atomic* value);
atomic_increment:
        lock inc dword [rdi]
        ret
;void atomic_decrement(gsd_atomic* value);
atomic_decrement:
        lock dec dword [rdi]
        ret