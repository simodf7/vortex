# Host-side makefile 
CPU ?= a53

include $(HAL_SRC_DIR)/$(CPU).mk

CFLAGS += -std=c11 -Wall -Wextra -pedantic -Wfatal-errors -ffreestanding 
CFLAGS += -I$(VORTEX_HOME)/runtime/include -I$(ROOT_DIR)/hw -I$(SW_COMMON_DIR) -I$(HAL_SRC_DIR)
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
