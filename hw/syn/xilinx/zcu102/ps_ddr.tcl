# ===========
# Tcl script to chose PS DDR as Vortex Global Memory 
# ===========

### Cells 

## Processing System 
# Setting properties in order to enable Slave AXI PORT HP0 (PL master and PS slave) 

set_property -dict [list \
    CONFIG.PSU__SAXIGP2__DATA_WIDTH {128} \
    CONFIG.PSU__USE__S_AXI_GP2 {1} \
  ] [get_bd_cells zynq_ultra_ps_e] 


### Interface connections 

## AXI Smartconnect M00 -> S_AXI_HP0_FPD
connect_bd_intf_net -intf_net smartconnect_M00_AXI [get_bd_intf_pins smartconnect/M00_AXI] [get_bd_intf_pins zynq_ultra_ps_e/S_AXI_HP0_FPD]

### Port connections 
## PL clock -> S_AXI_HP0_FPD clock  
connect_bd_net [get_bd_pins zynq_ultra_ps_e/pl_clk0] [get_bd_pins zynq_ultra_ps_e/saxihp0_fpd_aclk]
 

### Addresses 

## Vortex Address space 

# Mapping a DDR high range in PS address space for Vortex Global Memory 
assign_bd_address -offset $::env(MEM_BASE) -range $::env(MEM_SIZE) -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_DDR_HIGH] -force

# Excluding from Vortex Address space, all other segments referring to other ranges (DDR LOW, PCIE, QSPI) accessible from S_AXI_HP0_FPD 
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_DDR_LOW]
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_LPS_OCM]
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_PCIE_LOW]
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_QSPI]
  

# Excluding from PS address space, ALL segments (DDR HIGH, DDR LOW, PCIE, QSPI) accessible from S_AXI_HP0_FPD -> Theorically would be possible for PS to access as Master Smartconnect through M_AXI_HPM0 to S_AXI_HP0_FPD 

exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_DDR_HIGH]  
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_DDR_LOW]
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_LPS_OCM]
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_PCIE_LOW]
exclude_bd_addr_seg -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] [get_bd_addr_segs zynq_ultra_ps_e/SAXIGP2/HP0_QSPI]


## Debug 

## ILA for AXI Vortex Memory Transaction 
create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_vortex
set_property -dict [list \
    CONFIG.C_MONITOR_TYPE {AXI} \
] [get_bd_cells ila_vortex]

connect_bd_intf_net [get_bd_intf_pins ila_vortex/SLOT_0_AXI] [get_bd_intf_pins Vortex_top/m_axi_mem]
connect_bd_net [get_bd_pins ila_vortex/clk] [get_bd_pins zynq_ultra_ps_e/pl_clk0]


## ILA for Smartconnect  
create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_m00
set_property -dict [list \
    CONFIG.C_MONITOR_TYPE {AXI} \
  ] [get_bd_cells ila_m00]

connect_bd_intf_net [get_bd_intf_pins ila_m00/SLOT_0_AXI] [get_bd_intf_pins smartconnect/M00_AXI]
connect_bd_net [get_bd_pins ila_m00/clk] [get_bd_pins zynq_ultra_ps_e/pl_clk0]


