# Host-side makefile 
PLATFORM			?= zcu102_a53
PLATFORM_DIR	?= $(VORTEX_HOME)/hw/syn/xilinx/zcu102/bare/platform/$(PLATFORM)

include $(PLATFORM_DIR)/platform.mk

CFLAGS += -std=c11 -Wall -Wextra -pedantic -Wfatal-errors -ffreestanding 
CFLAGS += $(CONFIGS)
CPPFLAGS += -I$(VORTEX_HOME)/runtime/include -I$(ROOT_DIR)/hw -I$(SW_COMMON_DIR) -I$(HAL_INC_DIR)

# Debugging
ifdef DEBUG
	CFLAGS += -g -O0
else
	CFLAGS += -O2 -DNDEBUG
endif

HOST_OBJ := host.o

$(HOST_OBJ): $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

.depend: $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $^ > .depend;

clean-host:
	rm -rf $(PROJECT) *.o *.log .depend

ifneq ($(MAKECMDGOALS),clean)
    -include .depend
endif
