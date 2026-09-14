import json
import sys
import argparse 


def align_up(address, size): 
	return (address + size - 1) & ~(size - 1) 

parser = argparse.ArgumentParser() 
parser.add_argument("config", help="Configuration file (json)")
parser.add_argument("-o", "--output", help="Makefile to generate")

args = parser.parse_args()
config_file = args.config  
makefile = args.output 

with open(args.config, 'r', encoding='utf-8') as file:
    cfg = json.load(file)


xlen = cfg["xlen"]

num_cores = cfg["num_cores"] 
num_cluster = cfg["num_cluster"] 
num_warps = cfg["num_warps"] 
num_threads = cfg["num_threads"] 
num_sockets = cfg["num_sockets"] 

# Stack size calculation 

total_threads = num_cluster * num_cores * num_warps * num_threads; 

mem_base = int(cfg["mem_base"], 16)  
mem_size = int(cfg["mem_size"], 16)  

kernel_size = int(cfg["kernel_size"], 16)
startup_addr = mem_base 

stack_log2_size = cfg["stack_log2_size"] 
stack_size = total_threads << stack_log2_size
stack_base_addr = startup_addr + kernel_size + int(cfg["guard_size"], 16) + stack_size 

# MPM size calculation 

mpm_size = 256 * num_cores
mpm_base = mem_base + mem_size - mpm_size 
 

if stack_base_addr >= mpm_base: 
	raise SystemExit(
        f"memoria insufficiente: {total_threads} thread richiedono "
        f"{stack_size//1024} KB di stack, heap a {stack_base_addr:#x} "
        f"oltrepassa la MPM a {mpm_base:#x}")

mpm_offs = mpm_base - mem_base 

# Heap 

heap_offs = stack_base_addr - mem_base  # offset from MEM_BASE addr 
heap_size = mpm_base - stack_base_addr 


# Peripherals 

console_size = int(cfg["console_size"], 16) 
dcr_size = int(cfg["dcr_size"], 16) 
gpio_size = int(cfg["gpio_size"],16) 

io_base_addr = align_up(mem_base + mem_size, console_size) 
console_base = io_base_addr
dcr_base = align_up(console_base + console_size, dcr_size)
gpio_base = align_up(dcr_base + dcr_size, gpio_size) 
io_end_addr = gpio_base + gpio_size



### Generate Makefile 

rows = [] 

## CONFIGS 



configs = []

rows.append(f"XLEN := {xlen}") 
rows.append(f"NUM_CORES	:= {num_cores}") 
rows.append(f"NUM_THREADS	:= {num_threads}") 
rows.append(f"NUM_WARPS	:= {num_warps}") 
rows.append(f"NUM_CLUSTER	:= {num_cluster}") 
rows.append(f"NUM_SOCKETS	:= {num_sockets}") 
rows.append(f"STACK_LOG2_SIZE := {stack_log2_size}")

configs.append(f"-DXLEN_{xlen}") 
configs.append(f"-DNUM_CORES={num_cores}") 
configs.append(f"-DNUM_THREADS={num_threads}") 
configs.append(f"-DNUM_WARPS={num_warps}") 
configs.append(f"-DNUM_CLUSTERS={num_cluster}") 
configs.append(f"-DNUM_SOCKETS={num_sockets}") 
configs.append(f"-DSTACK_LOG2_SIZE={stack_log2_size}") 
configs.append(f"-DSTARTUP_ADDR={xlen}\\'h{startup_addr:x}") 
configs.append(f"-DSTACK_BASE_ADDR={xlen}\\'h{stack_base_addr:x}") 
configs.append(f"-DIO_BASE_ADDR={xlen}\\'h{io_base_addr:x}") 
configs.append(f"-DIO_MPM_ADDR={xlen}\\'h{mpm_base:x}") 
configs.append(f"-DIO_END_ADDR={xlen}\\'h{io_end_addr:x}") 

if(not cfg["l1_cache"]): 
	configs.append(f"-DL1_DISABLE") 
else: 
	l1_size = cfg["l1_cache_dim"]
	configs.append(f"-DICACHE_SIZE={l1_size}") 
	configs.append(f"-DDCACHE_SIZE={l1_size}") 


if(not cfg["local_mem"]): 
	configs.append(f"-DLMEM_DISABLE") 
else: 
	lmem_log_size = cfg["local_mem_log_size"]
	configs.append(f"-DLMEM_LOG_SIZE={lmem_log_size}") 


rows.append("CONFIGS += " + " ".join(configs))


## EXTENSION FLAGS
 
OPT_OUT = {"M", "F", "D", "ZICOND", "CRYPTO"}   # enabled by default
OPT_IN  = {"A", "C", "V", "TCU"}                # disabled by default

ext_flags = []

if cfg["extensions"]["D"] and cfg["xlen"] != 64:
    raise SystemExit("EXT_D requires XLEN=64")
if cfg["extensions"]["D"] and not cfg["extensions"]["F"]:
    raise SystemExit("EXT_D requires EXT_F")

for name, on in cfg["extensions"].items():
    if name in OPT_OUT and not on:
        ext_flags.append(f"-DEXT_{name}_DISABLE")
    elif name in OPT_IN and on:
        ext_flags.append(f"-DEXT_{name}_ENABLE")
    elif name not in OPT_OUT | OPT_IN:
        raise SystemExit(f"unknown extension")


rows.append("EXT_FLAGS := " + " ".join(ext_flags))

## ADDRESS 

flags = [] 

flags.append(f"-DSTARTUP_ADDR={startup_addr:#x}") 
flags.append(f"-DSTACK_BASE_ADDR={stack_base_addr:#x}") 
flags.append(f"-DIO_MPM_ADDR={mpm_base:#x}") 
flags.append(f"-DIO_BASE_ADDR={io_base_addr:#x}") 
flags.append(f"-DIO_END_ADDR={io_end_addr:#x}") 

rows.append("KERN_ADDRESS_FLAGS := " + " ".join(flags))

flags = [] 

flags.append(f"-DSTARTUP_ADDR={startup_addr:#x}") 
flags.append(f"-DHEAP_OFFS={heap_offs:#x}") 
flags.append(f"-DHEAP_SIZE={heap_size:#x}") 
flags.append(f"-DMEM_BASE={mem_base:#x}") 
flags.append(f"-DMEM_SIZE={mem_size:#x}") 
flags.append(f"-DMPM_OFFS={mpm_offs:#x}") 
flags.append(f"-DMPM_SIZE={mpm_size:#x}") 
flags.append(f"-DKERNEL_SIZE={kernel_size:#x}") 

rows.append("ADDRESS_FLAGS := " + " ".join(flags)) 


## BLOCK DESIGN 

global_mem = cfg["global_mem"] 
rows.append(f"GLOBAL_MEM := {global_mem}") 
rows.append(f"MEM_BASE := {mem_base:#x}") 
rows.append(f"MEM_SIZE := {mem_size:#x}") 
rows.append(f"DCR_ADDR := {dcr_base:#x}") 
rows.append(f"DCR_SIZE := {dcr_size:#x}") 
rows.append(f"GPIO_ADDR := {gpio_base:#x}") 
rows.append(f"GPIO_SIZE := {gpio_size:#x}") 
rows.append(f"CONSOLE_ADDR := {console_base:#x}") 
rows.append(f"CONSOLE_SIZE := {console_size:#x}") 

var = ["GLOBAL_MEM", "MEM_BASE", "MEM_SIZE", "DCR_ADDR", "DCR_SIZE", "GPIO_ADDR", "GPIO_SIZE", "CONSOLE_ADDR", "CONSOLE_SIZE"] 

rows.append("BD_ENV = " + " ".join(f"{x}=$({x})" for x in var)) 



# Write 
body = "\n".join(rows) + "\n"
if makefile: 
	 with open(makefile, "w") as f: 
				f.write(body) 
else: 
	sys.stdout.write(body)






