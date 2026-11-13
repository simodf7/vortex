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

static void convolution_cpu(TYPE *O, TYPE *I, TYPE *W, int32_t width, int32_t height) {
  int paddedWidth = width + 2;
  for (int32_t y = 0; y < height; ++y) {
    for (int32_t x = 0; x < width; ++x) {
      int paddedY = y + 1;
      int paddedX = x + 1;
      TYPE sum(0);
      for (int32_t ky = -1; ky <= 1; ++ky) {
        for (int32_t kx = -1; kx <= 1; ++kx) {
          int32_t iy = paddedY + ky;
          int32_t ix = paddedX + kx;
          TYPE value = I[iy * paddedWidth + ix];
          TYPE weight = W[(ky + 1) * 3 + (kx + 1)];
          sum += value * weight;
        }
      }
      O[y * width + x] = sum;
    }
  }
}

const char* kernel_file = "kernel.vxbin";
int size = 32;
bool use_lmem = false;

vx_device_h device = nullptr;
vx_buffer_h I_buffer = nullptr;
vx_buffer_h W_buffer = nullptr;
vx_buffer_h O_buffer = nullptr;
vx_buffer_h krnl_buffer = nullptr;
vx_buffer_h args_buffer = nullptr;
kernel_arg_t kernel_arg = {};

static void show_usage() {
   std::cout << "Vortex Test." << std::endl;
   std::cout << "Usage: [-k kernel] [-l: local memory] [-n size] [-h|?: help]" << std::endl;
}

static void parse_args(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "n:k:lh")) != -1) {
    switch (c) {
    case 'n':
      size = atoi(optarg);
      break;
    case 'l':
      use_lmem = true;
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
    vx_mem_free(I_buffer);
    vx_mem_free(W_buffer);
    vx_mem_free(O_buffer);
    vx_mem_free(krnl_buffer);
    vx_mem_free(args_buffer);
    vx_dev_close(device);
  }
}

int main(int argc, char *argv[]) {
  // parse command arguments
  parse_args(argc, argv);

  std::srand(50);

  PRINT("open device connection");
  RT_CHECK(vx_dev_open(&device));

  PRINT("data type: " << Comparator<TYPE>::type_str());
  PRINT("matrix size: " << size << "x" << size);

  kernel_arg.grid_dim[0] = size;
  kernel_arg.grid_dim[1] = size;
  kernel_arg.width = size;
  kernel_arg.use_lmem = use_lmem;

  uint32_t o_points = size * size;
  uint32_t i_points = (size+2) * (size+2);
  uint32_t w_points = 3 * 3;

  PRINT("allocate device memory");
  size_t i_nbytes = i_points * sizeof(TYPE);
  size_t w_nbytes = w_points * sizeof(TYPE);
  size_t o_nbytes = o_points * sizeof(TYPE);
  RT_CHECK(vx_mem_alloc(device, i_nbytes, VX_MEM_READ, &I_buffer));
  RT_CHECK(vx_mem_address(I_buffer, &kernel_arg.I_addr));
  RT_CHECK(vx_mem_alloc(device, w_nbytes, VX_MEM_READ, &W_buffer));
  RT_CHECK(vx_mem_address(W_buffer, &kernel_arg.W_addr));
  RT_CHECK(vx_mem_alloc(device, o_nbytes, VX_MEM_WRITE, &O_buffer));
  RT_CHECK(vx_mem_address(O_buffer, &kernel_arg.O_addr));

  if (use_lmem) {
    uint64_t dev_local_mem_size;
    RT_CHECK(vx_dev_caps(device, VX_CAPS_LOCAL_MEM_SIZE, &dev_local_mem_size));
    if (w_nbytes > dev_local_mem_size) {
      PRINT("Error: Not enough local memory: needed=" << w_nbytes << ", available=" << dev_local_mem_size);
      cleanup();
      exit(1);
    }
  }

  PRINT("dev_argI=0x" << std::hex << kernel_arg.I_addr);
  PRINT("dev_argW=0x" << std::hex << kernel_arg.W_addr);
  PRINT("dev_argO=0x" << std::hex << kernel_arg.O_addr);
  fprintf(stderr, "O_addr=0x%lx\n", (unsigned long)kernel_arg.O_addr);


  // Generate input values
  std::vector<TYPE> h_I(i_points);
  std::vector<TYPE> h_W(w_points);
  std::vector<TYPE> h_O(o_points);
  for (int32_t y = -1; y < size+1; ++y) {
    for (int32_t x = -1; x < size+1; ++x) {
      if (x >= 0 && x < size && y >= 0 && y < size) {
        h_I[(y+1) * (size+2) + (x+1)] = static_cast<TYPE>(rand()) / RAND_MAX;
      } else {
        h_I[(y+1) * (size+2) + (x+1)] = 0;
      }
    }
  }
  for (uint32_t i = 0; i < w_points; ++i) {
    h_W[i] = static_cast<TYPE>(rand()) / RAND_MAX;
  }


  // DEBUG 
  std::vector<TYPE> h_fill(o_points, -1.0);  // conv3 non produce mai valori negativi
  RT_CHECK(vx_copy_to_dev(O_buffer, h_fill.data(), 0, o_nbytes));



  auto t1 = std::chrono::high_resolution_clock::now();

  // upload input buffer
  {
    PRINT("upload source buffer");
    RT_CHECK(vx_copy_to_dev(I_buffer, h_I.data(), 0, i_nbytes));
  }

  // upload weight buffer
  {
    PRINT("upload weight buffer");
    RT_CHECK(vx_copy_to_dev(W_buffer, h_W.data(), 0, w_nbytes));
  }

  auto t2 = std::chrono::high_resolution_clock::now();

  // Upload kernel binary
  PRINT("Upload kernel binary");
  RT_CHECK(vx_upload_kernel_file(device, kernel_file, &krnl_buffer));

  // upload kernel argument
  PRINT("upload kernel argument");
  RT_CHECK(vx_upload_bytes(device, &kernel_arg, sizeof(kernel_arg_t), &args_buffer));

#ifndef TEST
  auto time_start = std::chrono::high_resolution_clock::now();
#endif

  // start device
  PRINT("start device");
  RT_CHECK(vx_start(device, krnl_buffer, args_buffer));

  // wait for completion
  PRINT("wait for completion");
  RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

#ifndef TEST
  auto time_end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
  PRINT("Elapsed time: " << elapsed << " ms");
#endif

  // download destination buffer
  PRINT("download destination buffer");

  auto t3 = std::chrono::high_resolution_clock::now();

  RT_CHECK(vx_copy_from_dev(h_O.data(), O_buffer, 0, o_nbytes));

  auto t4 = std::chrono::high_resolution_clock::now();

  // verify result
  PRINT("verify result");
  int errors = 0;
  std::vector<TYPE> h_ref(o_points);
  convolution_cpu(h_ref.data(), h_I.data(), h_W.data(), size, size);
  
  {
    for (uint32_t i = 0; i < h_ref.size(); ++i) {
      auto ref = h_ref[i];
      auto cur = h_O[i];
      if (!Comparator<TYPE>::compare(cur, ref, i, errors)) {
        ++errors;
      }
    }
  }

  if (errors != 0) {
    fprintf(stderr,"Found %d errors!\n", errors);
    uint64_t mw = 0, dw = 0, cm = 0; 
    vx_mpm_query(device, VX_CSR_MPM_MEM_WRITES, 0, &mw);
    vx_mpm_query(device, VX_CSR_MPM_DCACHE_WRITES, 0, &dw);
    vx_mpm_query(device, VX_CSR_MPM_COALESCER_MISS, 0, &cm);
    fprintf(stderr, "MEM_WRITES=%lu DCACHE_WRITES=%lu COALESCER_MISS=%lu \n", (unsigned long)mw, (unsigned long)dw, (unsigned long)cm);

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

  uint64_t dcache_num_writes = 0;
  uint64_t coalescer_miss = 0;  
  MPM_QUERY(device, VX_CSR_MPM_DCACHE_WRITES, dcache_num_writes);
  MPM_QUERY(device, VX_CSR_MPM_DCACHE_READS, dcache_num_reads);
  MPM_QUERY(device, VX_CSR_MPM_DCACHE_MISS_R, dcache_miss_r);
  MPM_QUERY(device, VX_CSR_MPM_MEM_READS, gmem_num_reads);
  MPM_QUERY(device, VX_CSR_MPM_MEM_WRITES, gmem_num_writes);
  MPM_QUERY(device, VX_CSR_MPM_MEM_LT, gmem_tot_pending_req);
  MPM_QUERY(device, VX_CSR_MPM_COALESCER_MISS, coalescer_miss);
  
  std::cout << std::dec
            << "conv3" << ","
            << size << ","
            << mcycles << ","
            << minstret << ","
            << gmem_num_reads << ","
            << gmem_num_writes << ","
            << gmem_tot_pending_req << ","
            << dcache_miss_r << "," 
            << dcache_num_reads << ","
            << dcache_num_writes << "," 
            << coalescer_miss << ","   
            << t_upload << "," 
            << t_download 
            << std::endl;                 

#endif

  cleanup();

  return 0;
}
