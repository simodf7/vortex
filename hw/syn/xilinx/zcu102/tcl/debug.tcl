## System ILA 
create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila
set_property -dict [list \
    CONFIG.C_MON_TYPE {MIX} \
    CONFIG.C_NUM_OF_PROBES {5} \
    CONFIG.C_NUM_MONITOR_SLOTS {2} \
    CONFIG.C_DATA_DEPTH {4096} \
    CONFIG.C_SLOT_0_APC_EN {1} \
    CONFIG.C_SLOT_1_APC_EN {1} \
] [get_bd_cells system_ila]

connect_bd_net [get_bd_pins DCR_Vortex/dcr_wr_addr] [get_bd_pins system_ila/probe0]
connect_bd_net [get_bd_pins DCR_Vortex/dcr_wr_valid] [get_bd_pins system_ila/probe1]
connect_bd_net [get_bd_pins DCR_Vortex/dcr_vx_reset] [get_bd_pins system_ila/probe2]
connect_bd_net [get_bd_pins DCR_Vortex/dcr_wr_data] [get_bd_pins system_ila/probe3]
connect_bd_net [get_bd_pins Vortex_top/busy] [get_bd_pins system_ila/probe4]

connect_bd_net [get_bd_pins system_ila/clk] [get_bd_pins zynq_ultra_ps_e/pl_clk0]
connect_bd_net [get_bd_pins proc_sys_reset/peripheral_aresetn] [get_bd_pins system_ila/resetn]

connect_bd_intf_net [get_bd_intf_pins system_ila/SLOT_0_AXI] [get_bd_intf_pins Vortex_top/m_axi_mem]
connect_bd_intf_net [get_bd_intf_pins system_ila/SLOT_1_AXI] [get_bd_intf_pins DCR_Vortex/s00_axi]


