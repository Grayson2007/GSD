# Central Configuration File
export DEBUG=1
export ARCH=x86_64
export TC_CC=gcc
export TC_CF=-ffreestanding -nostdlib -mno-red-zone -fno-stack-protector -mcmodel=large -c -DGSD_ARCHITECTURE=$(ARCH)
export TC_LD=ld 
export TC_LF=-nostdlib -static
export TC_AS=nasm
export TC_AF=-f elf64 
ifdef DEBUG
TC_LF += -g
TC_CF += -g
TC-AF += -g
endif

