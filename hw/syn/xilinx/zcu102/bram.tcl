# =========== 
# Tcl script to choose BRAM as Vortex Global Memory 
# =========== 

### Cells

## AXI BRAM CONTROLLER
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl
set_property CONFIG.DATA_WIDTH {512} [get_bd_cells axi_bram_ctrl]

## BRAM
# 16384 parole da 512 bit = 1 MB.
create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 axi_bram_ctrl_bram
set_property -dict [list \
    CONFIG.Assume_Synchronous_Clk {true} \
    CONFIG.EN_SAFETY_CKT {true} \
    CONFIG.Enable_32bit_Address {true} \
    CONFIG.Fill_Remaining_Memory_Locations {true} \
    CONFIG.Load_Init_File {false} \
    CONFIG.Memory_Type {Simple_Dual_Port_RAM} \
    CONFIG.Operating_Mode_A {NO_CHANGE} \
    CONFIG.Read_Width_B {512} \
    CONFIG.Register_PortB_Output_of_Memory_Primitives {false} \
    CONFIG.Use_RSTB_Pin {true} \
    CONFIG.Write_Depth_A {16384} \
    CONFIG.Write_Width_A {512} \
    CONFIG.use_bram_block {Stand_Alone} \
] [get_bd_cells axi_bram_ctrl_bram]



### Interface connections 

## Controller -> BRAM, port A
connect_bd_intf_net -intf_net axi_bram_ctrl_BRAM_PORTA \
    [get_bd_intf_pins axi_bram_ctrl/BRAM_PORTA] \
    [get_bd_intf_pins axi_bram_ctrl_bram/BRAM_PORTA]

## Controller -> BRAM, port B 
connect_bd_intf_net -intf_net axi_bram_ctrl_BRAM_PORTB \
    [get_bd_intf_pins axi_bram_ctrl/BRAM_PORTB] \
    [get_bd_intf_pins axi_bram_ctrl_bram/BRAM_PORTB]

## AXI SmartConnect M00 -> BRAM Controller 
connect_bd_intf_net -intf_net smartconnect_M00_AXI \
    [get_bd_intf_pins smartconnect/M00_AXI] \
    [get_bd_intf_pins axi_bram_ctrl/S_AXI]


### Port connections 

# Clock
connect_bd_net [get_bd_pins proc_sys_reset/peripheral_aresetn] \
               [get_bd_pins axi_bram_ctrl/s_axi_aresetn]


# Reset 
connect_bd_net [get_bd_pins zynq_ultra_ps_e/pl_clk0] \
               [get_bd_pins axi_bram_ctrl/s_axi_aclk]


### Addresses 

## VORTEX Address space 

# Mapping BRAM in Vortex address space
assign_bd_address -offset $::env(MEM_BASE) -range $::env(MEM_SIZE)  \
    -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] \
    [get_bd_addr_segs axi_bram_ctrl/S_AXI/Mem0] -force

## ZYNQ ULTRASCALE PS Address Space  

# Mapping BRAM in PS address space
assign_bd_address -offset $::env(MEM_BASE) -range $::env(MEM:SIZE)  \
    -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] \
    [get_bd_addr_segs axi_bram_ctrl/S_AXI/Mem0] -force











