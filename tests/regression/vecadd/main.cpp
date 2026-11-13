#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <vortex.h>
#include "common.h"
#include <VX_types.h>
#include <chrono>

#define FLOAT_ULP 6

#ifdef TEST
#define PRINT(x) ((void)0) // NOP 
#else
#define PRINT(x) do { std::cout << x << std::endl; } while (false)
#endif

#define MPM_QUERY(device, csr, value) \
    do { \
        if (0 != vx_mpm_query((device), (csr), 0, &(value))) { \
            fprintf(stderr, "mpm_query failed for %s\n", #csr); \
            cleanup(); \
            return -1; \
        } \
    } while (false)


#define RT_CHECK(_expr)                                         \
   do {                                                         \
     int _ret = _expr;                                          \
     if (0 == _ret)                                             \
       break;                                                   \
     fprintf(stderr, "Error: '%s' returned %d!\n", #_expr, (int)_ret);   \
     cleanup();			                                              \
     exit(-1);                                                  \
   } while (false)

///////////////////////////////////////////////////////////////////////////////

template <typename Type>
class Comparator {};

template <>
class Comparator<int> {
public:
  static const char* type_str() {
    return "integer";
  }
  static int generate() {
    return rand();
  }
  static bool compare(int a, int b, int index, int errors) {
    if (a != b) {
      if (errors < 100) {
        fprintf(stderr,"*** error: [%d] expected=%d, actual=%d\n", index, b, a);
      }
      return false;
    }
    return true;
  }
};

template <>
class Comparator<float> {
private:
  union Float_t { float f; int i; };
public:
  static const char* type_str() {
    return "float";
  }
  static float generate() {
    return static_cast<float>(rand()) / RAND_MAX;
  }
  static bool compare(float a, float b, int index, int errors) {
    union fi_t { float f; int32_t i; };
    fi_t fa, fb;
    fa.f = a;
    fb.f = b;
    auto d = std::abs(fa.i - fb.i);
    if (d > FLOAT_ULP) {
      if (errors < 100) {
        fprintf(stderr, "*** error: [%d] expected=%f, actual=%f\n", index, b, a);
      }
      return false;
    }
    return true;
  }
};

const char* kernel_file = "kernel.vxbin";
uint32_t size = 16;

vx_device_h device = nullptr;
vx_buffer_h src0_buffer = nullptr;
vx_buffer_h src1_buffer = nullptr;
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
      size = atoi(optarg);
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
    vx_mem_free(src0_buffer);
    vx_mem_free(src1_buffer);
    vx_mem_free(dst_buffer);
    vx_mem_free(krnl_buffer);
    vx_mem_free(args_buffer);
    vx_dev_close(device);
  }
}

int main(int argc, char *argv[]) {
  // parse command arguments
  parse_args(argc, argv);

  std::srand(50);

  // open device connection
  PRINT("open device connection");
  RT_CHECK(vx_dev_open(&device));

  uint32_t num_points = size;
  uint32_t buf_size = num_points * sizeof(TYPE);

  PRINT("number of points: " << num_points);
  PRINT("data type: " << Comparator<TYPE>::type_str());
  PRINT("buffer size: " << buf_size << " bytes");

  kernel_arg.num_points = num_points;

  // allocate device memory
  PRINT("allocate device memory"); 
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ, &src0_buffer));
  RT_CHECK(vx_mem_address(src0_buffer, &kernel_arg.src0_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ, &src1_buffer));
  RT_CHECK(vx_mem_address(src1_buffer, &kernel_arg.src1_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_WRITE, &dst_buffer));
  RT_CHECK(vx_mem_address(dst_buffer, &kernel_arg.dst_addr));

  PRINT("dev_src0=0x" << std::hex << kernel_arg.src0_addr); 
  PRINT("dev_src1=0x" << std::hex << kernel_arg.src1_addr);
  PRINT("dev_dst=0x" << std::hex << kernel_arg.dst_addr); 

  // allocate host buffers
  PRINT("allocate host buffers");
  std::vector<TYPE> h_src0(num_points);
  std::vector<TYPE> h_src1(num_points);
  std::vector<TYPE> h_dst(num_points);

  for (uint32_t i = 0; i < num_points; ++i) {
    h_src0[i] = Comparator<TYPE>::generate();
    h_src1[i] = Comparator<TYPE>::generate();
  }

  auto t1 = std::chrono::high_resolution_clock::now();

  // upload source buffer0
  PRINT("upload source buffer0");
  RT_CHECK(vx_copy_to_dev(src0_buffer, h_src0.data(), 0, buf_size));

  // upload source buffer1
  PRINT("upload source buffer1");
  RT_CHECK(vx_copy_to_dev(src1_buffer, h_src1.data(), 0, buf_size));

  auto t2 = std::chrono::high_resolution_clock::now();

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

  auto t3 = std::chrono::high_resolution_clock::now();

  // download destination buffer
  PRINT("download destination buffer");
  RT_CHECK(vx_copy_from_dev(h_dst.data(), dst_buffer, 0, buf_size));

  auto t4 = std::chrono::high_resolution_clock::now();


  // verify result
  PRINT("verify result");
  int errors = 0;
   for (uint32_t i = 0; i < num_points; ++i) {
	auto ref = h_src0[i] + h_src1[i];
	auto cur = h_dst[i];
	  if (!Comparator<TYPE>::compare(cur, ref, i, errors)) {
	    ++errors;
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
   // IN case we are in testing, we print just the perfomance counters on stdout, so we can recover them from bash

  auto t_upload = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
  auto t_download = std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count();

  uint64_t mcycles = 0;
	uint64_t minstret = 0; 

	MPM_QUERY(device, VX_CSR_MCYCLE, mcycles); 
	MPM_QUERY(device, VX_CSR_MINSTRET, minstret); 

  uint64_t gmem_tot_pending_req = 0;  
  uint64_t gmem_num_reads = 0;  
  uint64_t gmem_num_writes = 0; 
  uint64_t dcache_miss_r = 0;
  uint64_t dcache_num_reads = 0;   


  MPM_QUERY(device, VX_CSR_MPM_DCACHE_READS, dcache_num_reads);
  MPM_QUERY(device, VX_CSR_MPM_DCACHE_MISS_R, dcache_miss_r);
  MPM_QUERY(device, VX_CSR_MPM_MEM_READS, gmem_num_reads);
  MPM_QUERY(device, VX_CSR_MPM_MEM_WRITES, gmem_num_writes);
  MPM_QUERY(device, VX_CSR_MPM_MEM_LT, gmem_tot_pending_req);


  std::cout << std::dec
            << "vecadd" << ","
            << size << ","
            << mcycles << ","
            << minstret << ","
            << gmem_num_reads << ","
            << gmem_num_writes << ","
            << gmem_tot_pending_req << ","
            << dcache_miss_r << "," 
            << dcache_num_reads << ","
            << t_upload << "," 
            << t_download 
            << std::endl;       


#endif


  cleanup();

  return 0;
} 
