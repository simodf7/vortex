ROOT_DIR := $(realpath ../../..)

VORTEX_RT_PATH ?= $(ROOT_DIR)/runtime
VORTEX_KN_PATH ?= $(ROOT_DIR)/kernel
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare
HAL_SRC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare

.PHONY: all
all: $(PROJECT)

include $(VORTEX_HOME)/tests/bare/host.mk
include $(VORTEX_HOME)/tests/bare/device.mk

VX_OBJ := vxbin.o

SECTION_NAME := vxbin
CPFLAGS += --rename-section .data=.$(SECTION_NAME),alloc,load,readonly,data,contents
CPFLAGS += --set-section-alignment .data=8
CPFLAGS += --redefine-sym _binary_kernel_vxbin_start=_vxbin_start
CPFLAGS += --redefine-sym _binary_kernel_vxbin_end=_vxbin_end

LDFLAGS += -L$(VORTEX_RT_PATH) -lvortex_bare
LDFLAGS += -L$(HAL_LIB_DIR) -lhal_$(CPU)
LDFLAGS += $(LDLIBS)

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
