relpath=src/loader/impl/x86/efi-x86_64
sources=$(relpath)/boot-efi.c $(relpath)/tramp.s
objects=$(relpath)/boot-efi.o $(relpath)/tramp.o
outbin=gsdb.efi
INC=-Ideps/common/ -Isrc/inc/gsd/ -Isrc/inc/ -Isrc/inc/gsd/impl/x86_64/ \
	-Ideps/common/efi-headers/Include/ -Ideps/common/efi-headers/Include/X64/ \
	-Ideps/common

CC=x86_64-w64-mingw32-gcc
CF=-ffreestanding -Wno-incompatible-pointer-types -Wno-int-conversion -c $(INC)
LF=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main
.DEFAULT=all
.PHONY:all

%.o : %.s
	nasm -f win64 -o $@ $^
%.o : %.c
	$(CC) $(CF) -o $@ $^

$(outbin): $(objects)
	$(CC) $(LF) -o $@ $^
	rm $(objects)
all: $(outbin)

