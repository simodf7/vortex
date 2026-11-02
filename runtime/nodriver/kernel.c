#include <vx_driver.h>
#include <VX_types.h> 
#include <stdint.h> 
#include <stddef.h> 
#include <stdio.h>

extern const uint8_t _vxbin_start[]; 
extern const uint8_t _vxbin_end[]; 

#define END_BIT 15
#define START_BIT 14
#define RESET_BIT 13
#define VALID_BIT 12


void delay() { 
    for(volatile int i=0; i < 100000; i++){ 
    } 
} 


void copy_kernel_bin(vx_device *dev){ 
  
    const uint8_t *bin_ptr = (const uint8_t *) _vxbin_start;

    // skips header  
    printf("[Kernel copy]: min_vma: 0x%lX\n", *((volatile uint64_t*)(bin_ptr)));
    bin_ptr += sizeof(uint64_t); 
    printf("[Kernel copy]: min_vma: 0x%lX\n", *((volatile uint64_t*)(bin_ptr)));
    bin_ptr += sizeof(uint64_t); 
    
    
    size_t bin_size = _vxbin_end - bin_ptr;  
       
    int ret = vortex_write(dev, bin_ptr, bin_size, 0); 
    if (VXDRV_OK != ret) {printf("Error Write: %d\n", ret); return;} 

    printf("Kernel successfully copied\n"); 

}; 

void control_DCR(vx_device *dev, uint32_t data, uint16_t dcr_addr){ 

    uint32_t dcr_data = data; 
    uint32_t dcr_valaddr = (1 << RESET_BIT) | (1 << VALID_BIT) | dcr_addr; 
     
    int ret = vortex_writeDCR(dev, dcr_data, dcr_valaddr); 
    if (VXDRV_OK != ret) { printf("Error Write DCR: %d\n", ret); return ; } 

    dcr_valaddr = (1 << RESET_BIT) | dcr_addr; 
    
    ret = vortex_writeDCR(dev, dcr_data, dcr_valaddr); 
    if (VXDRV_OK != ret) { printf("Error Write DCR: %d\n", ret); return ; } 

} 


void start_vortex(vx_device *dev){ 
    
    control_DCR(dev, STARTUP_ADDR & 0xFFFFFFFF, VX_DCR_BASE_STARTUP_ADDR0); 
    control_DCR(dev, STARTUP_ADDR >> 32, VX_DCR_BASE_STARTUP_ADDR1); 
    
    int ret = vortex_writeDCR(dev, 0, 0); 
    if (VXDRV_OK != ret) { printf("Error Write DCR: %d\n", ret); return ; } 
  
} 


int main(void) { 

  vx_device dev; 

  // Opening vortex
  int ret = vortex_open(&dev); 
  if(VXDRV_OK != ret) { 
    printf("Error opening Vortex: %d\n", ret); 
    return 1; 
  } 

  // Copying binary 
  copy_kernel_bin(&dev);   

  // We copy something at the address 
  uint64_t s = 0xFFFFFFFF; 
  ret = vortex_write(&dev, &s, sizeof(s), 0x62000);
  if (VXDRV_OK != ret) {printf("Error Write: %d\n", ret); return 1;}


  // Start vortex
  start_vortex(&dev); 

  uint32_t data, addr = 0; 
  
  while(1) {
    ret = vortex_readDCR(&dev, &data, &addr); 
    if (VXDRV_OK != ret) { printf("Error Read DCR: %d\n", ret); return 1; }   
    if((addr >> START_BIT) & 1) break; 
    delay(); 
  } 
  
  printf("Kernel started\n"); 


  while(1) {
    ret = vortex_readDCR(&dev, &data, &addr); 
    if (VXDRV_OK != ret) { printf("Error Read DCR: %d\n", ret); return 1; }   
    if((addr >> END_BIT) & 1) break; 
    delay(); 
  } 

  printf("Kernel ended\n"); 

  // check s after copy  
  ret = vortex_read(&dev, &s, sizeof(s), 0x62000);
  if (VXDRV_OK != ret) {printf("Error Write: %d\n", ret); return 1;}

  vortex_close(&dev);  
  
  return 0; 
} 
