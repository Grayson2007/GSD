reldir=src/kernel
stdlibdir=src/gsdstd/
build_dir=build/kernel/$(ARCH)
obj_dir=$(build_dir)/o
OUTPUT=$(build_dir)/kernel.elf
INC-DIR=src/inc
COMMON-DEP-DIR=deps/common
IMPL_DEP_DIR=deps/$(ARCH)/

INCLUDE= -I $(INC-DIR)/gsd/\
	-I $(INC-DIR)/gsd/impl/$(ARCH)/ \
	-I $(COMMON-DEP-DIR) \
	-I $(COMMON-DEP-DIR)/efi-headers/ \
	-I $(COMMON-DEP-DIR)/rb-tree \
	-I $(COMMON-DEP-DIR)/efi-headers/Include/ \
	-I $(DEP-DIR)/$(ARCH) \
	-I $(INC-DIR)/gsdstd\
	-I $(INC-DIR) \
	-I $(COMMON-DEP-DIR)/scalable-font2/

SRCVP=$(shell find $(reldir) -type d -not -path $(reldir)/impl) \
	$(shell find $(stdlibdir) -type d) \
	$(shell find $(reldir)/impl/$(arch) -type d)  \
	deps/common/rb_tree/
CSRC=$(foreach dir,$(SRCVP),$(wildcard $(dir)/*.c)) \
	rb_tree.c
ASRC=$(foreach dir,$(SRCVP),$(wildcard $(dir)/*.s))
COBJ=$(patsubst %.c,$(obj_dir)/%.o,$(notdir $(CSRC)))
AOBJ=$(patsubst %.s,$(obj_dir)/%.o,$(notdir $(ASRC)))
LDS=$(build_dir)/link.lds
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
vpath %.c $(subst $(SPACE),:,$(SRCVP)) 
vpath %.s $(subst $(SPACE),:,$(SRCVP))

.DEFAULT=$(OUTPUT)
$(obj_dir)/%.o : %.c
	$(TC_CC) $(TC_CF) $(INCLUDE) -o $@ $<
$(obj_dir)/%.o : %.s
	$(TC_AS) $(TC_AF) $< -o $@

$(OUTPUT) : $(COBJ) $(AOBJ)
	$(TC_LD) $(LDF) -T $(LDS) -o $@ $^