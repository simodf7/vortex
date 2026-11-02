include ../common.mk 

DESTDIR ?= $(CURDIR)/..
SRC_DIR := $(VORTEX_HOME)/runtime/bare
HAL_SRC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare
CPU ?= a53

# Toolchain and cpu-specific flags 
include $(HAL_SRC_DIR)/$(CPU).mk 

CFLAGS += -std=c11 -Wall -Wextra -ffreestanding 
CFLAGS += -I$(SRC_DIR)/include -I$(ROOT_DIR)/hw -I$(INC_DIR)
CFLAGS += -I$(HAL_SRC_DIR)
CFLAGS += $(SW_CONFIGS) 

VX_DEBUG ?= 0
ifeq ($(VX_DEBUG),1)
  CFLAGS += -O0 -g -DVX_DEBUG
else
  CFLAGS += -O2
endif

VXRT_SRCS := $(SRC_DIR)/vortex.c 
VXDRV_SRCS   := $(SRC_DIR)/vx_driver.c
TARGET := libvortex_bare.a
OBJS := vortex.o vx_driver.o

.PHONY: all boot test kernel clean

all: $(DESTDIR)/$(TARGET) 

$(DESTDIR)/$(TARGET): $(OBJS)
	$(AR) rcs $@ $^ 

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DESTDIR)/$(TARGET) *.o $(KERNEL_BIN) 
