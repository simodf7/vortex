#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <vortex.h>
#include "common.h"

#ifdef TEST
#define PRINT(x) ((void)0) // NOP 
#else
#define PRINT(x) do { std::cout << x << std::endl; } while (false)
#endif


#define RT_CHECK(_expr)                                          \
   do {                                                          \
     int _ret = _expr;                                           \
     if (0 == _ret)                                              \
       break;                                                    \
     fprintf(stderr, "Error: '%s' returned %d!\n", #_expr, (int)_ret);    \
     cleanup();                                                  \
     exit(-1);                                                   \
   } while (false)

static vx_device_h device      = nullptr;
static vx_buffer_h krnl_buffer = nullptr;
static vx_buffer_h args_buffer = nullptr;
static vx_buffer_h dst_buffer  = nullptr;

static void cleanup() {
  if (dst_buffer)  vx_mem_free(dst_buffer);
  if (args_buffer) vx_mem_free(args_buffer);
  if (krnl_buffer) vx_mem_free(krnl_buffer);
  if (device)      vx_dev_close(device);
}

int main(int argc, char* argv[]) {
  const char* kernel_file = (argc > 1) ? argv[1] : "kernel.vxbin";
  
  kernel_arg_t kernel_arg = {};
  const uint64_t buf_size = NUM_WORDS * sizeof(uint32_t);


  PRINT("open device"); 
  RT_CHECK(vx_dev_open(&device));

  PRINT("allocate destination buffer"); 
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ_WRITE, &dst_buffer));
  RT_CHECK(vx_mem_address(dst_buffer, &kernel_arg.dst_addr));

  PRINT("Destination buffer: 0x" << std::hex << kernel_arg.dst_addr);  

  kernel_arg.magic      = MAGIC;
  kernel_arg.num_points = NUM_WORDS;

  // prefill with a sentinel so "the kernel wrote nothing" is distinguishable
  // from "the kernel wrote zeros"

  std::vector<uint32_t> host(NUM_WORDS, SENTINEL);
  RT_CHECK(vx_copy_to_dev(dst_buffer, host.data(), 0, buf_size));


  PRINT("upload kernel: " << kernel_file); 
  RT_CHECK(vx_upload_kernel_file(device, kernel_file, &krnl_buffer));

  PRINT("upload kernel arguments"); 
  RT_CHECK(vx_upload_bytes(device, &kernel_arg, sizeof(kernel_arg_t), &args_buffer));


  uint64_t krnl_addr = 0, args_addr = 0;
  
  RT_CHECK(vx_mem_address(krnl_buffer, &krnl_addr));
  PRINT("Kernel address: 0x" << std::hex << krnl_addr);  
  
  RT_CHECK(vx_mem_address(args_buffer, &args_addr));
  PRINT("Argument address: 0x" << std::hex << args_addr);  


  PRINT("start"); 
  RT_CHECK(vx_start(device, krnl_buffer, args_buffer));

  PRINT("wait for completion"); 
  RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

  PRINT("read back results"); 
  std::memset(host.data(), 0, buf_size);
  RT_CHECK(vx_copy_from_dev(host.data(), dst_buffer, 0, buf_size));

  for (int i = 0; i < NUM_WORDS; ++i)
    PRINT("  dst[" << i << "] = 0x" << std::hex << host[i] << std::dec); 

  int errors = 0;

  if (host[0] == SENTINEL) {
    PRINT("FAIL: kernel never wrote to the buffer");
    ++errors;
  } else if (host[0] != KERNEL_TAG) {
    PRINT("FAIL: dst[0] expected 0x" << std::hex << KERNEL_TAG << std::dec);
    ++errors;
  }

  uint64_t seen_arg = ((uint64_t)host[2] << 32) | host[1];
  if (seen_arg != args_addr) {
    PRINT("FAIL: MSCRATCH was 0x" << std::hex << seen_arg
              << ", expected 0x" << args_addr << std::dec
              << "  (STARTUP_ARG0/ARG1 not delivered correctly)");
    ++errors;
  }

  if (host[3] != MAGIC) {
    PRINT("FAIL: magic read back as 0x" << std::hex << host[3]
              << ", expected 0x" << MAGIC << std::dec
              << "  (argument buffer contents wrong)");
    ++errors;
  }


  
  if (errors != 0) { 
  	PRINT("FAILED! " << errors << " error(s)"); 
	cleanup();
	return -1; 
  }

   PRINT("PASSED!"); 

#ifdef TEST
   // IN case we are in testing, this will be the only value printed on stdout, so we can recover it from bash 
   uint64_t cycles = 0;
   if( 0 == vx_kernel_stats(device, &cycles, nullptr)) std::cout << cycles << std::endl; 
#endif 


  cleanup();
  
  return 0;
}
