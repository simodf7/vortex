#include "hal.h" 
#include "print.h"
#include <stdint.h>
#include <stdarg.h> 

void uart_send_char(uint8_t ch){ (void)ch; } 

void hal_printf(const char *restrict format, ...){ 
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
}

void hal_init(){} 
