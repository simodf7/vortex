#include <vx_intrinsics.h>
#include <vx_print.h>
#include <vx_spawn.h>

#define LED_ADDR  0xA0110000
#define OFF_DATA 0
#define OFF_TRI 4


#define NUM_TICKS 100000000

void delay(){
  for(volatile int i=0; i<NUM_TICKS; i++){
    //delay
  }
}

void kernel(){

    *(volatile char*)(0xA0120000 + 5) = 'X';
    *(volatile char*)(0xA0120000 + 5) = '\n';
    vx_printf(">> Vortex: Led Only Starting\n"); 

    volatile uint32_t *addr = (volatile uint32_t*) (LED_ADDR + OFF_TRI); 
    *addr = 0xFF;
    
    addr = (volatile uint32_t*) (LED_ADDR + OFF_DATA);  
    while(1) {
	*addr ^= 0xFF; 
	delay(); 
    }



}; 

int main() {
    kernel(); 
    return 0;
}

