relpath=src/loader/impl/x86_64
buildpath=build/loader
sources=$(relpath)/boot-efi.c \
	$(relpath)/mem-efi.c \
	$(relpath)/tramp.s \
	$(relpath)/fs-efi.c 
objects=$(buildpath)/boot-efi.o \
	$(buildpath)/mem-efi.o \
	$(buildpath)/fs-efi.o \
	$(buildpath)/tramp.o
	

outbin=gsdb.efi
INC=-Ideps/common/ -Isrc/inc/gsd/ -Isrc/inc/ -Isrc/inc/gsd/impl/x86_64/ \
	-Ideps/common/efi-headers/Include/ -Ideps/common/efi-headers/Include/X64/ \
	-Ideps/common 

CC=x86_64-w64-mingw32-gcc
CF=-ffreestanding -mno-red-zone -nostdlib -Wno-incompatible-pointer-types -Wno-int-conversion -c $(INC)
LF=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main
AF=-f win64 
ifdef DEBUG
CF += -g
AF += -g
LF += -g
endif

.DEFAULT=all
.PHONY:all

vpath %.s $(relpath)
vpath %.c $(relpath)


$(buildpath)/%.o : %.s
	nasm $(AF) -o $@ $^
$(buildpath)/%.o : %.c
	$(CC) $(CF) -o $@ $^

$(outbin): $(objects)
	$(CC) $(LF) -o $@ $^
	rm $(objects)
	objcopy --only-keep-debug $(outbin) $(outbin).dbg
all: $(outbin)

