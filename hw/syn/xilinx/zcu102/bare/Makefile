ROOT_DIR := $(realpath ../../../../..)
include $(ROOT_DIR)/config.mk

PLATFORM ?= zcu102_a53
PLATFORM_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare/platform/$(PLATFORM)
HAL_LIB_DIR := $(ROOT_DIR)/hw/syn/xilinx/zcu102/bare

.DEFAULT_GOAL := all

include $(PLATFORM_DIR)/platform.mk

vpath %.c $(HAL_SRC_DIR) $(TINYIO_DIR)/src

HAL_OBJ := $(HAL_LIB_DIR)/hal.o
LIB  := $(HAL_LIB_DIR)/libhal.a

CFLAGS += -std=c11 -O2 -Wall -Wextra -ffreestanding
CPPFLAGS += -I$(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare -MMD -MP

TARGETS := platform $(LIB) 

# If plaform.mk defines a custom startup file
ifdef STARTUP_FILE
  STARTUP_OBJ := $(HAL_LIB_DIR)/startup.o
  TARGETS += $(STARTUP_OBJ)

$(STARTUP_OBJ): $(STARTUP_FILE)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
endif

OBJS := ${HAL_OBJ} ${PLATFORM_OBJS}

.PHONY: all clean
all: $(TARGETS) 

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(HAL_LIB_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(HAL_LIB_DIR)/include
	rm -rf $(PLATFORM_FILES) $(LIB) $(OBJS) $(OBJS:.o=.d) 
	rm -rf $(STARTUP_OBJ) 

-include $(OBJS:.o=.d)

