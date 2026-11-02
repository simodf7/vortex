#include <vx_driver.h> 
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define GMEM_PHYS_ADDR 0x800000000
#define GMEM_SIZE 0x20000000

#define DCR_PHYS_ADDR 0xA0001000
#define DCR_SIZE 0x1000

#define OFFSET_DCR_DATA 0 
#define OFFSET_DCR_VALADDR 1 


int vortex_open(vx_device *vx_dev) {

    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 

    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        perror("Failed to open /dev/mem");
        return -VXDRV_INIT_ERR;
    }

    long page_size = sysconf(_SC_PAGE_SIZE);

    if (page_size <= 0) {
        perror("sysconf");
        close(mem_fd); 
        return -VXDRV_INIT_ERR;
    }

    // Obtain Vortex global memory virtual address 
    off_t gmem_page_addr = GMEM_PHYS_ADDR & ~(page_size - 1); 
    off_t gmem_page_offs = GMEM_PHYS_ADDR - gmem_page_addr; 
    void *gmem_base = mmap(NULL, GMEM_SIZE + gmem_page_offs,  PROT_READ | PROT_WRITE, 
                             MAP_SHARED, mem_fd, gmem_page_addr);
   
    if (gmem_base == MAP_FAILED) {
        perror("Vortex global memory mmap failed");
        close(mem_fd);
        return -VXDRV_INIT_ERR;
    }
    
    vx_dev->global_mem.map_base = gmem_base; 
    vx_dev->global_mem.map_size = GMEM_SIZE + gmem_page_offs; 
    vx_dev->global_mem.addr = (void*) ((volatile uint8_t*) gmem_base + gmem_page_offs);
 

    // Obtain DCR IP virtual address
    off_t dcr_page_addr = DCR_PHYS_ADDR & ~(page_size - 1); 
    off_t dcr_page_offs = DCR_PHYS_ADDR - dcr_page_addr; 
    void *dcr_base = mmap(NULL, DCR_SIZE + dcr_page_offs, PROT_READ | PROT_WRITE, 
                             MAP_SHARED, mem_fd, dcr_page_addr);

    if (dcr_base == MAP_FAILED) {
        perror("Vortex DCR mmap failed");
        munmap(gmem_base, GMEM_SIZE + gmem_page_offs); 
        close(mem_fd);
        return -VXDRV_INIT_ERR;
    }

    close(mem_fd);

    vx_dev->dcr.map_base = dcr_base;  
    vx_dev->dcr.map_size = DCR_SIZE + dcr_page_offs; 
    vx_dev->dcr.addr = (void*) ((volatile uint8_t *) dcr_base + dcr_page_offs); 
    
    return VXDRV_OK; 
} 


int vortex_close(vx_device *vx_dev) {
   
   if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 
    
   munmap(vx_dev->global_mem.map_base, vx_dev->global_mem.map_size);
   munmap(vx_dev->dcr.map_base, vx_dev->dcr.map_size);
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
    if(offset > GMEM_SIZE || count > GMEM_SIZE - offset) return -VXDRV_OOM; 
    

    uint64_t *d = (uint64_t *) buffer; 
    
    volatile uint8_t* addr = (volatile uint8_t*) vx_dev->global_mem.addr + offset; 
    volatile const uint64_t *s = (volatile const uint64_t *) addr; 

    for(size_t i = 0; i < count / 8 ; i++){ 
        d[i] = s[i]; 
    }; 

    printf("[VORTEX][READ]: Read %zu bytes from Global MEM\n", count); 
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
    if(offset > GMEM_SIZE || count > GMEM_SIZE - offset) return -VXDRV_OOM; 


    const uint64_t *s = (const uint64_t *) buffer; 
    
    volatile uint8_t* addr = (volatile uint8_t*) vx_dev->global_mem.addr + offset; 
    volatile uint64_t *d = (volatile uint64_t *) addr;  
   
    for(size_t i = 0; i < count/8; i++){ 
        d[i] = s[i]; 
    }; 

    printf("[VORTEX][WRITE]: Copied %zu bytes to Global MEM\n", count); 
    return VXDRV_OK;      
}; 



int vortex_writeDCR(vx_device *vx_dev, uint32_t dcr_data, uint32_t dcr_valaddr){ 
    
    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 
    
    volatile uint8_t *base = (volatile uint8_t *) vx_dev->dcr.addr;
    *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_DATA) = dcr_data;  
    *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_VALADDR) = dcr_valaddr;  
 
    printf("[VORTEX][DCR_WRITE]\n"); 

    return VXDRV_OK; 
}; 

int vortex_readDCR(vx_device *vx_dev, uint32_t *dcr_data, uint32_t *dcr_valaddr){
    
    if(vx_dev == NULL) return -VXDRV_INVALID_ARG; 

    volatile uint8_t *base = (volatile uint8_t *) vx_dev->dcr.addr;
    *(dcr_data) = *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_DATA); 
    *(dcr_valaddr) = *(volatile uint32_t *)(base + sizeof(uint32_t) * OFFSET_DCR_VALADDR); 
 
    printf("[VORTEX][DCR_READ]\n"); 
 
    return VXDRV_OK; 
}; 
