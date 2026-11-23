HAL_SRC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare/platform/zcu102_a53
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare
TCL_DIR     := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/tcl
TINYIO_DIR  := $(VORTEX_HOME)/third_party/tinyio

XSCT := $(XILINX_VITIS)/bin/xsct
VITIS_BIN ?= $(XILINX_VITIS)/gnu/aarch64/lin/aarch64-none/bin
CROSS     := aarch64-none-elf

CC  = $(VITIS_BIN)/$(CROSS)-gcc
CXX = $(VITIS_BIN)/$(CROSS)-g++
AR  = $(VITIS_BIN)/$(CROSS)-ar
NM  = $(VITIS_BIN)/$(CROSS)-nm
CP  = $(VITIS_BIN)/$(CROSS)-objcopy

CFLAGS   += -mcpu=cortex-a53

ifdef SW_CONFIGS
	CPPFLAGS += $(SW_CONFIGS)
endif

CPPFLAGS += -I$(HAL_LIB_DIR)/include
CPPFLAGS += -I$(TINYIO_DIR)/inc

LDSCRIPT := $(HAL_LIB_DIR)/lscript.ld
LDFLAGS  += -T$(LDSCRIPT) -L$(HAL_LIB_DIR)
LDLIBS   += -Wl,--start-group -lxil -lgcc -lc -Wl,--end-group

# Flags for Objcopy
OBJCP_FLAGS += -I binary -O elf64-littleaarch64 -B aarch64

PLATFORM_FILES := $(addprefix $(HAL_LIB_DIR)/, fsbl.elf pmufw.elf lscript.ld libxil.a)
TINYIO_SRCS := $(addprefix $(TINYIO_DIR)/src/, print.c scan.c tinyIO.c)
PLATFORM_OBJS := $(addprefix $(HAL_LIB_DIR)/, print.o)

.PHONY: platform
platform: $(PLATFORM_FILES) $(TINYIO_SRCS)

$(PLATFORM_FILES) &:
	CONFIG=$(CONFIG) $(XSCT) $(TCL_DIR)/build_platform.tcl $(HAL_LIB_DIR)
	rm -rf $(HAL_LIB_DIR)/tmp

$(TINYIO_SRCS) &:
	git -C $(VORTEX_HOME) submodule update --init third_party/tinyio
