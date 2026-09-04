#ifndef _COMMON_H_
#define _COMMON_H_

#define MAGIC       0xCAFEBABE
#define KERNEL_TAG  0xC0FFEE00
#define SENTINEL    0xDEADBEEF
#define NUM_WORDS   4

typedef struct {
  uint32_t magic;
  uint32_t num_points;
  uint64_t dst_addr;
} kernel_arg_t;

#endif
