#include "alloc.h"
#include <stdint.h>
#include <stdio.h>
#include <stddef.h> 


// vortex 
#include <VX_config.h>
#include <VX_types.h>
#include <vortex.h>
#include <vx_driver.h> 

/* 
	Notice: Global Memory Map is automatically created by choosing two parameters (json config file) 
	1) Global Memory base address 
	2) Global Memory size 
	
	Memory map is created	as following from global mem base address: 
	- 128 kb for kernel and global variable (could be changed in json file) 
	- 4 kb protection guard between kernel and stack (could be changed in json file) 
	- Variable area for stack (calculated automatically from number of total threads) 
	- Heap area (variable because depends on stack and on mpm) 
	- Last 256 byte * NUM_CORES reserved to MPM perfomance monitor 

	Macros available here (passed through CXXFLAGS in hw/syn/xilinx/zcu102) 
	- HEAP_OFFS: heap offset from global memory base address
	- HEAP_SIZE
	- MPM_OFFS
	- MPM_SIZE
	- MEM_BASE
	- MEM_SIZE
	- STARTUP_ADDR
	- KERNEL_SIZE
*/ 


static vx_device dev; 

#define END_BIT 15
#define START_BIT 14
#define RESET_BIT 13
#define VALID_BIT 12



static void delay() {
  for(volatile int i = 0; i < 100000; i++) {}; 
} 

int vx_dev_open(vx_device_h* hdevice){
    
    if(hdevice == NULL) return -1;     
    if(VXDRV_OK != vortex_open(&dev)) return -1; 
    
    *hdevice = &dev;  
    return 0;
}

int vx_dev_caps(vx_device_h hdevice, uint32_t caps_id, uint64_t* value) {
    if (NULL == hdevice || NULL == value)
        return -1;

    switch (caps_id) {
    case VX_CAPS_VERSION:         *value = 0; break;
    case VX_CAPS_NUM_THREADS:     *value = NUM_THREADS; break;
    case VX_CAPS_NUM_WARPS:       *value = NUM_WARPS; break;
    case VX_CAPS_NUM_CORES:       *value = NUM_CORES; break;
    case VX_CAPS_CACHE_LINE_SIZE: *value = MEM_BLOCK_SIZE; break;
    case VX_CAPS_GLOBAL_MEM_SIZE: *value = HEAP_SIZE; break;
    case VX_CAPS_LOCAL_MEM_SIZE:  *value = 1 << LMEM_LOG_SIZE; break;  
    case VX_CAPS_NUM_MEM_BANKS:   *value = 1; break;
    case VX_CAPS_MEM_BANK_SIZE:   *value = HEAP_SIZE; break;
    default:
        printf("vx_dev_caps: unknown caps_id 0x%x\n", caps_id);
        return -1;
    }
    return 0;
}




int vx_dev_close(vx_device_h hdevice){
    vx_device *dev = (vx_device *) hdevice; 
    return vortex_close(dev);
}



int vx_mem_access(vx_buffer_h hbuffer, uint64_t offset, uint64_t size, int flags){ 
    return 0; 
};



// reserve memory address range
int vx_mem_reserve(vx_device_h hdevice, uint64_t address, uint64_t size, int flags, vx_buffer_h* hbuffer){ 
    return 0; 
}; 


// allocate device memory and return address
int vx_mem_alloc(vx_device_h hdevice, uint64_t size, int flags, vx_buffer_h* hbuffer){
    
    if (hbuffer == NULL || *hbuffer == NULL) return -1; 
  
    uint64_t offs = 0;
    
    if(alloc(size, &offs) != 0){
			printf("[VORTEX RUNTIME] Memory Allocation failed\n");  
			return -1; 
    } 

    uint64_t *p = (uint64_t*) (*hbuffer); 
    *p = offs; 
   
    return 0;
}; 

// release device memory
int vx_mem_free(vx_buffer_h hbuffer) {
    release(); 
    return 0; 
}; 



// return device memory address
int vx_mem_address(vx_buffer_h hbuffer, uint64_t* address){ 
	 
    *address = *(uint64_t *)(hbuffer) + MEM_BASE; 
    return 0; 
}; 


int vx_copy_to_dev(vx_buffer_h hbuffer, const void* host_ptr, uint64_t dst_offset, uint64_t size){
    
    if(NULL == hbuffer || NULL == host_ptr){ 
			return -1; 
    }; 

    uint64_t offset = dst_offset + *(uint64_t*)hbuffer; 

    return vortex_write(&dev, host_ptr, size, offset);  
}

int vx_copy_from_dev(void* host_ptr, vx_buffer_h hbuffer, uint64_t src_offset, uint64_t size){
    
    if(NULL == hbuffer || NULL == host_ptr){ 
			return -1; 
    }; 
    
    uint64_t offset = src_offset + *(uint64_t*)hbuffer; 
    return vortex_read(&dev, host_ptr, size, offset); 
}


int vx_start(vx_device_h hdevice, vx_buffer_h hkernel, vx_buffer_h harguments){

    // Zeroing MPM region    
    uint64_t zeros[MPM_SIZE / 8] = {0};
    if (VXDRV_OK != vortex_write(&dev, zeros, sizeof(zeros), MPM_OFFS)) return -1; 

      
    uint64_t kernel_addr = 0;
    uint64_t args_addr = 0; 
    
    // Obtaining kernel_address and arguments address
    vx_mem_address(hkernel, &kernel_addr); 
    vx_mem_address(harguments, &args_addr); 
    
    
    uint32_t addr = 0; 
    uint32_t value = 0; 

    // TODO : check barrier

    addr = VX_DCR_BASE_STARTUP_ADDR0; // 0x001 = VX_DCR_BASE_STARTUP_ADDR0
    value = kernel_addr & 0xffffffff;  
    vx_dcr_write(hdevice, addr, value); 
    
    
    addr = VX_DCR_BASE_STARTUP_ADDR1; // 0x002 = VX_DCR_BASE_STARTUP_ADDR1
    value = kernel_addr >> 32;
    vx_dcr_write(hdevice, addr, value);

    addr = VX_DCR_BASE_STARTUP_ARG0; // 0x003 = VX_DCR_BASE_STARTUP_ARG0
    value = args_addr & 0xffffffff;  
    vx_dcr_write(hdevice, addr, value); 
    
    addr = VX_DCR_BASE_STARTUP_ARG1; // 0x004 = VX_DCR_BASE_STARTUP_ARG0
    value = args_addr >> 32;
    vx_dcr_write(hdevice, addr, value);

#ifdef TEST 
		// Set Monitor MPM class 
		addr = VX_DCR_BASE_MPM_CLASS;  // 0x005 = VX_DCR_BASE_MPM_CLASS
		value = VX_DCR_MPM_CLASS_MEM; // 2 
		vx_dcr_write(hdevice, addr, value); 
#endif 

    // Reset
    addr = 0;  // when addr = 0, it means where are deasserting reset and valid
    vx_dcr_write(hdevice, addr, 0); 
 

		uint8_t is_started = 0;    
		uint32_t val; 		
		uint64_t timeout = 1000;  
    
		while (1) {
        vx_dcr_read(hdevice, addr, &val);
				is_started = (val >> START_BIT) & 1; // Start must return 1 
				if (is_started) break;
        if(0 == timeout) {
	    			return -1; 
				} 
    
        delay(); 
        timeout -= 1;     
    }
    
    return 0;
}

int vx_dcr_read(vx_device_h hdevice, uint32_t addr, uint32_t* value){

    vx_device *dev = (vx_device*) hdevice; 
         
    if(VXDRV_OK != vortex_readDCR(dev,&addr,value)) return -1;
    return 0;
}


int vx_dcr_write(vx_device_h hdevice, uint32_t addr, uint32_t value){
    vx_device *dev = (vx_device*) hdevice; 

    if(addr == 0) { 
      if(VXDRV_OK != vortex_writeDCR(dev, 0, 0)) return -1;
      return 0; 
    } 

    uint32_t valaddr = 0; 

    // VALID = 1, RESET = 1 
    valaddr = (1 << RESET_BIT) | (1 << VALID_BIT) | addr; 

    if(VXDRV_OK != vortex_writeDCR(dev, value, valaddr)) return -1;
    
    // VALID = 0, RESET = 1 
    
    valaddr = (1 << RESET_BIT) | addr;  
    if(VXDRV_OK != vortex_writeDCR(dev, value, valaddr)) return -1;
    
    return 0;
}


// Wait for device ready with milliseconds timeout
int vx_ready_wait(vx_device_h hdevice, uint64_t timeout){
    uint32_t val;
    
    uint8_t is_ended = 0;  
   
    // PORT: no timespec -> timer 

    while (1) {
        vx_dcr_read(hdevice, 0, &val);
				is_ended = (val >> END_BIT) & 1; // end bit  must return 1 
				if (is_ended) break;
        if(0 == timeout) {
	    			return -1; 
				} 
    
				delay(); 
        timeout -= 1; // timeout is in millseconds 
    }

    return 0;
}; 

// query device performance counter
int vx_mpm_query(vx_device_h hdevice, uint32_t addr, uint32_t core_id, uint64_t* value){
    
		vx_device *dev = (vx_device*)hdevice;

		uint32_t mpm_index = addr - VX_CSR_MPM_BASE;
    if (mpm_index > 31)
      return -1;
    
    uint64_t offset = MPM_OFFS + core_id * 32 * sizeof(uint64_t) + mpm_index * sizeof(uint64_t); 
      
    if(VXDRV_OK != vortex_read(dev, value, sizeof(uint64_t), offset)) return -1; 
		
    return 0;
}




int vx_upload_kernel_bytes(vx_device_h hdevice, const void* content, uint64_t size, vx_buffer_h* hbuffer) {
  
  if (hdevice == NULL || content == NULL || size <= 16 || hbuffer == NULL || *hbuffer == NULL)
        return -1;

  vx_device *dev = (vx_device *)hdevice;
  const uint64_t *hdr = (const uint64_t *)content;
  uint64_t min_vma = hdr[0];
  uint64_t max_vma = hdr[1];

  if (min_vma != STARTUP_ADDR) return -1;               /* non è un .vxbin per questo design */
  if (max_vma > STARTUP_ADDR + KERNEL_SIZE) return -1;  /* kernel troppo grande */

  const uint8_t *payload  = (const uint8_t *) content + 16;
  uint64_t  payload_size = size - 16;                 
  uint64_t  offs = STARTUP_ADDR - MEM_BASE;

  *(uint64_t*) *hbuffer = offs; 
  if (VXDRV_OK != vortex_write(dev, payload, payload_size, offs)) return -1; 
  printf("[VORTEX LIB] Kernel successfully copied\n");
  
  return 0;
}


int vx_upload_bytes(vx_device_h hdevice, const void* content, uint64_t size, vx_buffer_h* hbuffer) {

  if (NULL == hdevice || NULL == *hbuffer) return -1; 

  if(0 != vx_mem_alloc(hdevice, size, VX_MEM_READ, hbuffer)) return -1; 
  
  uint64_t *p = (uint64_t *) (*hbuffer);  
  vx_device *dev = (vx_device*) hdevice;
  if (VXDRV_OK != vortex_write(dev, content, size, *p)) return -1;

  return 0; 
}


int vx_upload_kernel_file(vx_device_h hdevice, const char* filename, vx_buffer_h* hbuffer) {
  /* PORT: No fstream operations */ 
  return 0;
}
