// Copyright © 2019-2023
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#include <common.h>


#include <mem_alloc.h>

#include <arch.h>
#include <constants.h>
#include <mem.h>
#include <processor.h>
#include <util.h>

#include <assert.h>
#include <chrono>
#include <future>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>

#include <VX_config.h>
#ifdef VM_ENABLE
#include <malloc.h>

#include <VX_types.h>

#include <util.h>

#include <arch.h>
#include <array>
#include <cmath>
#include <constants.h>
#include <mem.h>
#include <processor.h>
#include <unordered_map>
#endif

#include <VX_config.h>
#include <VX_types.h>
#include <vortex.h>

using namespace vortex;


// at the moment 
// Stack size is 128 kb (good for 1 warp and 16 threads ad each thread uses 8 kb of stack) 
// Kernel size is max 128 kb 

// Current Memory Map 
// Kernel bytes from 0xA000_0000 to 0xA002_0000  (128 kb) 
// Stack from 0xA002_0000 to 0xA004_0000 (128 kb)
// Guard from 0xA004_0000 to 0xA004_1000 (check whether to remove)  (4 kb) 
// Heap from 0xA004_1000 to 0xA00F_FF00 
// MPM from 0xA00F_FF00 to 0xA010_0000 (256 byte)  (This is good just for 1 core -> 256 byte * NUM_CORES) 

#define BRAM_ADDRESS 0xA0000000
#define BRAM_SIZE (1 << 20)
#define STACK_SIZE (1 << 17) // 128 kb  
#define HEAP_GUARD (1 << 12) // 4 kb 
#define MPM_SIZE (1 << 8) // 256 byte 
#define HEAP_BASE (STACK_BASE_ADDR - BRAM_ADDRESS + HEAP_GUARD) 
#define HEAP_SIZE (BRAM_SIZE - HEAP_BASE - MPM_SIZE) 

#define PAGE_SIZE 4096
#define BLOCK_SIZE 64  


// VX_DEVICE

#define DEVICE_FILE_NAME "/dev/vortex"

struct dcr_ioctl {
	uint32_t dcr_data;
	uint32_t dcr_valaddr;
};

struct dcr_ioctl data_dcr;
#define RESET_BIT 13
#define VALID_BIT 12
#define BUSY_BIT 14

#define DCR_WRITE	_IOW('k', 'a', struct dcr_ioctl)
#define DCR_READ	_IOR('k', 'b', struct dcr_ioctl)

struct vx_device; 

struct vx_buffer{
	vx_device* dev; 
	uint64_t offset; 
	uint64_t size; 
	bool from_allocator = true; // true if allocation is managed from allocator
}; 

struct vx_device{
	int fd = -1; 
	MemoryAllocator* allocator = nullptr; 
	vx_buffer* mpm_buffer = nullptr; 
        uint64_t cycles = 0; 
	uint64_t exitcode = 0; 	
}; 




int vx_dev_open(vx_device_h* hdevice){
    int fd;
    fd = open(DEVICE_FILE_NAME, O_RDWR);
    
    if (fd < 0){
	fprintf(stderr, "[VORTEX LIB] Cannot open device\n"); 
	return -1;
    }
    

    auto device = new vx_device(); 
    device->fd = fd; 

    // Instantiating the Memory allocator
    // Notice: Base address is set to HEAP_BASE, so the allocator returns offset from bram base addr 
    // This has been done because the vortex driver write method requires offset and it adds automatically the bram base address 
    device->allocator = new MemoryAllocator(HEAP_BASE, HEAP_SIZE, PAGE_SIZE, BLOCK_SIZE); 
    device->mpm_buffer = new vx_buffer{device, BRAM_SIZE-MPM_SIZE, MPM_SIZE, false}; 
    

    *hdevice = device; 

    return 0;
}

int vx_dev_caps(vx_device_h hdevice, uint32_t caps_id, uint64_t* value) {
    if (nullptr == hdevice || nullptr == value)
        return -1;

    switch (caps_id) {
    case VX_CAPS_VERSION:         *value = 0; break;
    case VX_CAPS_NUM_THREADS:     *value = NUM_THREADS; break;
    case VX_CAPS_NUM_WARPS:       *value = NUM_WARPS; break;
    case VX_CAPS_NUM_CORES:       *value = NUM_CORES; break;
    case VX_CAPS_CACHE_LINE_SIZE: *value = MEM_BLOCK_SIZE; break;
    case VX_CAPS_GLOBAL_MEM_SIZE: *value = HEAP_SIZE; break;
    case VX_CAPS_LOCAL_MEM_SIZE:  *value = 0; break;   // LMEM_DISABLE
    case VX_CAPS_NUM_MEM_BANKS:   *value = 1; break;
    case VX_CAPS_MEM_BANK_SIZE:   *value = HEAP_SIZE; break;
    default:
        fprintf(stderr, "vx_dev_caps: unknown caps_id 0x%x\n", caps_id);
        return -1;
    }
    return 0;
}




int vx_dev_close(vx_device_h hdevice){
    if(nullptr == hdevice) return 0; 

    auto device = (vx_device *) hdevice; 
    vx_mem_free(device->mpm_buffer); 
    delete device->allocator; 

    close(device->fd);
   
    delete device;  
    return 0;
}



int vx_mem_access(vx_buffer_h hbuffer, uint64_t offset, uint64_t size, int flags){ 
    return 0; 
};



// reserve memory address range
int vx_mem_reserve(vx_device_h hdevice, uint64_t address, uint64_t size, int flags, vx_buffer_h* hbuffer){ 
    auto device = (vx_device*)hdevice; 


    uint64_t offs = address - BRAM_ADDRESS; 
    if((device->allocator)->reserve(offs, size) != 0){ 
	fprintf(stderr, "[VORTEX LIB] Memory Reservation failed\n");  
	return -1; 
    }; 
	
    auto buffer = new vx_buffer{device, offs, size}; 
    if(nullptr == buffer){
    	(device->allocator)->release(offs); 
	return -1; 
    }; 


    *hbuffer = buffer; 

    return 0; 
}; 



// allocate device memory and return address
int vx_mem_alloc(vx_device_h hdevice, uint64_t size, int flags, vx_buffer_h* hbuffer){
    auto device = (vx_device*)hdevice; 

    uint64_t offs = 0;
   
    if((device->allocator)->allocate(size, &offs) != 0){
	fprintf(stderr, "[VORTEX LIB] Memory Allocation failed\n");  
	return -1; 
    } 

    
    auto buffer = new vx_buffer{device, offs, size}; 
    if(nullptr == buffer){
	(device->allocator)->release(offs); 
	return -1; 
    }; 
    
    
    *hbuffer = buffer;  
   
    return 0;
}; 

// release device memory
int vx_mem_free(vx_buffer_h hbuffer) {
    if (nullptr == hbuffer) return 0; 
    auto buffer = (vx_buffer*) hbuffer; 
    int err = buffer->from_allocator ? (buffer->dev->allocator)->release(buffer->offset) : 0; 
    delete buffer; 
    return err; 
}; 



// return device memory address
int vx_mem_address(vx_buffer_h hbuffer, uint64_t* address){ 
	
    auto buffer = (vx_buffer*) hbuffer; 
    *address = buffer->offset + BRAM_ADDRESS; 

    return 0; 
}; 


int vx_copy_to_dev(vx_buffer_h hbuffer, const void* host_ptr, uint64_t dst_offset, uint64_t size){
    if(nullptr == hbuffer || nullptr == host_ptr){ 
	return -1; 
    }; 
	
    auto buffer = (vx_buffer*) hbuffer; 
    int fd = buffer->dev->fd; 


    if(dst_offset + size > buffer->size){ 
	fprintf(stderr, "[VORTEX LIB] Copy failed: Number of bytes required exceded Vortex buffer size\n"); 
	return -1; 
    } 

    pwrite(fd, host_ptr, size, buffer->offset + dst_offset);
    return 0;
}

int vx_copy_from_dev(void* host_ptr, vx_buffer_h hbuffer, uint64_t src_offset, uint64_t size){
    if(nullptr == hbuffer || nullptr == host_ptr){ 
	return -1; 
    }; 
    
    auto buffer = (vx_buffer*) hbuffer; 
    int fd = buffer->dev->fd; 

    if(src_offset + size > buffer->size){ 
	fprintf(stderr, "[VORTEX LIB] Copy failed: Number of bytes required exceded Vortex buffer size\n"); 
	return -1; 
    } 
    
    pread(fd, host_ptr, size, buffer->offset + src_offset);
    return 0;
}


int vx_start(vx_device_h hdevice, vx_buffer_h hkernel, vx_buffer_h harguments){
   
    auto device = (vx_device*)hdevice;  
    // azzeriamo i buffer dei performance monitor 
    uint64_t zeros[MPM_SIZE / 8] = {0};
    vx_copy_to_dev(device->mpm_buffer, zeros, 0, sizeof(zeros));

    auto kernel = (vx_buffer*) hkernel; 
    auto arguments = (vx_buffer*) harguments; 

    uint64_t kernel_addr = BRAM_ADDRESS + kernel->offset;
    uint64_t args_addr = BRAM_ADDRESS + arguments->offset; 


    uint32_t addr; 
    uint32_t value; 


    addr = 0x001; // VX_DCR_BASE_STARTUP_ADDR0
    value = kernel_addr & 0xffffffff;  
    vx_dcr_write(hdevice, addr, value); 
    
    
    addr = 0x002; // VX_DCR_BASE_STARTUP_ADDR1
    value = kernel_addr >> 32;
    vx_dcr_write(hdevice, addr, value);

    addr = 0x003; // VX_DCR_BASE_STARTUP_ARG0
    value = args_addr & 0xffffffff;  
    vx_dcr_write(hdevice, addr, value); 
    
    
    addr = 0x004; // VX_DCR_BASE_STARTUP_ARG0
    value = args_addr >> 32;
    vx_dcr_write(hdevice, addr, value);


    // Reset
    addr = 0;  // when addr = 0, it means where are deasserting reset and valid
    vx_dcr_write(hdevice, addr, 0); 
  
    
    
    return 0;
}

int vx_dcr_read(vx_device_h hdevice, uint32_t addr, uint32_t* value){
    auto device = (vx_device*) hdevice; 

    ioctl(device->fd, DCR_READ, &data_dcr);
   
    *value = data_dcr.dcr_valaddr; // at this moment, value is equivalent to val_addr, as we want to use dcr_read to read the busy bit 
   
    return 0;
}


int vx_dcr_write(vx_device_h hdevice, uint32_t addr, uint32_t value){
    auto device = (vx_device*) hdevice; 

    // first VALID = 0 and RESET = 1
    data_dcr.dcr_valaddr = 0x2000; 
    ioctl(device->fd, DCR_WRITE, &data_dcr); 

    // second VALID = 1 and RESET = 1 
    data_dcr.dcr_data = value; 
    data_dcr.dcr_valaddr = (0 != addr) ? ((1 << RESET_BIT) | (1 << VALID_BIT) | addr) : 0x0;      
    ioctl(device->fd, DCR_WRITE, &data_dcr);

    return 0;
}

// ADDED
int vx_kernel_stats(vx_device_h hdevice, uint64_t *c, uint64_t *e){ 
   if(nullptr == c) return -1; 

   auto device = (vx_device*)hdevice; 

   if(0 == device->cycles) return -1;
   
   *c = device->cycles; 
   
   if(nullptr != e) *e = device->exitcode; 

   return 0; 
}; 


// Wait for device ready with milliseconds timeout
int vx_ready_wait(vx_device_h hdevice, uint64_t timeout){
    auto device = (vx_device*)hdevice; 
    uint32_t val;
    
    bool is_done;  
    struct timespec sleep_time; 
    sleep_time.tv_sec = 0; 
    sleep_time.tv_nsec = 1000000; 

    while (1) {
        vx_dcr_read(hdevice, 0, &val);
    	vx_copy_from_dev(&device->cycles, device->mpm_buffer, 0x00, sizeof(device->cycles));
	
	is_done = !((val >> BUSY_BIT) & 1) && (device->cycles != 0); // Busy must return to 0 
	if (is_done) break;
        if(0 == timeout) {
	    return -1; 
	} 
        
	nanosleep(&sleep_time, nullptr); 
	timeout -= 1; // timeout is in millseconds 
    }
   

    vx_copy_from_dev(&device->exitcode, device->mpm_buffer, 0x08, sizeof(device->exitcode));
    
    return 0;
}; 


int vx_upload_kernel_bytes(vx_device_h hdevice, const void* content, uint64_t size, vx_buffer_h* hbuffer) {
  if (nullptr == hdevice || nullptr == content || size <= 16 || nullptr == hbuffer)
    return -1;
  
  auto bytes = reinterpret_cast<const uint64_t*>(content); 
  auto min_vma = *bytes++; 
  auto max_vma = *bytes++; 

  auto bin_size = size - 2*8; 
  auto runtime_size = (max_vma - min_vma); 

  if (min_vma != STARTUP_ADDR) {
	fprintf(stderr, "Kernel linked at 0x%lx, expected 0x%lx -- check STARTUP_ADDR\n", min_vma, (uint64_t)STARTUP_ADDR);
	return -1;
  }

  if (max_vma > STACK_BASE_ADDR - STACK_SIZE) {
	  fprintf(stderr, "Kernel ends at 0x%lx, stacks ends at 0x%lx\n", max_vma, (uint64_t)(STACK_BASE_ADDR - STACK_SIZE));
	  return -1;
   }


  auto device = (vx_device*)hdevice; 
  // Differently from stub, here we are not calling allocator anymore, just creating a new buffer
  auto buffer = new vx_buffer{device, STARTUP_ADDR - BRAM_ADDRESS, runtime_size, false}; 

  // Copying bytes to buffer 
  if(vx_copy_to_dev(buffer, bytes, 0, bin_size) != 0){
  	delete buffer; 
	return -1;  
  }; 

  *hbuffer = buffer;  
  
  return 0;
}


int vx_upload_bytes(vx_device_h hdevice, const void* content, uint64_t size, vx_buffer_h* hbuffer) {
  if (nullptr == hdevice || nullptr == content || 0 == size || nullptr == hbuffer)
    return -1;
  
  vx_buffer_h _hbuffer; 
  if(vx_mem_alloc(hdevice, size, VX_MEM_READ, &_hbuffer) != 0){ 
  	return -1; 
  }; 

  // Copying bytes to buffer 
  if(vx_copy_to_dev(_hbuffer, content, 0, size) != 0){
  	vx_mem_free(_hbuffer); 
	return -1;  
  }; 

  *hbuffer = _hbuffer;  
  return 0;
}




int vx_upload_kernel_file(vx_device_h hdevice, const char* filename, vx_buffer_h* hbuffer) {
if (nullptr == hdevice || nullptr == filename || nullptr == hbuffer)
    return -1;

  std::ifstream ifs(filename);
  if (!ifs) {
    std::cerr << "Error: " << filename << " not found" << std::endl;
    return -1;
  }

  // read file content
  ifs.seekg(0, ifs.end);
  auto size = ifs.tellg();
  std::vector<char> content(size);
  ifs.seekg(0, ifs.beg);
  ifs.read(content.data(), size);

  // upload buffer
  if(vx_upload_kernel_bytes(hdevice, content.data(), size, hbuffer) != 0){
	return -1; 	  
  };

  return 0;
}
