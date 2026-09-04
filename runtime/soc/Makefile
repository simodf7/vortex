include ../common.mk

HOST_ARCH = AARCH64
DESTDIR ?= $(CURDIR)/..

CONFIG_FILE := $(DESTDIR)/soc_rt_config.stamp
SRC_DIR := $(VORTEX_HOME)/runtime/soc

CXXFLAGS += -std=c++17 -Wall -Wextra -Wfatal-errors
CXXFLAGS += -fPIC -Wno-maybe-uninitialized
CXXFLAGS += -I$(INC_DIR) -I../common -I$(ROOT_DIR)/hw -I$(SIM_DIR)/simx -I$(SW_COMMON_DIR) -I$(RT_COMMON_DIR)
CXXFLAGS += -DXLEN_$(XLEN)
CXXFLAGS += $(CONFIGS)

# NEW: Adding CXXFLAGS to have STARTUP and stack base available
STARTUP_ADDR ?= 0xA0000000
STACK_BASE_ADDR ?= 0xA0040000

CXXFLAGS += -DSTARTUP_ADDR=$(STARTUP_ADDR) 
CXXFLAGS += -DSTACK_BASE_ADDR=$(STACK_BASE_ADDR) 

LDFLAGS += -shared
LDFLAGS += -L$(DESTDIR)

VITIS_BIN := /tools/Xilinx/Vitis/2022.2/gnu/aarch64/lin/aarch64-linux/bin

ifdef HOST_ARCH
	ifeq ($(HOST_ARCH), AARCH64)
		CC = $(VITIS_BIN)/aarch64-linux-elf-gcc
		CXX = $(VITIS_BIN)/aarch64-linux-gnu-g++ 
	endif
endif

SRCS := $(SRC_DIR)/vortex.cpp

# Debugging
ifdef DEBUG
	CXXFLAGS += -g -O0
else
	CXXFLAGS += -O2 -DNDEBUG
endif

PROJECT := libvortex-soc.so

.PHONY: all force driver clean-runtime clean

all: $(DESTDIR)/$(PROJECT)

$(DESTDIR)/$(PROJECT): $(SRCS) $(CONFIG_FILE)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $@

# updates the timestamp when flags changed.
$(CONFIG_FILE): force
	@mkdir -p $(@D)
	@printf '%s\n' "$(CXXFLAGS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then \
	  mv $@.tmp $@; \
	else \
	  rm $@.tmp; \
	fi

clean-runtime:
	rm -f $(DESTDIR)/$(PROJECT)

clean: clean-runtime
