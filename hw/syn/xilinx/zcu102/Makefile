ROOT_DIR := $(realpath ../../../..)
include $(ROOT_DIR)/config.mk


DEVICE ?= xczu9eg-ffvb1156-2-e

MAX_JOBS ?= 2
NCPUS := $(shell lscpu | grep "^Core(s) per socket:" | awk '{print $$4}')
JOBS ?= $(shell echo $$(( $(NCPUS) > $(MAX_JOBS) ? $(MAX_JOBS) : $(NCPUS) )))
VIVADO := $(XILINX_VIVADO)/bin/vivado

SRC_DIR := $(VORTEX_HOME)/hw/syn/xilinx/zcu102
RTL_DIR := $(VORTEX_HOME)/hw/rtl
DPI_DIR := $(VORTEX_HOME)/hw/dpi
AFU_DIR := $(RTL_DIR)/afu/xrt
SCRIPT_DIR := $(VORTEX_HOME)/hw/scripts

KERNEL ?= blinky

# ADD: Vortex Hardware configuration file 

CONFIG ?= pl_ddr

vortex_config.mk: $(SRC_DIR)/configs/$(CONFIG).json $(SRC_DIR)/gen_mk.py
	python3 $(SRC_DIR)/gen_mk.py $< -o $@
		 
include vortex_config.mk 

#denable DEBUG
DEBUG ?= 3


# include paths
FPU_INCLUDE = -I$(RTL_DIR)/fpu
ifneq (,$(findstring -DFPU_FPNEW, $(CONFIGS)))
	FPU_INCLUDE += -J$(THIRD_PARTY_DIR)/cvfpu/src/common_cells/include -J$(THIRD_PARTY_DIR)/cvfpu/src/common_cells/src -J$(THIRD_PARTY_DIR)/cvfpu/src/fpu_div_sqrt_mvp/hdl -J$(THIRD_PARTY_DIR)/cvfpu/src
endif
RTL_INCLUDE = -I$(RTL_DIR) -I$(RTL_DIR)/libs -I$(RTL_DIR)/interfaces -I$(RTL_DIR)/core -I$(RTL_DIR)/mem -I$(RTL_DIR)/cache
RTL_INCLUDE += $(FPU_INCLUDE)
RTL_INCLUDE += -I$(SRC_DIR)

# Add V extension sources
ifneq (,$(findstring -DEXT_V_ENABLE, $(CONFIGS)))
	RTL_INCLUDE += -I$(RTL_DIR)/vpu
endif

# Add TCU extension sources
ifneq (,$(findstring -DEXT_TCU_ENABLE, $(CONFIGS)))
	RTL_INCLUDE += -I$(RTL_DIR)/tcu
	RTL_INCLUDE += -I$(RTL_DIR)/tcu/bhf
	RTL_INCLUDE += -J$(THIRD_PARTY_DIR)/hardfloat/source/RISCV
	RTL_INCLUDE += -I$(THIRD_PARTY_DIR)/hardfloat/source
endif

# compilation flags
CFLAGS += -DNDEBUG -DSYNTHESIS -DVIVADO
CFLAGS += $(CONFIGS) # (from vortex-config.mk) 
CFLAGS += $(RTL_INCLUDE)
CFLAGS += $(EXT_FLAGS) # (from vortex-config.mk) 


# control RTL debug tracing states
DBG_TRACE_FLAGS += -DDBG_TRACE_PIPELINE
DBG_TRACE_FLAGS += -DDBG_TRACE_MEM
DBG_TRACE_FLAGS += -DDBG_TRACE_CACHE
DBG_TRACE_FLAGS += -DDBG_TRACE_AFU
DBG_TRACE_FLAGS += -DDBG_TRACE_GBAR
DBG_TRACE_FLAGS += -DDBG_TRACE_TCU

# Debugging
ifdef DEBUG
	ifneq ($(TARGET), fpga)
		CFLAGS += -DDEBUG_LEVEL=$(DEBUG) $(DBG_TRACE_FLAGS) -DSIMULATION
	endif
endif

all: build

$(KERNEL).bin:
	$(MAKE) -C $(ROOT_DIR)/kernel clean
	$(MAKE) -C $(ROOT_DIR)/kernel CFLAGS+="$(KERN_ADDRESS_FLAGS)" 
	$(MAKE) -C $(ROOT_DIR)/tests/kernel/$(KERNEL) clean
	$(MAKE) -C $(ROOT_DIR)/tests/kernel/$(KERNEL) CFLAGS+="$(KERN_ADDRESS_FLAGS)"
	cp $(ROOT_DIR)/tests/kernel/$(KERNEL)/$(KERNEL).bin $(KERNEL).bin

kernel.bin.coe: $(KERNEL).bin
	$(SCRIPT_DIR)/bin2coe.py --out=$@ --binfile=8192:$(KERNEL).bin --depth=16384 --wordsize=64 --little_endian


project.tcl: $(SRC_DIR)/project.tcl.in
	sed 's|@VORTEX_HOME@|$(VORTEX_HOME)|g; s|@XLEN@|$(XLEN)|g; s|@BUILDDIR@|$(CURDIR)|g' $< > $@

%.tcl: $(SRC_DIR)/%.tcl 
	cp $< $@

%.xdc: $(SRC_DIR)/%.xdc
	cp $< $@

.PHONY: hardware bd ip_repo host-dev
ip_repo: 
	cp -r $(SRC_DIR)/$@ .

gen-sources: vortex_config.mk project_1/sources.txt
project_1/sources.txt:
	mkdir -p project_1
	$(SCRIPT_DIR)/gen_sources.sh $(CFLAGS) -P -Cproject_1/src -Oproject_1/sources.txt

build: done.dcp
done.dcp: project_1/sources.txt project.tcl pre_opt_hook.tcl soc_bd.tcl ip_repo top.xdc $(GLOBAL_MEM).tcl
	MAX_JOBS=$(JOBS) FPU_IP=project_1/ip TOOL_DIR=$(SCRIPT_DIR) $(BD_ENV) $(VIVADO) -mode batch -source project.tcl -tclargs $(DEVICE) project_1/sources.txt
	echo done > done.dcp

hardware: xsa_gen.tcl done.dcp 	
	MAX_JOBS=$(JOBS) TOOL_DIR=$(SCRIPT_DIR) $(VIVADO) -mode batch -source xsa_gen.tcl -tclargs project_1/project_1.xpr 

run: simulate.tcl done.dcp
	MAX_JOBS=$(JOBS) TOOL_DIR=$(SCRIPT_DIR) $(VIVADO) -mode batch -source simulate.tcl -tclargs project_1/project_1.xpr 50000ns

open: done.dcp
	TOOL_DIR=$(SCRIPT_DIR) $(VIVADO) project_1/project_1.xpr &


bd: project_1/sources.txt init_bd.tcl pre_opt_hook.tcl soc_bd.tcl ip_repo top.xdc $(GLOBAL_MEM).tcl 
	$(BD_ENV) FPU_IP=project_1/ip TOOL_DIR=$(SCRIPT_DIR) \
	  $(VIVADO) -mode gui -source init_bd.tcl -tclargs $(DEVICE) project_1/sources.txt


TEST ?= n

host-dev: 
	$(MAKE) -C $(ROOT_DIR)/kernel clean
	$(MAKE) -C $(ROOT_DIR)/kernel CFLAGS+="$(KERN_ADDRESS_FLAGS)"
	mkdir -p host-dev 
	$(MAKE) -C $(ROOT_DIR)/runtime/soc clean 
	$(MAKE) -C $(ROOT_DIR)/runtime/soc CXXFLAGS+="$(ADDRESS_FLAGS)"
	cp $(ROOT_DIR)/runtime/libvortex-soc.so host-dev/libvortex-soc.so 	
	$(MAKE) -C $(ROOT_DIR)/tests/regression/$(KERNEL) clean
	TEST=$(TEST) $(MAKE) -C $(ROOT_DIR)/tests/regression/$(KERNEL) 
	cp $(ROOT_DIR)/tests/regression/$(KERNEL)/$(KERNEL) host-dev/$(KERNEL) 
	cp $(ROOT_DIR)/tests/regression/$(KERNEL)/kernel.vxbin host-dev/kernel.vxbin 
	cp $(ROOT_DIR)/tests/regression/$(KERNEL)/kernel.dump host-dev/kernel.dump 

clean:
	rm -rf project_1 $(KERNEL).bin kernel.bin.coe
	rm -rf .Xil *.log *.jou *.dcp *.rpt
	rm -rf host-dev
	rm -rf *.mk *.tcl *.xdc
	rm -rf vivado_* ip_repo
