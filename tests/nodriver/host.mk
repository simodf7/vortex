# Host-side makefile 

HOST_ARCH ?= AARCH64
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

CFLAGS += -std=c11 -Wall -Wextra -pedantic -Wfatal-errors
CFLAGS += -I$(VORTEX_HOME)/runtime/include -I$(ROOT_DIR)/hw -I$(SW_COMMON_DIR)
CFLAGS += $(CONFIGS)

# Debugging
ifdef DEBUG
	CFLAGS += -g -O0
else
	CFLAGS += -O2 -DNDEBUG
endif

HOST_OBJ := host.o

$(HOST_OBJ): $(SRCS)
	$(CC) $(CFLAGS) -c $^ -o $@

.depend: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > .depend;

clean-host:
	rm -rf $(PROJECT) *.o *.log .depend

ifneq ($(MAKECMDGOALS),clean)
    -include .depend
endif
