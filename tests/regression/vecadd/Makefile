ROOT_DIR := $(realpath ../../..)
include $(ROOT_DIR)/config.mk

PROJECT := vecadd

SRC_DIR := $(VORTEX_HOME)/tests/regression/$(PROJECT)

SRCS := $(SRC_DIR)/main.cpp

VX_SRCS := $(SRC_DIR)/kernel.cpp

OPTS ?= -n64
TYPE ?= int
CXXFLAGS += -DTYPE=$(TYPE)
VX_CFLAGS += -DTYPE=$(TYPE) 

STARTUP_ADDR ?= 0xA0000000
VORTEX_RT_LIB ?= vortex-soc

TEST ?= y

ifeq ($(TEST), y)
       CXXFLAGS += -DTEST
endif


HOST_ARCH ?= AARCH64
VITIS_BIN := /tools/Xilinx/Vitis/2022.2/gnu/aarch64/lin/aarch64-linux/bin

ifdef HOST_ARCH
        ifeq ($(HOST_ARCH), AARCH64)
                CC := $(VITIS_BIN)/aarch64-linux-elf-gcc
                CXX := $(VITIS_BIN)/aarch64-linux-gnu-g++
        endif
endif

include ../common.mk

LDFLAGS := -L$(VORTEX_RT_PATH) -l$(VORTEX_RT_LIB)

