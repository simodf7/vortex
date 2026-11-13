#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <vortex.h>
#include <cmath>
#include "common.h"
#include <VX_types.h>

#define FLOAT_ULP 6

#ifdef TEST
#define PRINT(x) ((void)0)
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
        fprintf(stderr, "*** error: [%d] expected=%d, actual=%d\n", index, b, a);
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

static void matmul_cpu(TYPE* out, const TYPE* A, const TYPE* B, uint32_t width, uint32_t height) {
  for (uint32_t row = 0; row < height; ++row) {
    for (uint32_t col = 0; col < width; ++col) {
      TYPE sum(0);
      for (uint32_t e = 0; e < width; ++e) {
        TYPE a = A[row * width + e];
        TYPE b = B[e * width + col];
        TYPE c = a * b;
        sum += c;
      }
      out[row * width + col] = sum;
    }
  }
}

const char* kernel_file = "kernel.vxbin";
uint32_t size = 16;
uint32_t tile_size = 4;

vx_device_h device = nullptr;
vx_buffer_h A_buffer = nullptr;
vx_buffer_h B_buffer = nullptr;
vx_buffer_h C_buffer = nullptr;
vx_buffer_h krnl_buffer = nullptr;
vx_buffer_h args_buffer = nullptr;
kernel_arg_t kernel_arg = {};

static void show_usage() {
   std::cout << "Vortex Test." << std::endl;
   std::cout << "Usage: [-k: kernel] [-n matrix_size] [-t:tile_size] [-h: help]" << std::endl;
}

static void parse_args(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "n:t:k:h")) != -1) {
    switch (c) {
    case 'n':
      size = atoi(optarg);
      break;
    case 't':
      tile_size = atoi(optarg);
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
    vx_mem_free(A_buffer);
    vx_mem_free(B_buffer);
    vx_mem_free(C_buffer);
    vx_mem_free(krnl_buffer);
    vx_mem_free(args_buffer);
    vx_dev_close(device);
  }
}

int main(int argc, char *argv[]) {
  // parse command arguments
  parse_args(argc, argv);

  if ((size / tile_size) * tile_size != size) {
    printf("Error: matrix size %d must be a multiple of tile size %d\n", size, tile_size);
    return -1;
  }

  std::srand(50);

  PRINT("open device connection");
  RT_CHECK(vx_dev_open(&device));

  uint32_t size_sq = size * size;
  uint32_t buf_size = size_sq * sizeof(TYPE);
  uint32_t group_size = tile_size * tile_size;
  uint32_t local_mem = 2 * group_size * sizeof(TYPE);

  PRINT("data type: " << Comparator<TYPE>::type_str());
  PRINT("matrix size: " << size << "x" << size);
  PRINT("tile size: " << tile_size << "x" << tile_size);
  PRINT("local memory: " << local_mem << " bytes");

  kernel_arg.grid_dim[0] = size / tile_size;
  kernel_arg.grid_dim[1] = size / tile_size;
  kernel_arg.block_dim[0] = tile_size;
  kernel_arg.block_dim[1] = tile_size;
  kernel_arg.size = size;
  kernel_arg.tile_size = tile_size;

  /* 
  uint32_t max_localmem;
  RT_CHECK(vx_check_occupancy(device, group_size, &max_localmem));
  PRINT("occupancy: max_localmem=" << max_localmem << " bytes");
  RT_CHECK(max_localmem < local_mem);
  */ 

  PRINT("allocate device memory");
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ, &A_buffer));
  RT_CHECK(vx_mem_address(A_buffer, &kernel_arg.A_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_READ, &B_buffer));
  RT_CHECK(vx_mem_address(B_buffer, &kernel_arg.B_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_WRITE, &C_buffer));
  RT_CHECK(vx_mem_address(C_buffer, &kernel_arg.C_addr));

  PRINT("A_addr=0x" << std::hex << kernel_arg.A_addr);
  PRINT("B_addr=0x" << std::hex << kernel_arg.B_addr);
  PRINT("C_addr=0x" << std::hex << kernel_arg.C_addr);

  PRINT("allocate host buffers");
  std::vector<TYPE> h_A(size_sq);
  std::vector<TYPE> h_B(size_sq);
  std::vector<TYPE> h_C(size_sq);

  for (uint32_t i = 0; i < size_sq; ++i) {
    h_A[i] = Comparator<TYPE>::generate();
    h_B[i] = Comparator<TYPE>::generate();
  }

  auto t1 = std::chrono::high_resolution_clock::now();

  PRINT("upload source buffer0");
  RT_CHECK(vx_copy_to_dev(A_buffer, h_A.data(), 0, buf_size));

  PRINT("upload source buffer1");
  RT_CHECK(vx_copy_to_dev(B_buffer, h_B.data(), 0, buf_size));

  auto t2 = std::chrono::high_resolution_clock::now();

  PRINT("Upload kernel binary");
  RT_CHECK(vx_upload_kernel_file(device, kernel_file, &krnl_buffer));

  PRINT("upload kernel argument");
  RT_CHECK(vx_upload_bytes(device, &kernel_arg, sizeof(kernel_arg_t), &args_buffer));

  PRINT("start device");

#ifndef TEST
  auto time_start = std::chrono::high_resolution_clock::now();
#endif

  RT_CHECK(vx_start(device, krnl_buffer, args_buffer));

  PRINT("wait for completion");
  RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

#ifndef TEST
  auto time_end = std::chrono::high_resolution_clock::now();

  double elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          time_end - time_start).count();

  PRINT("Elapsed time: " << elapsed << " ms");
#endif

  PRINT("download destination buffer");

  auto t3 = std::chrono::high_resolution_clock::now();

  RT_CHECK(vx_copy_from_dev(h_C.data(), C_buffer, 0, buf_size));

  auto t4 = std::chrono::high_resolution_clock::now();

  PRINT("verify result");

  int errors = 0;
  {
    std::vector<TYPE> h_ref(size_sq);
    matmul_cpu(h_ref.data(), h_A.data(), h_B.data(), size, size);

    for (uint32_t i = 0; i < h_ref.size(); ++i) {
      if (!Comparator<TYPE>::compare(h_C[i], h_ref[i], i, errors)) {
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

  auto t_upload = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
  auto t_download = std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count();

  uint64_t mcycles = 0;
  uint64_t minstret = 0;

  uint64_t gmem_tot_pending_req = 0;  
  uint64_t gmem_num_reads = 0;  
  uint64_t gmem_num_writes = 0; 
  uint64_t dcache_miss_r = 0;
  uint64_t dcache_num_reads = 0;   

  MPM_QUERY(device, VX_CSR_MCYCLE, mcycles);
  MPM_QUERY(device, VX_CSR_MINSTRET, minstret);

  MPM_QUERY(device, VX_CSR_MPM_DCACHE_READS, dcache_num_reads);
  MPM_QUERY(device, VX_CSR_MPM_DCACHE_MISS_R, dcache_miss_r);
  MPM_QUERY(device, VX_CSR_MPM_MEM_READS, gmem_num_reads);
  MPM_QUERY(device, VX_CSR_MPM_MEM_WRITES, gmem_num_writes);
  MPM_QUERY(device, VX_CSR_MPM_MEM_LT, gmem_tot_pending_req);

  std::cout << std::dec
            << "sgemm2" << ","
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
