ROOT_DIR := $(realpath ../../..)

VORTEX_RT_PATH ?= $(ROOT_DIR)/runtime
VORTEX_KN_PATH ?= $(ROOT_DIR)/kernel
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare
HAL_INC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare

.PHONY: all
all: $(PROJECT)

include $(VORTEX_HOME)/tests/bare/host.mk
include $(VORTEX_HOME)/tests/bare/device.mk

VX_OBJ := vxbin.o

SECTION_NAME := vxbin
OBJCP_FLAGS += --rename-section .data=.$(SECTION_NAME),alloc,load,readonly,data,contents
OBJCP_FLAGS += --set-section-alignment .data=8
OBJCP_FLAGS += --redefine-sym _binary_kernel_vxbin_start=_vxbin_start
OBJCP_FLAGS += --redefine-sym _binary_kernel_vxbin_end=_vxbin_end

LDFLAGS += -L$(VORTEX_RT_PATH) -lvortex_bare
LDFLAGS += -L$(HAL_LIB_DIR) -lhal
LDFLAGS += $(LDLIBS)

OBJS := $(VX_OBJ) $(HOST_OBJ)

ifdef STARTUP_FILE
	OBJS += $(HAL_LIB_DIR)/startup.o
endif 

$(PROJECT): $(OBJS) 
	$(CC) $^ $(LDFLAGS) -o $@

$(VX_OBJ): kernel.vxbin kernel.dump
	truncate -s %8 kernel.vxbin 
	$(CP) $(OBJCP_FLAGS) kernel.vxbin vxbin.o 

clean: clean-kernel clean-host

ifneq ($(MAKECMDGOALS),clean)
    -include .depend
endif
