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


## PSR del dominio ui_clk
# MIG generates a 300 Mhz (so another clock domain). 
# We need a second Processsor System Reset because reset must be synchronized 
# on ui_clk and not on pl_ck0 

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_ddr4_0_300M

## Set Smartconnect NUM_CLKS property to add a new clock domain 
set_property CONFIG.NUM_CLKS {2} [get_bd_cells smartconnect]



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
#   4. il PSR lo converte in un aresetn pulito e sincrono
#   5. la porta AXI del MIG esce dal reset ed e' utilizzabile
# ---------------------------------------------------------

## DCR vx reset to SYS rst 

## sys_rst del MIG: si aggancia alla net gia' pilotata da
## DCR_Vortex/dcr_vx_reset (creata in soc_bd.tcl).
## Indicare il pin sorgente aggiunge il nuovo pin alla net
## esistente, senza doverla ridichiarare con tutti i suoi pin.
connect_bd_net [get_bd_pins DCR_Vortex/dcr_vx_reset] \
               [get_bd_pins ddr4/sys_rst]


## UI CLK to ACLK1 smartconnect and to DDR PSR 

## ui_clk: clock GENERATO dal MIG, non ricevuto.
## E' il dominio in cui vive la sua porta AXI. Va a due posti:
##   - al PSR, che deve sincronizzare il reset su questo clock
##   - allo SmartConnect (aclk1), che cosi' sa che esiste un
##     secondo dominio e inserisce il CDC da solo
connect_bd_net -net ddr4_c0_ddr4_ui_clk \
    [get_bd_pins ddr4/c0_ddr4_ui_clk] \
    [get_bd_pins rst_ddr4_0_300M/slowest_sync_clk] \
    [get_bd_pins smartconnect/aclk1]

## UI SYNC RST to DDR PSR EXT RESET IN 

## ui_clk_sync_rst: reset attivo ALTO generato dal MIG, gia'
## sincrono su ui_clk. Resta alto finche' il controller non e'
## pronto. Entra nel PSR come reset esterno.
connect_bd_net -net ddr4_c0_ddr4_ui_clk_sync_rst \
    [get_bd_pins ddr4/c0_ddr4_ui_clk_sync_rst] \
    [get_bd_pins rst_ddr4_0_300M/ext_reset_in]

## 

## Ritorno: il PSR produce un reset attivo BASSO, pulito e
## sincrono su ui_clk, che sblocca la porta AXI del MIG.
connect_bd_net -net rst_ddr4_0_300M_peripheral_aresetn \
    [get_bd_pins rst_ddr4_0_300M/peripheral_aresetn] \
    [get_bd_pins ddr4/c0_ddr4_aresetn]


### Addresses 
# TODO
