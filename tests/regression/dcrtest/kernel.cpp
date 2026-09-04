#include <vx_intrinsics.h>
#include <vx_print.h>
#include <vx_spawn.h> 
#include "common.h"

void kernel_body(kernel_arg_t* __UNIFORM__ arg){
	vx_printf("IN kernel body \n"); 

	uint64_t a = (uint64_t)(uintptr_t)arg;
	uint32_t* dst = (uint32_t*)arg->dst_addr;

	dst[0] = KERNEL_TAG;                   // the kernel ran and stored
	dst[1] = (uint32_t) a;           // STARTUP_ARG0 arrived
	dst[2] = (uint32_t)(a >> 32);   // STARTUP_ARG1 arrived
	dst[3] = arg->magic;                   // the arg struct read back correctly

	vx_printf("kernel: magic=0x%x dst=0x%x\n",
		  arg->magic, (uint32_t)arg->dst_addr);
}

int main(){
	vx_printf(">> DCRTEST starting\n");
	
	kernel_arg_t *arg = (kernel_arg_t*)csr_read(VX_CSR_MSCRATCH); 
	vx_printf("Argument address = 0x%lx\n", arg);
	
	uint32_t num_threads = 1; 
	return vx_spawn_threads(1, &num_threads, nullptr, (vx_kernel_func_cb)kernel_body, arg);
}; 
