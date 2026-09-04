#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <vortex.h>
#include "common.h"

#define RT_CHECK(_expr)                                          \
   do {                                                          \
     int _ret = _expr;                                           \
     if (0 == _ret)                                              \
       break;                                                    \
     printf("Error: '%s' returned %d!\n", #_expr, (int)_ret);    \
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

  std::cout << "open device" << std::endl;
  RT_CHECK(vx_dev_open(&device));

  std::cout << "allocate destination buffer" << std::endl;
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ_WRITE, &dst_buffer));
  RT_CHECK(vx_mem_address(dst_buffer, &kernel_arg.dst_addr));

  std::cout << "Destination buffer: 0x" << std::hex << kernel_arg.dst_addr << std::endl << std::endl; 

  kernel_arg.magic      = MAGIC;
  kernel_arg.num_points = NUM_WORDS;

  // prefill with a sentinel so "the kernel wrote nothing" is distinguishable
  // from "the kernel wrote zeros"

  std::vector<uint32_t> host(NUM_WORDS, SENTINEL);
  RT_CHECK(vx_copy_to_dev(dst_buffer, host.data(), 0, buf_size));

  std::cout << "upload kernel: " << kernel_file << std::endl;
  RT_CHECK(vx_upload_kernel_file(device, kernel_file, &krnl_buffer));

  std::cout << "upload kernel arguments" << std::endl;
  RT_CHECK(vx_upload_bytes(device, &kernel_arg, sizeof(kernel_arg_t), &args_buffer));


  uint64_t krnl_addr = 0, args_addr = 0;
  
  RT_CHECK(vx_mem_address(krnl_buffer, &krnl_addr));
  std::cout << "Kernel address: 0x" << std::hex << krnl_addr << std::endl << std::endl; 
  
  RT_CHECK(vx_mem_address(args_buffer, &args_addr));
  std::cout << "Argument address: 0x" << std::hex << args_addr << std::endl << std::endl; 


  std::cout << "start" << std::endl;
  RT_CHECK(vx_start(device, krnl_buffer, args_buffer));

  std::cout << "wait for completion" << std::endl;
  RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

  std::cout << "read back results" << std::endl;
  std::memset(host.data(), 0, buf_size);
  RT_CHECK(vx_copy_from_dev(host.data(), dst_buffer, 0, buf_size));

  for (int i = 0; i < NUM_WORDS; ++i)
    std::cout << "  dst[" << i << "] = 0x" << std::hex << host[i] << std::dec << std::endl;

  int errors = 0;

  if (host[0] == SENTINEL) {
    std::cout << "FAIL: kernel never wrote to the buffer" << std::endl;
    ++errors;
  } else if (host[0] != KERNEL_TAG) {
    std::cout << "FAIL: dst[0] expected 0x" << std::hex << KERNEL_TAG << std::dec << std::endl;
    ++errors;
  }

  uint64_t seen_arg = ((uint64_t)host[2] << 32) | host[1];
  if (seen_arg != args_addr) {
    std::cout << "FAIL: MSCRATCH was 0x" << std::hex << seen_arg
              << ", expected 0x" << args_addr << std::dec
              << "  (STARTUP_ARG0/ARG1 not delivered correctly)" << std::endl;
    ++errors;
  }

  if (host[3] != MAGIC) {
    std::cout << "FAIL: magic read back as 0x" << std::hex << host[3]
              << ", expected 0x" << MAGIC << std::dec
              << "  (argument buffer contents wrong)" << std::endl;
    ++errors;
  }

  cleanup();

  if (errors == 0) {
    std::cout << "PASSED!" << std::endl;
    return 0;
  }
  std::cout << "FAILED! " << errors << " error(s)" << std::endl;
  return -1;
}
