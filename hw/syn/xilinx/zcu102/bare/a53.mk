HAL_SRC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare

VITIS_BIN := /tools/Xilinx/Vitis/2022.2/gnu/aarch64/lin/aarch64-none/bin
CROSS := aarch64-none-elf

CC = $(VITIS_BIN)/$(CROSS)-gcc
CXX = $(VITIS_BIN)/$(CROSS)-g++
AR = $(VITIS_BIN)/$(CROSS)-ar
NM = $(VITIS_BIN)/$(CROSS)-nm
CP = $(VITIS_BIN)/$(CROSS)-objcopy

CFLAGS += -mcpu=cortex-a53

ifdef SW_CONFIGS 
	CPPFLAGS += $(SW_CONFIGS)
endif


LDSCRIPT := $(HAL_LIB_DIR)/lscript.ld
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += -L$(HAL_LIB_DIR) 
LDLIBS += -Wl,--start-group -lxil -lgcc -lc -Wl,--end-group

# Flags for Objcopy 
CPFLAGS += -I binary -O elf64-littleaarch64 -B aarch64
