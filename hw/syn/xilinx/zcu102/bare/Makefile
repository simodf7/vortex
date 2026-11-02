ROOT_DIR := $(realpath ../../../../..)
include $(ROOT_DIR)/config.mk

HAL_SRC_DIR   := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare
TINYIO_DIR := $(HAL_SRC_DIR)/tinyIO

CPU ?= a53

include $(HAL_SRC_DIR)/$(CPU).mk

vpath %.c $(HAL_SRC_DIR) $(TINYIO_DIR)/src

OBJS := $(HAL_LIB_DIR)/hal.o $(HAL_LIB_DIR)/print.o
LIB  := $(HAL_LIB_DIR)/libhal_$(CPU).a

CFLAGS += -std=c11 -O2 -Wall -Wextra -ffreestanding
CPPFLAGS += -I$(TINYIO_DIR)/inc -I$(HAL_LIB_DIR)/include -MMD -MP

.PHONY: all clean
all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(HAL_LIB_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.elf *.a *.ld
	rm -rf include
	
-include $(OBJS:.o=.d)
