ROOT_DIR := $(realpath ../../..)
include $(ROOT_DIR)/config.mk

PROJECT := vecadd

SRC_DIR := $(VORTEX_HOME)/tests/bare/$(PROJECT)
SRCS := $(SRC_DIR)/main.c
VX_SRCS := $(SRC_DIR)/kernel.cpp

OPTS ?= -n64
TYPE ?= float
CFLAGS += -DTYPE=$(TYPE)
VX_CFLAGS += -DTYPE=$(TYPE) 

TEST ?= n

ifeq ($(TEST), y)
       CFLAGS += -DTEST
endif

include ../common.mk


