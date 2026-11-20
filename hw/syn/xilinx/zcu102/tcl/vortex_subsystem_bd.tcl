####################### 
# Vortex Subsystem 
# This tcl files obtains the Vortex Subsytem IP from the corrisponding Block Design 
#######################


# Create new block design 
set BD_NAME "vortex_subsystem"
create_bd_design $BD_NAME


######################
# Import RTL modules #
#####################
create_bd_cell -type module -reference Vortex_top Vortex_top


############################
# Import and configure IPs #
############################

## DCR_VORTEX
create_bd_cell -type ip -vlnv user.org:user:DCR_Vortex:2.0 DCR_Vortex

## MOD: VX CONSOLE
create_bd_cell -type ip -vlnv user.org:user:vx_console:2.0 vx_console
if {![info exists clk_freq_hz]} { set clk_freq_hz 100000000 }
set_property CONFIG.CLK_FREQ_HZ $clk_freq_hz [get_bd_cells vx_console]

## AXI SMARTCONNECT 
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 smartconnect
set_property -dict [list \
    CONFIG.NUM_MI {2} \
    CONFIG.NUM_SI {1} \
  ] [get_bd_cells smartconnect]


#############
# Interface # 
############# 

## INTERFACE PORTS

# AXI4 Master Interface of Vortex Subsystem 
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_VORTEX
set_property -dict [ list \
   CONFIG.ADDR_WIDTH {64} \
   CONFIG.DATA_WIDTH {512} \
   CONFIG.PROTOCOL {AXI4} \
] [get_bd_intf_port M_AXI_VORTEX]

# AXI-Lite Slave Interface of Vortex Subsystem 
create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_VORTEX

set_property -dict [ list \
   CONFIG.ADDR_WIDTH {13} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.PROTOCOL {AXI4LITE} \
] [get_bd_intf_port S_AXI_VORTEX]



## PORTS 

# Clock port
create_bd_port -dir I -type clk aclk
set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {S_AXI_VORTEX:M_AXI_VORTEX} \
   CONFIG.ASSOCIATED_RESET {aresetn} \
] [get_bd_port aclk] 

# Areset Port
create_bd_port -dir I -type rst aresetn
set_property CONFIG.POLARITY ACTIVE_LOW [get_bd_ports aresetn]

# Uart TX pin 
create_bd_port -dir O uart_tx


###############
# Connections #
###############

## Interface connections

# Vortex m_axi_mem to M_AXI_VORTEX
connect_bd_intf_net [get_bd_intf_ports M_AXI_VORTEX] [get_bd_intf_pins Vortex_top/m_axi_mem]

# S_AXI_VORTEX to S00 AXI Smartconnect
connect_bd_intf_net [get_bd_intf_ports S_AXI_VORTEX] [get_bd_intf_pins smartconnect/S00_AXI]

# M00 AXI Smartconnect to DCR AXI
connect_bd_intf_net [get_bd_intf_pins DCR_Vortex/s00_axi] [get_bd_intf_pins smartconnect/M00_AXI]

# M01 AXI Smartconnect to VX Console AXI
connect_bd_intf_net [get_bd_intf_pins smartconnect/M01_AXI] [get_bd_intf_pins vx_console/s_axi]

## Port connections

# DCR Reset to Vortex Reset
connect_bd_net [get_bd_pins DCR_Vortex/dcr_vx_reset] [get_bd_pins Vortex_top/reset]

# DCR WR ADDR to Vortex dcr_wr_addr
connect_bd_net [get_bd_pins DCR_Vortex/dcr_wr_addr] [get_bd_pins Vortex_top/dcr_wr_addr]

# DCR WR DATA to Vortex dcr_wr_data
connect_bd_net -net DCR_Vortex_dcr_wr_data [get_bd_pins DCR_Vortex/dcr_wr_data] [get_bd_pins Vortex_top/dcr_wr_data]
 
# DCR WR VALID to Vortex dcr_wr_valid
connect_bd_net -net DCR_Vortex_dcr_wr_valid [get_bd_pins DCR_Vortex/dcr_wr_valid] [get_bd_pins Vortex_top/dcr_wr_valid]
  
# Vortex busy to DCR busy 
connect_bd_net [get_bd_pins DCR_Vortex/busy] [get_bd_pins Vortex_top/busy]
  

# External Clock signal to all modules
connect_bd_net [get_bd_ports aclk] [get_bd_pins DCR_Vortex/s00_axi_aclk] [get_bd_pins Vortex_top/clk] [get_bd_pins smartconnect/aclk] [get_bd_pins vx_console/aclk]
  
# External AResetn signal to all modules
connect_bd_net [get_bd_ports aresetn] [get_bd_pins DCR_Vortex/s00_axi_aresetn] [get_bd_pins smartconnect/aresetn] [get_bd_pins vx_console/aresetn]
  

# VX CONSOLE UART Tx to external uart tx pin 
connect_bd_net [get_bd_ports uart_tx] [get_bd_pins vx_console/uart_tx]


###################
# Address Map #
###################

if {[info exists ::env(IO_BASE)]} {
    set CONSOLE_OFFS [expr {$::env(CONSOLE_ADDR) - $::env(IO_BASE)}]
    set DCR_OFFS     [expr {$::env(DCR_ADDR)     - $::env(IO_BASE)}]
    set CONSOLE_SIZE $::env(CONSOLE_SIZE)
    set DCR_SIZE     $::env(DCR_SIZE)
} else {
    set CONSOLE_OFFS 0x0000
    set DCR_OFFS     0x1000
    set CONSOLE_SIZE 4K
    set DCR_SIZE     4K
}


## EXTERNAL MASTER (S_AXI_SLAVE)

assign_bd_address -offset $CONSOLE_OFFS -range $CONSOLE_SIZE \
    -target_address_space [get_bd_addr_spaces /S_AXI_VORTEX] \
    [get_bd_addr_segs vx_console/s_axi/reg0] -force

# Mapping DCR Vortex registers 
assign_bd_address -offset $DCR_OFFS -range $DCR_SIZE \
    -target_address_space [get_bd_addr_spaces /S_AXI_VORTEX] \
    [get_bd_addr_segs DCR_Vortex/s00_axi/reg0] -force


## VORTEX (M_AXI_VORTEX)

assign_bd_address -offset 0 -range 32G \
  -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] \
   [get_bd_addr_segs M_AXI_VORTEX/Reg] -force


# Validate, save and create the HDL wrapper
validate_bd_design
save_bd_design
make_wrapper -files [get_files $BD_NAME.bd] -top -import
generate_target all [get_files $BD_NAME.bd]

# Package the Block Design as an IP

if {![info exists ip_repo_out]} { set ip_repo_out ./ip_repo/vortex_sys } 

set VENDOR      "user.org"
set LIBRARY     "user"
set IP_REPO     ${ip_repo_out} 
set VERSION     "1.0"
set IP_NAME     "vortex_sys"

ipx::package_project \
    -root_dir $IP_REPO \
    -vendor $VENDOR \
    -library $LIBRARY \
    -taxonomy /UserIP \
    -module $BD_NAME \
    -import_files

set_property display_name $BD_NAME [ipx::current_core]

# Remove the fixed FREQ_HZ so IP Integrator propagates it from the parent's clock
foreach intf {CLK.ACLK M_AXI_VORTEX S_AXI_VORTEX} {
    set bi [ipx::get_bus_interfaces $intf -of_objects [ipx::current_core]]
    if {$bi ne ""} { ipx::remove_bus_parameter FREQ_HZ $bi }
}
ipx::update_checksums [ipx::current_core]

ipx::save_core [ipx::current_core]
set_property ip_repo_paths [concat [get_property ip_repo_paths [current_project]] [file normalize $IP_REPO]] [current_project]

update_ip_catalog
