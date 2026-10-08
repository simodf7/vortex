## import_ip.tcl ##


# FPU (xil_fma, xil_fdiv, xil_fsqrt, xil_fmul, xil_fadd) 

if {[info exists ::env(FPU_IP)]} {
  set ip_dir $::env(FPU_IP)

  set ::argv [list $ip_dir $device_part]
  set ::argc 2
  source ${tool_dir}/xilinx_ip_gen.tcl

  set xci_list {}
  foreach ip {xil_fma xil_fdiv xil_fsqrt xil_fmul xil_fadd} {
    lappend xci_list "${ip_dir}/${ip}/${ip}.xci"
  }
  import_ip $xci_list
}

# CHIPSCOPE + DBG_SCOPE_* 

proc create_ila {name depth widths} {
  create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name $name
  set props [list CONFIG.C_ADV_TRIGGER {true} \
                  CONFIG.C_EN_STRG_QUAL {1} \
                  CONFIG.C_DATA_DEPTH $depth \
                  CONFIG.C_NUM_OF_PROBES [llength $widths] \
                  CONFIG.ALL_PROBE_SAME_MU {true} \
                  CONFIG.ALL_PROBE_SAME_MU_CNT {4}]
  set i 0
  foreach w $widths {
    lappend props CONFIG.C_PROBE${i}_WIDTH $w
    incr i
  }
  set_property -dict $props [get_ips $name]
  generate_target {instantiation_template} [get_files $name.xci]
  set_property generate_synth_checkpoint false [get_files $name.xci]
}

set chipscope 0
set ila_lsu   0
set ila_issue 0

foreach def $vdefines_list {
  set name [lindex [split $def "="] 0]
  if { $name == "CHIPSCOPE" }       { set chipscope 1 }
  if { $name == "DBG_SCOPE_LSU" }   { set ila_lsu 1 }
  if { $name == "DBG_SCOPE_ISSUE" } { set ila_issue 1 }
}
puts "import_ip.tcl: FPU_IP=[info exists ::env(FPU_IP)] chipscope=$chipscope lsu=$ila_lsu issue=$ila_issue"


if { $chipscope == 1 } {
  if { $ila_lsu == 1 }   { create_ila ila_lsu   1024 {1024 1024 512} }
  if { $ila_issue == 1 } { create_ila ila_issue 1024 {512 512 1024 512} }
}
