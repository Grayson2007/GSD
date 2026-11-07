section .text
bits 64
global memset
global memcpy
global memmove
global memcmp

; rdi = dest
; sil = value 
; edx = n
memset:
        mov rcx,rdx
        mov al,sil
        rep stosb
        ret
memcpy:
        mov rcx,rdx
        rep movsb
        ret
memcmp:
        mov rcx,rdi
        mov al,sil
        rep cmpsb
        jb .less
        ja .pos
        mov eax,0
        ret 
        .less:
                mov eax,-1
                ret
        .pos:
                mov eax,1
                ret
memmove:
        mov r8,rdi
        add r8,rdx
        mov r9,rsi
        add r9,rdx
        
        cmp rdi,r9
        jb .overlap
        cmp rsi,r8
        jb .overlap
        ; standard memcpy 
        mov rcx,rdx
        rep movsb
        ret 
        .overlap: ; overlap case copy backwards from end to start 
                lea rdi,[r8-1]
                lea rsi,[r9-1]
                std 
                mov rcx,rdx
                rep movsb
                cld
                ret

