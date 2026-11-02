#include <vx_driver.h>
#include <stdint.h> 
#include <stddef.h> 
#include <stdio.h>

extern const uint8_t _binary_kernel_vxbin_start[]; 
extern const uint8_t _binary_kernel_vxbin_end[]; 

void copy_kernel_bin(vx_device *dev){ 
  
    const uint8_t *bin_ptr = (const uint8_t *) _binary_kernel_vxbin_start; 
    bin_ptr += sizeof(uint64_t) * 2; // skips header  
    
    size_t bin_size = _binary_kernel_vxbin_end - bin_ptr;  
       
    int ret = vortex_write(dev, bin_ptr, bin_size, 0); 
    if (VXDRV_OK != ret) {printf("Error Write: %d\n", ret); return;} 
   
    __attribute__((aligned(8))) uint8_t dst[240] = {0};  
 
    ret = vortex_read(dev, dst, sizeof(dst), 0); 
    if (VXDRV_OK != ret) { printf("Error Read: %d\n", ret); return; }
    
    for(int i = 0; i < 240; i++){ 
        if(bin_ptr[i] != dst[i]) { printf("%d-byte is different\n", i); return; }  
    }

    printf("Kernel successfully copied\n"); 

}; 

int main(void) { 

  vx_device dev; 

  // Opening vortex
  int ret = vortex_open(&dev); 
  if(VXDRV_OK != ret) { 
    printf("Error opening Vortex: %d\n", ret); 
    return 1; 
  } 
    
  // Testing Memory 
  __attribute__((aligned(8))) const char src[16] = "Hello World";  
  __attribute__((aligned(8))) char dst[16] = {0};  
 
  ret = vortex_write(&dev, src, sizeof(src), 0); 
  if (VXDRV_OK != ret) { printf("Error Write: %d\n", ret); return 1; } 

  ret = vortex_read(&dev, dst, sizeof(dst), 0); 
  if (VXDRV_OK != ret) { printf("Error Read: %d\n", ret); return 1; } 
  
  printf("Read: %s\n", dst);   

  // Testing DCR 

  uint32_t wd = 0x12345678, wa = 0x00000001;   // harmless DCR address (e.g. STARTUP_ADDR0)
  uint32_t rd, ra;

  ret = vortex_writeDCR(&dev, wd, wa); 
  if (VXDRV_OK != ret) { printf("Error Write DCR: %d\n", ret); return 1; } 

  ret = vortex_readDCR(&dev, &rd, &ra); 
  if (VXDRV_OK != ret) { printf("Error Read DCR: %d\n", ret); return 1; }

  printf("DCR DATA: 0x%08X\n", rd); 
  printf("DCR ADDR: 0x%08X\n", ra); 

  // testing firmware
  copy_kernel_bin(&dev);   

  vortex_close(&dev);  
  return 0; 
} 
