
section .mbr
bits 16
global _boot
_boot:
	mov [bootdisk],dl
        cli ; disable irq's
        mov sp,0x7000       ; setup stack
        mov bp,0x1000
	call enable_a20
	cmp ax,1
	jne a20_panic
; boot2 = 128K Loaded at 0x10000-0x20000
load_boot2_binary: 
	mov ah,0x00
	mov dl,[bootdisk]
	int 0x13
	mov ax,0
	mov es,ax
	mov bx,0x7E00
	mov ah,0x2
	mov al,63
	mov ch,0
	mov cl,2
	mov dh,0
	mov dl,bootdisk
	int 0x13
	jc load_error
	jmp 0x7E00
load_error:
	mov al,[load_tries]
	inc al
	mov [load_tries],al
	cmp al,5
	jle load_boot2_binary
	jmp boot2_panic

load_tries:
	db 0

a20_panic:
	cli
	mov ax,0
	mov ds,ax
	mov si,a20_panic_msg
	call print
	jmp panic
boot2_panic:
	cli
	mov ax,0
	mov ds,ax
	mov si,boot2_load_msg
	call print
	jmp panic
print:
    lodsb             ; Load next byte from DS:SI into AL, increment SI
    or al, al         ; Check if AL is zero (end of string)
    jz print_done         ; If zero, jump to done
    mov ah, 0x0e      ; BIOS teletype function
    int 0x10          ; Call BIOS video interrupt
    jmp print 	    ; Repeat for next character
print_done:
    ret

;	out:
;	ax - state (0 - disabled, 1 - enabled)
get_a20_state:
	pushf
	push si
	push di
	push ds
	push es
	cli

	mov ax, 0x0000					;	0x0000:0x0500(0x00000500) -> ds:si
	mov ds, ax
	mov si, 0x0500

	not ax							;	0xffff:0x0510(0x00100500) -> es:di
	mov es, ax
	mov di, 0x0510

	mov al, [ds:si]					;	save old values
	mov byte [.BufferBelowMB], al
	mov al, [es:di]
	mov byte [.BufferOverMB], al

	mov ah, 1						;	check byte [0x00100500] == byte [0x0500]
	mov byte [ds:si], 0
	mov byte [es:di], 1
	mov al, [ds:si]
	cmp al, [es:di]
	jne .exit
	dec ah
.exit:
	mov al, [.BufferBelowMB]
	mov [ds:si], al
	mov al, [.BufferOverMB]
	mov [es:di], al
	shr ax, 8
	pop es
	pop ds
	pop di
	pop si
	popf
	ret
	
	.BufferBelowMB:	db 0
	.BufferOverMB:	db 0

;	out:
;		ax - a20 support bits (bit #0 - supported on keyboard controller; bit #1 - supported with bit #1 of port 0x92)
;		cf - set on error
query_a20_support:
	push bx
	clc

	mov ax, 0x2403
	int 0x15
	jc .error

	test ah, ah
	jnz .error

	mov ax, bx
	pop bx
	ret
.error:
	stc
	pop bx
	ret

enable_a20_keyboard_controller:
	cli

	call .wait_io1
	mov al, 0xad
	out 0x64, al
	
	call .wait_io1
	mov al, 0xd0
	out 0x64, al
	
	call .wait_io2
	in al, 0x60
	push eax
	
	call .wait_io1
	mov al, 0xd1
	out 0x64, al
	
	call .wait_io1
	pop eax
	or al, 2
	out 0x60, al
	
	call .wait_io1
	mov al, 0xae
	out 0x64, al

    call .wait_io1
	sti
	ret
.wait_io1:
	in al, 0x64
	test al, 2
	jnz .wait_io1
	ret
.wait_io2:
	in al, 0x64
	test al, 1
	jz .wait_io2
	ret

;	out:
;		cf - set on error
enable_a20:
	clc									;	clear cf
	pusha
	mov bh, 0							;	clear bh

	call get_a20_state
	jc .fast_gate

	test ax, ax
	jnz .done

	call query_a20_support
	mov bl, al
	test bl, 1							;	enable A20 using keyboard controller
	jnz .keyboard_controller

	test bl, 2							;	enable A20 using fast A20 gate
	jnz .fast_gate
.bios_int:
	mov ax, 0x2401
	int 0x15
	jc .fast_gate
	test ah, ah
	jnz .failed
	call get_a20_state
	test ax, ax
	jnz .done
.fast_gate:
	in al, 0x92
	test al, 2
	jnz .done

	or al, 2
	and al, 0xfe
	out 0x92, al

	call get_a20_state
	test ax, ax
	jnz .done

	test bh, bh							;	test if there was an attempt using the keyboard controller
	jnz .failed
.keyboard_controller:
	call enable_a20_keyboard_controller
	call get_a20_state
	test ax, ax
	jnz .done

	mov bh, 1							;	flag enable attempt with keyboard controller

	test bl, 2
	jnz .fast_gate
	jmp .failed
.failed:
	stc
.done:
	popa
	ret



panic:
	hlt
	jmp panic
a20_panic_msg:
	db "PANIC:A20",0
boot2_load_msg:
	db "PANIC:B2",0
bootdisk:
	db 0
times 446 - ($-$$) db 0
dw 0xAA55

