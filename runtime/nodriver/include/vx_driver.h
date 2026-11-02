#ifndef VX_DRIVER_H 
#define VX_DRIVER_H 

#include <stdint.h> 
#include <stddef.h> 


#define VXDRV_OK 0 
#define VXDRV_INIT_ERR 1
#define VXDRV_INVALID_ARG 2
#define VXDRV_OFFS_UNALIGN 3
#define VXDRV_CNT_UNALIGN 4
#define VXDRV_OOM 5



typedef struct { 
    void *map_base;
    size_t map_size;
    void *addr; 
} dev_map; 

typedef struct { 
  dev_map global_mem; 
  dev_map dcr;  
} vx_device; 


int vortex_open(vx_device *vx_dev); 
int vortex_close(vx_device *vx_dev);  
int vortex_read(vx_device *vx_dev, char *buffer, size_t count, size_t offset); 
int vortex_write(vx_device *vx_dev, const char *buffer, size_t count, size_t offset); 
int vortex_writeDCR(vx_device *vx_dev, uint32_t dcr_data, uint32_t dcr_valaddr);  
int vortex_readDCR(vx_device *vx_dev, uint32_t *dcr_data, uint32_t *dcr_valaddr); 

#endif // VX_DRIVER_H 
