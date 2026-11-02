include ../common.mk 

HOST_ARCH = AARCH64
DESTDIR ?= $(CURDIR)/..

SRC_DIR := $(VORTEX_HOME)/runtime/nodriver

CFLAGS += -std=c11 -Wall -Wextra
CFLAGS += -I$(SRC_DIR)/include -I$(ROOT_DIR)/hw -I$(INC_DIR)

ifdef CONFIG_FLAGS
	CFLAGS += $(CONFIG_FLAGS) 
endif 


ifdef DEBUG
  CFLAGS += -O0 -g
else
  CFLAGS += -O2
endif

LDFLAGS := -static

CPFLAGS += -I binary -O elf64-littleaarch64 -B aarch64 
SECTION_NAME := vxbin
CPFLAGS += --rename-section .data=.$(SECTION_NAME),alloc,load,readonly,data,contents
CPFLAGS += --set-section-alignment .data=8
CPFLAGS += --redefine-sym _binary_kernel_vxbin_start=_vxbin_start
CPFLAGS += --redefine-sym _binary_kernel_vxbin_end=_vxbin_end


VITIS_BIN := /tools/Xilinx/Vitis/2022.2/gnu/aarch64/lin/aarch64-linux/bin

ifdef HOST_ARCH
	ifeq ($(HOST_ARCH), AARCH64)
		CC = $(VITIS_BIN)/aarch64-linux-gnu-gcc
		CXX = $(VITIS_BIN)/aarch64-linux-gnu-g++ 
		AR = $(VITIS_BIN)/aarch64-linux-gnu-ar
		NM = $(VITIS_BIN)/aarch64-linux-gnu-nm
		CP = $(VITIS_BIN)/aarch64-linux-gnu-objcopy
	endif
endif


KN_DIR := $(ROOT_DIR)/kernel
KERNEL_DIR := $(ROOT_DIR)/tests/regression


VXRT_SRCS := $(SRC_DIR)/vortex.c 
VXDRV_SRCS   := $(SRC_DIR)/vx_driver.c
TARGET := libvortex_nodrv.a
OBJS := vortex.o vx_driver.o


KERNEL_BIN := kernel.vxbin
TEST_KERNEL ?= copy
TEST_TARGET := vxtest
TEST_OBJS   := kernel.o vx_driver.o vxbin.o


.PHONY: all test kernel clean

all: $(DESTDIR)/$(TARGET) 

$(DESTDIR)/$(TARGET): $(OBJS)
	$(AR) rcs $@ $^ 

test: $(TEST_TARGET) 
	echo "Test target obtained"

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel: 
	$(MAKE) -C $(KN_DIR) clean 
	$(MAKE) -C $(KERNEL_DIR)/$(KERNEL) clean 
	$(MAKE) -C $(KN_DIR) CONFIG_FLAGS="$(CONFIG_FLAGS)"
	TEST=$(TEST) $(MAKE) -C $(KERNEL_DIR)/$(TEST_KERNEL) kernel.vxbin STARTUP_ADDR=$(STARTUP_ADDR) 
	TEST=$(TEST) $(MAKE) -C $(KERNEL_DIR)/$(TEST_KERNEL) kernel.dump STARTUP_ADDR=$(STARTUP_ADDR) 
	cp $(ROOT_DIR)/tests/regression/$(TEST_KERNEL)/kernel.vxbin .

$(KERNEL_BIN): kernel 

vxbin.o: $(KERNEL_BIN)
	truncate -s %8 $^
	$(CP) $(CPFLAGS) $^ $@ 

clean:
	rm -f $(DESTDIR)/$(TARGET) $(TEST_TARGET) *.o $(KERNEL_BIN) 
