ROOT_DIR := $(realpath ../../..)

VORTEX_RT_PATH ?= $(ROOT_DIR)/runtime
VORTEX_KN_PATH ?= $(ROOT_DIR)/kernel

.PHONY: all
all: $(PROJECT)

include $(VORTEX_HOME)/tests/nodriver/host.mk
include $(VORTEX_HOME)/tests/nodriver/device.mk

VX_OBJ := vxbin.o

CPFLAGS += -I binary -O elf64-littleaarch64 -B aarch64
SECTION_NAME := vxbin
CPFLAGS += --rename-section .data=.$(SECTION_NAME),alloc,load,readonly,data,contents
CPFLAGS += --set-section-alignment .data=8
CPFLAGS += --redefine-sym _binary_kernel_vxbin_start=_vxbin_start
CPFLAGS += --redefine-sym _binary_kernel_vxbin_end=_vxbin_end

LDFLAGS += -static 
LDFLAGS += -L$(VORTEX_RT_PATH) -lvortex_nodrv

OBJS := $(VX_OBJ) $(HOST_OBJ)


$(PROJECT): $(OBJS) 
	$(CC) $^ $(LDFLAGS) -o $@

$(VX_OBJ): kernel.vxbin kernel.dump
	truncate -s %8 kernel.vxbin 
	$(CP) $(CPFLAGS) kernel.vxbin vxbin.o 

clean: clean-kernel clean-host

ifneq ($(MAKECMDGOALS),clean)
    -include .depend
endif
