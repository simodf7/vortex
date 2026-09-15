# ============= 
# Tcl script to insert PL-side DDR as Vortex Global Memory 
# ============= 

### Cells 

## DDR4 (MIG)

# Board interfaces retrieve all paramaters as Memory Part, Timing, Pinout from Board file
# Parameters:
# AXI DATA WIDTH = 128 (max value possible as the available bandwith) 
# AXI ADDRESS WIDTH = 29 
# AXI ID WIDTH = 1 
# SYSTEM CLOCK = differential 
# INPUT CLOCK PERIOD = 3332 
# TIME PERIOD =  833

create_bd_cell -type ip -vlnv xilinx.com:ip:ddr4:2.2 ddr4
set_property -dict [list \
    CONFIG.C0_CLOCK_BOARD_INTERFACE {user_si570_sysclk} \
    CONFIG.C0_DDR4_BOARD_INTERFACE  {ddr4_sdram_062} \
    CONFIG.RESET_BOARD_INTERFACE    {Custom} \
] [get_bd_cells ddr4]



## Inverter for reset 
# ui_clk_sync_rst e' active high, aresetn wants active low.

create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 ddr_rstn
set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE      {1} \
] [get_bd_cells ddr_rstn]



### Interface Ports 

## DDR Physical pins (connection to pinout is driven by board file) 
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddr4_rtl:1.0 ddr4_sdram_062

## Differential System Clock 300 MHz (Si570 oscillator).
# All clocks (ui_clk) are derived from this 

create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 user_si570_sysclk
set_property CONFIG.FREQ_HZ {300000000} [get_bd_intf_ports user_si570_sysclk]


### Interface Connection 

## MIG <-> DDR physical pins 

connect_bd_intf_net -intf_net ddr4_C0_DDR4 \
    [get_bd_intf_ports ddr4_sdram_062] \
    [get_bd_intf_pins ddr4/C0_DDR4]

## System Clock -> MIG 
connect_bd_intf_net -intf_net user_si570_sysclk_1 \
    [get_bd_intf_ports user_si570_sysclk] \
    [get_bd_intf_pins ddr4/C0_SYS_CLK]

## SmartConnect M00 <-> MIG AXI slave port 
connect_bd_intf_net -intf_net smartconnect_M00_AXI \
    [get_bd_intf_pins smartconnect/M00_AXI] \
    [get_bd_intf_pins ddr4/C0_DDR4_S_AXI]


### Port connections 

# ---------------------------------------------------------
# Clock e reset
#
# Sequenza, dall'accensione:
#   1. sys_rst resetta il controller
#   2. il MIG genera ui_clk e tiene ui_clk_sync_rst ALTO
#   3. finita la calibrazione, ui_clk_sync_rst scende
#   5. la porta AXI del MIG esce dal reset ed e' utilizzabile
# ---------------------------------------------------------

# sys_rst: reset di sistema, attivo alto, dominio pl_clk0.
## Deve venire da fuori: ui_clk lo genera il MIG stesso.
connect_bd_net -net proc_sys_reset_peripheral_reset \
    [get_bd_pins proc_sys_reset/peripheral_reset] \
    [get_bd_pins ddr4/sys_rst]
 
## ui_clk: clock GENERATO dal MIG. E' il dominio della sua porta
## AXI; lo SmartConnect lo riceve su aclk1 per il CDC.
connect_bd_net -net ddr4_c0_ddr4_ui_clk \
    [get_bd_pins ddr4/c0_ddr4_ui_clk] \
    [get_bd_pins smartconnect/aclk1]
 
## ui_clk_sync_rst -> invertitore
connect_bd_net -net ddr4_c0_ddr4_ui_clk_sync_rst \
    [get_bd_pins ddr4/c0_ddr4_ui_clk_sync_rst] \
    [get_bd_pins ddr_rstn/Op1]
 
## Lo STESSO aresetn al MIG e a chi gli parla.
## E' questa la riga che impedisce allo SmartConnect di inoltrare
## transazioni mentre la DDR sta ancora calibrando.
connect_bd_net -net ddr_rstn_Res \
    [get_bd_pins ddr_rstn/Res] \
    [get_bd_pins ddr4/c0_ddr4_aresetn] \
    [get_bd_pins smartconnect/aresetn]



### Addresses 

# addresses must be chosen in pl_ddr.mk 

## VORTEX Address space 

# Mapping DDR in Vortex address space 

assign_bd_address -offset $::env(MEM_BASE) -range $::env(MEM_SIZE) \
    -target_address_space [get_bd_addr_spaces Vortex_top/m_axi_mem] \
		[get_bd_addr_segs ddr4/C0_DDR4_MEMORY_MAP/C0_DDR4_ADDRESS_BLOCK] -force


## ZYNQ ULTRASCALE PS Address Space  

# Mapping DDR in PS address space 
assign_bd_address -offset $::env(MEM_BASE) -range $::env(MEM_SIZE) \
    -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e/Data] \
	 [get_bd_addr_segs ddr4/C0_DDR4_MEMORY_MAP/C0_DDR4_ADDRESS_BLOCK] -force 

