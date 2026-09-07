#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vortex.h>
#include <vector>
#include "common.h"


#ifdef TEST
#define PRINT(x) ((void)0) // NOP 
#else
#define PRINT(x) do { std::cout << x << std::endl; } while (false)
#endif


#define RT_CHECK(_expr)                                         \
   do {                                                         \
     int _ret = _expr;                                          \
     if (0 == _ret)                                             \
       break;                                                   \
     fprintf(stderr,"Error: '%s' returned %d!\n", #_expr, (int)_ret);   \
	   cleanup();			                                              \
     exit(-1);                                                  \
   } while (false)

///////////////////////////////////////////////////////////////////////////////

const char* kernel_file = "kernel.vxbin";
uint32_t count = 0;

vx_device_h device = nullptr;
vx_buffer_h src_buffer = nullptr;
vx_buffer_h dst_buffer = nullptr;
vx_buffer_h krnl_buffer = nullptr;
vx_buffer_h args_buffer = nullptr;
kernel_arg_t kernel_arg = {};

static void show_usage() {
   std::cout << "Vortex Test." << std::endl;
   std::cout << "Usage: [-k: kernel] [-n words] [-h: help]" << std::endl;
}

static void parse_args(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "n:k:h")) != -1) {
    switch (c) {
    case 'n':
      count = atoi(optarg);
      break;
    case 'k':
      kernel_file = optarg;
      break;
    case 'h':
      show_usage();
      exit(0);
      break;
    default:
      show_usage();
      exit(-1);
    }
  }
}

void cleanup() {
  if (device) {
    vx_mem_free(src_buffer);
    vx_mem_free(dst_buffer);
    vx_mem_free(krnl_buffer);
    vx_mem_free(args_buffer);
    vx_dev_close(device);
  }
}

void gen_src_data(std::vector<TYPE>& src_data, uint32_t size) {
  src_data.resize(size);
  for (uint32_t i = 0; i < size; ++i) {
    auto r = static_cast<float>(std::rand()) / RAND_MAX;
    auto value = static_cast<TYPE>(r * size);
    src_data[i] = value;
    //std::cout << std::dec << i << ": value=" << value << std::endl;
  }
}

void gen_ref_data(std::vector<TYPE>& ref_data, const std::vector<TYPE>& src_data, uint32_t size) {
  ref_data.resize(size);
  for (uint32_t i = 0; i < size; ++i) {
    TYPE ref_value = src_data.at(i);
    uint32_t pos = 0;
    for (uint32_t j = 0; j < size; ++j) {
      TYPE cur_value = src_data.at(j);
      pos += (cur_value < ref_value) || (cur_value == ref_value && j < i);
    }
    ref_data.at(pos) = ref_value;
  }
}

int main(int argc, char *argv[]) {
  // parse command arguments
  parse_args(argc, argv);

  if (count == 0) {
    count = 1;
  }

  std::srand(50);
  
  // open device connection
  PRINT("open device connection");
  RT_CHECK(vx_dev_open(&device));

  uint64_t num_cores, num_warps, num_threads;
  RT_CHECK(vx_dev_caps(device, VX_CAPS_NUM_CORES, &num_cores));
  RT_CHECK(vx_dev_caps(device, VX_CAPS_NUM_WARPS, &num_warps));
  RT_CHECK(vx_dev_caps(device, VX_CAPS_NUM_THREADS, &num_threads));

  uint32_t total_threads = num_cores * num_warps * num_threads;
  uint32_t num_points = count * total_threads;
  uint32_t buf_size   = num_points * sizeof(TYPE);

  PRINT("number of points: " << num_points);
  PRINT("buffer size: " << buf_size << " bytes");

  kernel_arg.num_points = num_points;

  // allocate device memory
  PRINT("allocate device memory");
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ, &src_buffer));
  RT_CHECK(vx_mem_address(src_buffer, &kernel_arg.src_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_WRITE, &dst_buffer));
  RT_CHECK(vx_mem_address(dst_buffer, &kernel_arg.dst_addr));

  PRINT("dev_src=0x" << std::hex << kernel_arg.src_addr);
  PRINT("dev_dst=0x" << std::hex << kernel_arg.dst_addr);

  // allocate host buffers
  PRINT("allocate host buffers");
  std::vector<TYPE> h_src;
  std::vector<TYPE> h_dst(num_points);
  gen_src_data(h_src, num_points);

  // upload source buffer
  PRINT("upload source buffer");
  RT_CHECK(vx_copy_to_dev(src_buffer, h_src.data(), 0, buf_size));

  // Upload kernel binary
  PRINT("Upload kernel binary");
  RT_CHECK(vx_upload_kernel_file(device, kernel_file, &krnl_buffer));

  // upload kernel argument
  PRINT("upload kernel argument");
  RT_CHECK(vx_upload_bytes(device, &kernel_arg, sizeof(kernel_arg_t), &args_buffer));

  // start device
  PRINT("start device");
  RT_CHECK(vx_start(device, krnl_buffer, args_buffer));

  // wait for completion
  PRINT("wait for completion");
  RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

  // download destination buffer
  PRINT("download destination buffer");
  RT_CHECK(vx_copy_from_dev(h_dst.data(), dst_buffer, 0, buf_size));

  // verify result
  PRINT("verify result");
  int errors = 0;
  {
    std::vector<TYPE> h_ref;
    gen_ref_data(h_ref, h_src, num_points);

    for (uint32_t i = 0; i < num_points; ++i) {
      TYPE ref = h_ref[i];
      TYPE cur = h_dst[i];

      if (cur != ref) {
        PRINT("error at result #" << std::dec << i
              << std::hex << ": actual=" << cur << ", expected=" << ref);
        ++errors;
      }
    }
  }


  if (errors != 0) {
    PRINT("Found " << std::dec << errors << " errors!");
    PRINT("FAILED!");
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
