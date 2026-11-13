#include "hal.h" 
#include "print.h"
#include <stdint.h>
#include <stdarg.h> 
#include "xil_mmu.h"




void outbyte(char c); 

void uart_send_char(uint8_t ch){ 
    if(ch == '\n') outbyte('\r'); 
    outbyte(ch);
} 

void hal_printf(const char *restrict format, ...){ 
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
}


void hal_init(){
  /* setting 1Gb Region 0x8_0000_0000 as Normal non Cacheable */   
  Xil_SetTlbAttributes(MEM_BASE, NORM_NONCACHE); 
} 
