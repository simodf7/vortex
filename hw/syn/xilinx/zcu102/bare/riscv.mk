HAL_SRC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare

RISCV_BIN := $(TOOLDIR)/riscv-simplyv/bin
CROSS := riscv64-unknown-elf

CC = $(RISCV_BIN)/$(CROSS)-gcc
CXX = $(RISCV_BIN)/$(CROSS)-g++
AR = $(RISCV_BIN)/$(CROSS)-ar
NM = $(RISCV_BIN)/$(CROSS)-nm
CP = $(RISCV_BIN)/$(CROSS)-objcopy

CFLAGS += -march=rv64gc -mabi=lp64d

ifdef SW_CONFIGS 
	CPPFLAGS += $(SW_CONFIGS)
endif

# TODO (Simply-V): linker script, startup.s, -nostdlib + tinyio.a/libsimplyv.a

LDFLAGS += -L$(HAL_LIB_DIR) 

# Flags for Objcopy 
CPFLAGS += -I binary -O elf64-littleriscv -B riscv
