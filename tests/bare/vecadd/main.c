#include "hal.h" 
#include <stdint.h>
#include <stddef.h>

#include <vortex.h>
#include <VX_types.h>
#include "common.h"

#include "comparator.h" 


#define BUF_SIZE  ((uint32_t)(NUM_POINTS * sizeof(TYPE)))


_Static_assert((NUM_POINTS * sizeof(TYPE)) % 8 == 0, "buffer size must be a multiple of 8 bytes");
_Static_assert(sizeof(kernel_arg_t) % 8 == 0,         "kernel_arg_t size must be a multiple of 8 bytes");



extern const uint8_t _vxbin_start[];
extern const uint8_t _vxbin_end[];


#ifdef TEST
#define PRINT(...) ((void)0)  // NOP
#else
#define PRINT(...) do { hal_printf(__VA_ARGS__); hal_printf("\n"); } while (0)
#endif

#define TYPE_SPEC _Generic((TYPE)0, int: "%d", float: "%f") 


#define RT_CHECK(_expr)                                         \
   do {                                                         \
     int _ret = _expr;                                          \
     if (0 == _ret)                                             \
       break;                                                   \
     hal_printf("Error: '%s' returned %d!\n", #_expr, (int)_ret);   \
     goto cleanup;                                              \
   } while (0)


///////////////////////////////////////////////////////////////////////////////

static TYPE h_src0[NUM_POINTS];
static TYPE h_src1[NUM_POINTS];
static TYPE h_dst[NUM_POINTS];

int main(void)
{
    hal_init(); 
    glibc_srand(50); 

    int ret = -1;

    /* stato del test: tutto locale a main, niente variabili globali */
    vx_device_h device = NULL;

    /* convenzione del runtime: ogni handle punta a un uint64_t (l'offset) */
    uint64_t src0_offs = 0, src1_offs = 0, dst_offs = 0, krnl_offs = 0, args_offs = 0;
    vx_buffer_h src0_buffer = &src0_offs;
    vx_buffer_h src1_buffer = &src1_offs;
    vx_buffer_h dst_buffer  = &dst_offs;
    vx_buffer_h krnl_buffer = &krnl_offs;
    vx_buffer_h args_buffer = &args_offs;


    kernel_arg_t kernel_arg = {0};

    /* open device */
    PRINT("open device connection");
    RT_CHECK(vx_dev_open(&device));

    kernel_arg.num_points = NUM_POINTS;

    /* allocate device memory */
    PRINT("allocate device memory");
    RT_CHECK(vx_mem_alloc(device, BUF_SIZE, VX_MEM_READ, &src0_buffer));
    RT_CHECK(vx_mem_address(src0_buffer, &kernel_arg.src0_addr));

    RT_CHECK(vx_mem_alloc(device, BUF_SIZE, VX_MEM_READ, &src1_buffer));
    RT_CHECK(vx_mem_address(src1_buffer, &kernel_arg.src1_addr));

    RT_CHECK(vx_mem_alloc(device, BUF_SIZE, VX_MEM_WRITE, &dst_buffer));
    RT_CHECK(vx_mem_address(dst_buffer, &kernel_arg.dst_addr));

    /* generate input data */
    PRINT("generate input data");
    for (uint32_t i = 0; i < NUM_POINTS; ++i) {
        h_src0[i] = gen_value();
        h_src1[i] = gen_value();
        h_dst[i]  = 0;
    }

    /* PORT: misura dei tempi (std::chrono) rimossa */ 


    /* upload source buffers */
    PRINT("upload source buffer0");
    RT_CHECK(vx_copy_to_dev(src0_buffer, h_src0, 0, BUF_SIZE));

    PRINT("upload source buffer1");
    RT_CHECK(vx_copy_to_dev(src1_buffer, h_src1, 0, BUF_SIZE));

    /* upload kernel binary */
    PRINT("upload kernel binary");
    RT_CHECK(vx_upload_kernel_bytes(device, _vxbin_start,
                                 (uint64_t)(_vxbin_end - _vxbin_start), &krnl_buffer));

    /* upload kernel arguments */
    PRINT("upload kernel argument");
    RT_CHECK(vx_upload_bytes(device, &kernel_arg, sizeof(kernel_arg_t), &args_buffer));

    /* start and wait */
    PRINT("start device");
    RT_CHECK(vx_start(device, krnl_buffer, args_buffer));

    PRINT("wait for completion");
    RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

    /* download destination buffer */
    PRINT("download destination buffer");
    RT_CHECK(vx_copy_from_dev(h_dst, dst_buffer, 0, BUF_SIZE));

    /* verify result */
    PRINT("verify result");
    {
        uint32_t errors = 0;
        verify_result(h_src0, h_src1, h_dst, &errors);  
        if (errors != 0) {
            PRINT("FAILED, Number of errors: %u", errors);
            goto cleanup;
        }
    }
    PRINT("PASSED");

#ifdef TEST
    /* performance counters */
    {
        uint64_t mcycles = 0, minstret = 0;
        uint64_t gmem_tot_pending_req = 0, gmem_num_reads = 0, gmem_num_writes = 0;
        uint64_t dcache_miss_r = 0, dcache_num_reads = 0;

        RT_CHECK(vx_mpm_query(device, VX_CSR_MCYCLE,           0, &mcycles));
        RT_CHECK(vx_mpm_query(device, VX_CSR_MINSTRET,         0, &minstret));
        RT_CHECK(vx_mpm_query(device, VX_CSR_MPM_DCACHE_READS, 0, &dcache_num_reads));
        RT_CHECK(vx_mpm_query(device, VX_CSR_MPM_DCACHE_MISS_R,0, &dcache_miss_r));
        RT_CHECK(vx_mpm_query(device, VX_CSR_MPM_MEM_READS,    0, &gmem_num_reads));
        RT_CHECK(vx_mpm_query(device, VX_CSR_MPM_MEM_WRITES,   0, &gmem_num_writes));
        RT_CHECK(vx_mpm_query(device, VX_CSR_MPM_MEM_LT,       0, &gmem_tot_pending_req));

        /* PORT: stampa della riga CSV
           "vecadd,<NUM_POINTS>,<mcycles>,<minstret>,<gmem_reads>,<gmem_writes>,
            <gmem_lt>,<dcache_miss_r>,<dcache_reads>,<t_upload>,<t_download>"
           Serve una funzione di stampa di piattaforma per gli interi a 64 bit. */
        (void)mcycles; (void)minstret; (void)gmem_tot_pending_req;
        (void)gmem_num_reads; (void)gmem_num_writes; (void)dcache_miss_r; (void)dcache_num_reads;
    }
#endif

    ret = 0;

cleanup:
    vx_mem_free(src0_buffer);
    if (device)  vx_dev_close(device);

    return ret;
}
