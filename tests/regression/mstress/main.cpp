#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vortex.h>
#include "common.h"
#include <assert.h>
#include <limits>
#include <math.h>
#include <vector>
#include <VX_types.h> 
#include <chrono> 

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
     fprintf(stderr, "Error: '%s' returned %d!\n", #_expr, (int)_ret); \
     cleanup();                                                 \
     exit(-1);                                                  \
   } while (false)

///////////////////////////////////////////////////////////////////////////////

union Float_t {
    float f;
    int   i;
    struct {
        uint32_t man  : 23;
        uint32_t exp  : 8;
        uint32_t sign : 1;
    } parts;
};

inline float fround(float x, int32_t precision = 8) {
  auto power_of_10 = std::pow(10, precision);
  return std::round(x * power_of_10) / power_of_10;
}

inline bool almost_equal_eps(float a, float b, int ulp = 128) {
  auto eps = std::numeric_limits<float>::epsilon() *
             (std::max(fabs(a), fabs(b)) * ulp);

  auto d = fabs(a - b);

  if (d > eps) {
    fprintf(stderr,
            "*** almost_equal_eps: d=%g, eps=%g\n",
            d, eps);
    return false;
  }

  return true;
}

inline bool almost_equal_ulp(float a, float b, int32_t ulp = 6) {
  Float_t fa{a}, fb{b};

  auto d = std::abs(fa.i - fb.i);

  if (d > ulp) {
    fprintf(stderr,
            "*** almost_equal_ulp: a=%f, b=%f, ulp=%d, ia=%x, ib=%x\n",
            a, b, d, fa.i, fb.i);
    return false;
  }

  return true;
}

inline bool almost_equal(float a, float b) {
  if (a == b)
    return true;

  /*if (almost_equal_eps(a, b))
    return true;*/

  return almost_equal_ulp(a, b);
}

///////////////////////////////////////////////////////////////////////////////

const char* kernel_file = "kernel.vxbin";
uint32_t count = 0;

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
    vx_mem_free(src0_buffer);
    vx_mem_free(src1_buffer);
    vx_mem_free(dst_buffer);
    vx_mem_free(krnl_buffer);
    vx_mem_free(args_buffer);
    vx_dev_close(device);
  }
}

void gen_src_data(std::vector<float>& test_data,
                  std::vector<uint32_t>& addr_table,
                  uint32_t num_points,
                  uint32_t num_addrs) {
  test_data.resize(num_points);
  addr_table.resize(num_addrs);

  for (uint32_t i = 0; i < num_points; ++i) {
    float r = static_cast<float>(std::rand()) / RAND_MAX;
    test_data[i] = r;
  }

  for (uint32_t i = 0; i < num_addrs; ++i) {
    float r = static_cast<float>(std::rand()) / RAND_MAX;
    uint32_t index = static_cast<uint32_t>(r * num_points);
    assert(index < num_points);
    addr_table[i] = index;
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

  uint64_t num_cores;
  uint64_t num_warps;
  uint64_t num_threads;

  RT_CHECK(vx_dev_caps(device, VX_CAPS_NUM_CORES, &num_cores));
  RT_CHECK(vx_dev_caps(device, VX_CAPS_NUM_WARPS, &num_warps));
  RT_CHECK(vx_dev_caps(device, VX_CAPS_NUM_THREADS, &num_threads));

  uint32_t total_threads = num_cores * num_warps * num_threads;
  uint32_t num_points = count * total_threads;
  uint32_t num_addrs = num_points + NUM_LOADS - 1;

  uint32_t addr_buf_size = num_addrs * sizeof(int32_t);
  uint32_t src_buf_size = num_points * sizeof(int32_t);
  uint32_t dst_buf_size = num_points * sizeof(int32_t);

  PRINT("number of points: " << num_points);
  PRINT("addr buffer size: " << addr_buf_size << " bytes");
  PRINT("src buffer size: " << src_buf_size << " bytes");
  PRINT("dst buffer size: " << dst_buf_size << " bytes");

  kernel_arg.num_tasks = total_threads;
  kernel_arg.stride = count;

  // allocate device memory
  PRINT("allocate device memory");

  RT_CHECK(vx_mem_alloc(device, addr_buf_size, VX_MEM_READ, &src0_buffer));
  RT_CHECK(vx_mem_address(src0_buffer, &kernel_arg.src0_addr));
  RT_CHECK(vx_mem_alloc(device, src_buf_size, VX_MEM_READ, &src1_buffer));
  RT_CHECK(vx_mem_address(src1_buffer, &kernel_arg.src1_addr));
  RT_CHECK(vx_mem_alloc(device,dst_buf_size, VX_MEM_WRITE, &dst_buffer));
  RT_CHECK(vx_mem_address(dst_buffer, &kernel_arg.dst_addr));

  PRINT("dev_addr=0x" << std::hex << kernel_arg.src0_addr);
  PRINT("dev_src=0x" << std::hex << kernel_arg.src1_addr);
  PRINT("dev_dst=0x" << std::hex << kernel_arg.dst_addr);

  std::vector<uint32_t> h_addr;
  std::vector<float> h_src;
  std::vector<float> h_dst(num_points);

  gen_src_data(h_src, h_addr, num_points, num_addrs);

  auto t1 = std::chrono::high_resolution_clock::now();

  // upload source buffer0
  PRINT("upload address buffer");

  RT_CHECK(vx_copy_to_dev(src0_buffer,
                          h_addr.data(),
                          0,
                          addr_buf_size));

  // upload source buffer1
  PRINT("upload source buffer");

  RT_CHECK(vx_copy_to_dev(src1_buffer,
                          h_src.data(),
                          0,
                          src_buf_size));

  auto t2 = std::chrono::high_resolution_clock::now();



  // Upload kernel binary
  PRINT("Upload kernel binary");

  RT_CHECK(vx_upload_kernel_file(device,
                                 kernel_file,
                                 &krnl_buffer));

  // upload kernel argument
  PRINT("upload kernel argument");

  RT_CHECK(vx_upload_bytes(device,
                           &kernel_arg,
                           sizeof(kernel_arg_t),
                           &args_buffer));

  // start device
  PRINT("start device");

  RT_CHECK(vx_start(device,
                    krnl_buffer,
                    args_buffer));

  // wait for completion
  PRINT("wait for completion");

  RT_CHECK(vx_ready_wait(device,
                         VX_MAX_TIMEOUT));

  // download destination buffer
  PRINT("download destination buffer");

  auto t3 = std::chrono::high_resolution_clock::now();


  RT_CHECK(vx_copy_from_dev(h_dst.data(),
                            dst_buffer,
                            0,
                            dst_buf_size));

  auto t4 = std::chrono::high_resolution_clock::now();

  // verify result
  PRINT("verify result");

  int errors = 0;

  for (uint32_t i = 0; i < num_points; ++i) {
    float ref = 0.0f;

    for (uint32_t j = 0; j < NUM_LOADS; ++j) {
      uint32_t addr = i + j;
      uint32_t index = h_addr[addr];
      float value = h_src[index];

      //printf("*** [%d] addr=%d, index=%d, value=%f\n",
      //       i, addr, index, value);

      ref *= value;
    }

    float cur = h_dst[i];

    if (!almost_equal(cur, ref)) {
      if (errors < 100) {
        fprintf(stderr,
                "error at result #%u: actual %f, expected %f\n",
                i, cur, ref);
      }

      ++errors;
    }
  }

  if (errors != 0) {
    PRINT("Found " << std::dec << errors << " errors!");
    PRINT("FAILED!");

    cleanup();
    return 1;
  }

  PRINT("PASSED!");

#ifdef TEST
  // In testing mode, print only the performance counters on stdout
  // so they can be recovered from bash.

  auto t_upload = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
  auto t_download = std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count();

  uint64_t mcycles = 0;
  uint64_t minstret = 0;

  MPM_QUERY(device,
            VX_CSR_MCYCLE,
            mcycles);

  MPM_QUERY(device,
            VX_CSR_MINSTRET,
            minstret);

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
            << "mstress" << ","
            << count << ","
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
