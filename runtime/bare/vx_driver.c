#include "vx_driver.h" 
#include "hal.h" 

#define OFFSET_DCR_DATA 0 
#define OFFSET_DCR_VALADDR 1 

#ifndef VX_DEBUG
#define PRINT(...) ((void)0)  // NOP
#else
#define PRINT(...) do { hal_printf(__VA_ARGS__); hal_printf("\n"); } while (0)
#endif


int vortex_open(vx_device *vx_dev) {

    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 

    vx_dev->global_mem.addr = (void *) MEM_BASE; 
    vx_dev->global_mem.size = MEM_SIZE;
    vx_dev->dcr.addr = (void *) (DCR_BASE); 
    vx_dev->dcr.size = DCR_SIZE; 

    return VXDRV_OK; 
} 


int vortex_close(vx_device *vx_dev) {
   
   if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 
    
   vx_dev->global_mem.addr = NULL; 
   vx_dev->global_mem.size = 0; 
   vx_dev->dcr.addr = NULL; 
   vx_dev->dcr.size = 0; 
     
   return VXDRV_OK; 
} 



// Read from MEM 
int vortex_read(vx_device *vx_dev, char *buffer, size_t count, size_t offset){ 
        
    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 

    // check if offset is aligned to 8 Byte (64 bit) 
    if(offset % sizeof(uint64_t) != 0) return -VXDRV_OFFS_UNALIGN; 
    
    // check if count is multiple of 8 Byte (64 bit) 
    if(count % sizeof(uint64_t) != 0) return -VXDRV_CNT_UNALIGN; 

    // check if requested bytes are available
    if(offset > MEM_SIZE || count > MEM_SIZE - offset) return -VXDRV_OOM; 
    

    uint64_t *d = (uint64_t *) buffer; 
    
    volatile uint8_t* addr = (volatile uint8_t*) vx_dev->global_mem.addr + offset; 
    volatile const uint64_t *s = (volatile const uint64_t *) addr; 

    for(size_t i = 0; i < count / 8 ; i++){ 
        d[i] = s[i]; 
    }; 

    PRINT("[VORTEX][READ]: Read %zu bytes from Global MEM", count); 
    return VXDRV_OK;     
}; 



// Write to MEM 
int vortex_write(vx_device *vx_dev, const char *buffer, size_t count, size_t offset){ 
  
    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 
 
    // check if offset is aligned to 8 Byte (64 bit) 
    if(offset % sizeof(uint64_t) != 0) return -VXDRV_OFFS_UNALIGN; 
    
    // check if count is multiple of 8 Byte (64 bit) 
    if(count % sizeof(uint64_t) != 0) return -VXDRV_CNT_UNALIGN; 

    // check if requested bytes are available
    if(offset > MEM_SIZE || count > MEM_SIZE - offset) return -VXDRV_OOM; 


    const uint64_t *s = (const uint64_t *) buffer; 
    
    volatile uint8_t* addr = (volatile uint8_t*) vx_dev->global_mem.addr + offset; 
    volatile uint64_t *d = (volatile uint64_t *) addr;  
   
    for(size_t i = 0; i < count/8; i++){ 
        d[i] = s[i]; 
    }; 

    PRINT("[VORTEX][WRITE]: Copied %zu bytes to Global MEM", count); 
    return VXDRV_OK;      
}; 



int vortex_writeDCR(vx_device *vx_dev, uint32_t dcr_data, uint32_t dcr_valaddr){ 
    
    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 
    
    volatile uint8_t *base = (volatile uint8_t *) vx_dev->dcr.addr;
    *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_DATA) = dcr_data;  
    *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_VALADDR) = dcr_valaddr;  
 
    PRINT("[VORTEX][DCR_WRITE]"); 

    return VXDRV_OK; 
}; 

int vortex_readDCR(vx_device *vx_dev, uint32_t *dcr_data, uint32_t *dcr_valaddr){
    
    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 

    volatile uint8_t *base = (volatile uint8_t *) vx_dev->dcr.addr;
    *(dcr_data) = *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_DATA); 
    *(dcr_valaddr) = *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_VALADDR); 
 
    PRINT("[VORTEX][DCR_READ]"); 
 
    return VXDRV_OK; 
}; 
