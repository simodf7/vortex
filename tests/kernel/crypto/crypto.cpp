#include <vx_intrinsics.h>
#include <vx_print.h>
#include <vx_spawn.h>

#define LED_ADDR   0xA0110000
#define LED_OUTPUT 0xA0110004

#define BRAM_OUTPUT 0xA0000000
#define XLEN 64

#define NUM_TICKS 500000

#define TEST_RR(name, instr)                                      \
do {                                                              \
    vx_printf("\n==============================\n");               \
    vx_printf("%s\n", name);                                      \
    vx_printf("rs1 = 0x%08x\n", a);                               \
    vx_printf("rs2 = 0x%08x\n", b);                               \
                                                                  \
    __asm__(instr " %0,%1,%2"                                     \
            : "=r"(c)                                             \
            : "r"(a), "r"(b));                                    \
                                                                  \
    *output = c;                                                  \
                                                                  \
    vx_printf("rd = 0x%08x\n", c);                                \
    delay();                                                      \
} while(0)


#define TEST_RRI(name, instr, value)                                      \
do {                                                              \
    vx_printf("\n==============================\n");               \
    vx_printf("%s\n", name);                                      \
    vx_printf("rs1 = 0x%08x\n", a);                               \
    vx_printf("rs2 = 0x%08x\n", b);                               \
    vx_printf("i   = 0x%08x\n", value); 			 \
                                                                  \
    __asm__(instr " %0,%1,%2"                                     \
            : "=r"(c)                                             \
            : "r"(a), "r"(b), "i"(value));                                    \
                                                                  \
    *output = c;                                                  \
                                                                  \
    vx_printf("rd = 0x%08x\n", c);                                \
    delay();                                                      \
} while(0)


#define TEST64_RR(name, instr)                                      \
do {                                                              \
    vx_printf("\n==============================\n");               \
    vx_printf("%s\n", name);                                      \
    vx_printf("rs1 = 0x%08x\n", a);                               \
    vx_printf("rs2 = 0x%08x\n", b);                               \
                                                                  \
    __asm__(instr " %0,%1,%2"                                     \
            : "=r"(c_long)                                        \
            : "r"(a), "r"(b));                                    \
                                                                  \
    *output = c_long;                                                  \
                                                                  \
    vx_printf("rd = 0x%016llx\n", c_long);                             \
    delay();                                                      \
} while(0)

#define TEST_RI(name, instr, value) 				 \
do {                                                              \
    vx_printf("\n==============================\n");               \
    vx_printf("%s\n", name);                                      \
    vx_printf("rs1 = 0x%08x\n", a);                               \
    vx_printf("i   = 0x%08x\n", value);                           \
                                                                  \
    __asm__(instr " %0,%1,%2"                                     \
            : "=r"(c)                                             \
            : "r"(a), "i"(value));                                \
                                                                  \
    *output = c;                                                  \
                                                                  \
    vx_printf("rd = 0x%08x\n", c);                                \
    delay();                                                      \
} while(0)

#define TEST64_RI(name, instr, value) 				 \
do {                                                              \
    vx_printf("\n==============================\n");               \
    vx_printf("%s\n", name);                                      \
    vx_printf("rs1 = 0x%08x\n", a);                               \
    vx_printf("i   = 0x%08x\n", value);                           \
                                                                  \
    __asm__(instr " %0,%1,%2"                                     \
            : "=r"(c_long)                                             \
            : "r"(a), "i"(value));                                \
                                                                  \
    *output = c_long;                                                  \
                                                                  \
    vx_printf("rd = 0x%016llxx\n", c_long);                                \
    delay();                                                      \
} while(0)

#define TEST_R(name, instr)                                       \
do {                                                              \
    vx_printf("\n==============================\n");              \
    vx_printf("%s\n", name);                                      \
    vx_printf("rs1 = 0x%08x\n", a);                               \
                                                                  \
    __asm__(instr " %0,%1"                                        \
            : "=r"(c)                                             \
            : "r"(a));                                            \
                                                                  \
    *output = c;                                                  \
                                                                  \
    vx_printf("rd = 0x%08x\n", c);                                \
    delay();                                                      \
} while(0)



void delay(){
  for(volatile int i=0; i<NUM_TICKS; i++){
    //delay
  }
}

void blinky_kernel(void*) {
    uint32_t thread_id = blockIdx.x;

    volatile uint32_t* led_output = (volatile uint32_t*)LED_OUTPUT;
    volatile uint32_t* led        = (volatile uint32_t*)LED_ADDR;

    volatile uint64_t* output     = (volatile uint64_t*)BRAM_OUTPUT;

    volatile uint32_t a = 0; 
    volatile uint32_t b = 0; 
    volatile uint32_t c = 0;
    volatile uint64_t c_long = 0;

    
    a = 0xFF;
    b = 0x00;

    vx_printf("Testing ANDN\n");  
    *led = a & ~b;        // accende i led 
    delay();
      
    a = 0x01;
    b = 0xA5;


    vx_printf("Testing ORN\n");  
    *led = a | ~b;  
    delay();

    
    a = 0x5A;

    TEST_RR("ROR", "ror"); 
    TEST_RR("ROL", "rol"); 

    b=0x07;
    TEST_RI("RORI", "rori", 4); 
    
    
    if(XLEN == 64){

	  TEST64_RR("RORW", "rorw"); 
          TEST64_RR("ROLW", "rolw"); 
	  TEST64_RI("RORIW", "roriw", 4); 
    }

    TEST_RR("PACK", "pack"); 
    TEST_RR("PACKH", "packh"); 

    if(XLEN == 64){
	 TEST64_RR("PACKW", "packw"); 
    }

    TEST_R("BREV8", "brev8"); 
    TEST_R("REV8", "rev8"); 

    if(XLEN == 32){

	 TEST_R("ZIP", "zip"); 
	 TEST_R("UNZIP", "unzip"); 
    }


    TEST_RR("CLMUL", "clmul"); 
    TEST_RR("CLMULH", "clmulh"); 
    TEST_RR("XPERM8", "xperm8"); 
    TEST_RR("XPERM4", "xperm4"); 

     
    if(XLEN == 32){
	 TEST_RRI("AES32DSI", "aes32dsi", 3); 
	 TEST_RRI("AES32DSMI", "aes32dsmi", 2); 
    }

    if(XLEN == 64){
	
	TEST_RR("AES64DS", "aes64ds"); 
	TEST_RR("AES64DSM", "aes64dsm"); 
	TEST_R("AES64IM", "aes64im"); 
	TEST_RI("AES64KS1I", "aes64ks1i", 5); 
	TEST_RR("AES64KS2", "aes64ks2"); 
    }

     if(XLEN == 32){
         TEST_RRI("AES32ESI", "aes32esi", 2); 
	 TEST_RRI("AES32ESMI", "aes32esmi", 3); 
      }

      if(XLEN == 64){
        TEST_RR("AES64ES", "aes64es"); 
	TEST_RR("AES64ESM", "aes64esm"); 
      }

      TEST_R("SHA256SIG0", "sha256sig0"); 
      TEST_R("SHA256SIG1", "sha256sig1"); 
      TEST_R("SHA256SUM0", "sha256sum0"); 
      TEST_R("SHA256SUM1", "sha256sum1"); 

      if(XLEN == 32){
	 
	TEST_RR("SHA512SIG0H", "sha512sig0h"); 
	TEST_RR("SHA512SIG0L", "sha512sig0l"); 
	TEST_RR("SHA512SIG1H", "sha512sig1h"); 
	TEST_RR("SHA512SIG1L", "sha512sig1l"); 
	TEST_RR("SHA512SUM0R", "sha512sum0r"); 
	TEST_RR("SHA512SUM1R", "sha512sum1r"); 
	
      }

      if(XLEN == 64){
	
	TEST_R("SHA512SIG0", "sha512sig0"); 
	TEST_R("SHA512SIG1", "sha512sig1"); 
	TEST_R("SHA512SUM0", "sha512sum0"); 
	TEST_R("SHA512SUM1", "sha512sum1"); 


  	  }


  
}

int main() {
    vx_printf(">> Crypto starting (1 thread)\n");

    uint32_t num_threads = 1;
    vx_spawn_threads(1, &num_threads, nullptr, (vx_kernel_func_cb)blinky_kernel, nullptr);

    vx_printf(">> Crypto finished\n");

    return 0;
}

