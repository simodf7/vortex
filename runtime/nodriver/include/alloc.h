#include <stdint.h> 
#include <stddef.h> 
#include <stdio.h>  // Da capire come inserire solo la printf

// Already defined HEAP_OFFS and HEAP_SIZE
#define HEAP_LAST (HEAP_OFFS + HEAP_SIZE) 
#define BLOCK_SIZE 64u

static uint64_t current_addr = HEAP_OFFS; 

static inline size_t align_up(size_t size){ 
  return (size + BLOCK_SIZE -1) & ~(BLOCK_SIZE-1); 
} 

static int alloc(size_t size, uint64_t *addr){ 
    size_t asize = align_up(size);
    int exit_code = 0;  
    if (asize > HEAP_LAST - current_addr){
      printf("[ALLOCATOR]: Error. Aligned Size %ld requested exceed Heap (free space %ld)\n", asize, HEAP_LAST - current_addr);  
      exit_code = -1; 
    } 
    else {
      *addr = current_addr; 
      current_addr += asize;
    } 
    return exit_code;   
}; 


static void release(void){ 
    current_addr = HEAP_OFFS; 
}; 






