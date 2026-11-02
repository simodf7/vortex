#ifndef COMPARATOR_H
#define COMPARATOR_H

#include <stdint.h>
#include "hal.h"
#include "common.h"

#define ERR_PERCENTAGE_ALLOWED 10 

/* Abs value */ 
static inline float abs_v(float x){
   return (x >= 0.0f) ? x : -x; 
}; 


/* pseudo-random generator*/

#define GLIBC_RAND_MAX 2147483647
static int32_t  r[34];
static uint32_t idx;

static uint32_t glibc_rand()                     /* 0 .. 2^31 - 1 */
{
    int32_t v = (int32_t)((uint32_t)r[(idx - 31) % 34] + (uint32_t)r[(idx - 3) % 34]);
    r[idx % 34] = v;
    idx++;
    return (uint32_t)v >> 1;
}

static void glibc_srand(uint32_t seed)
{
    if (seed == 0) seed = 1;

    r[0] = (int32_t)seed;
    for (int i = 1; i < 31; i++) {
        int64_t v = (16807LL * r[i - 1]) % 2147483647;
        if (v < 0) v += 2147483647;
        r[i] = (int32_t)v;
    }
    for (int i = 31; i < 34; i++) r[i] = r[i - 31];
    idx = 34;
    for (int i = 0; i < 310; i++) (void)glibc_rand(); 
}



/* int */
static inline int gen_int(void) { return (int)glibc_rand(); }

static inline int compare_int(int actual, int expected)
{
    return actual == expected;
}

static inline void print_int(int actual, int expected)
{
    hal_printf("Expected: %d, Actual: %d\n", expected, actual);  
}



/* float */
static inline float gen_float(void)
{
    return (float)(int32_t)glibc_rand() / (float)GLIBC_RAND_MAX;
}

static inline int compare_float(float actual, float expected)
{
    // Comparation between actual and expected is now calculated in an arithmetic fashion 
    if (abs_v((actual - expected) / expected) * 100 < ERR_PERCENTAGE_ALLOWED) return 1; 
    return 0; 
}

static inline void print_float(float actual, float expected)
{
    hal_printf("Expected: %f, Actual: %f\n", expected, actual);  
}



#define gen_value() \
    _Generic((TYPE)0, int: gen_int, float: gen_float)()

#define compare(actual, expected) \
    _Generic((TYPE)0, int: compare_int, float: compare_float)((actual), (expected))

#define print_err(actual, expected) \
    _Generic((TYPE)0, int: print_int, float: print_float)((actual), (expected))

static inline void verify_result(TYPE *h_src0, TYPE *h_src1, TYPE *h_dst, uint32_t *e){ 
  for (uint32_t i = 0; i < NUM_POINTS; ++i) {
      TYPE ref = h_src0[i] + h_src1[i];
      if (!compare(h_dst[i], ref)) {
          ++(*e);
          print_err(h_dst[i], ref);
          hal_printf("\n");   
      }
  }
}; 




#endif /* COMPARATOR_H */
