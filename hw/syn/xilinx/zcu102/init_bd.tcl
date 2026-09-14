if { $::argc != 2 } {
    puts "ERROR: Program \"$::argv0\" requires 2 arguments!\n"
    puts "Usage: $::argv0 <device_part> <vcs_file>\n"
    exit
}

set device_part [lindex $::argv 0]
set vcs_file    [lindex $::argv 1]

set tool_dir   $::env(TOOL_DIR)
set script_dir [ file dirname [ file normalize [ info script ] ] ]

puts "Using device_part=$device_part"
puts "Using vcs_file=$vcs_file"
puts "Using tool_dir=$tool_dir"
puts "Using script_dir=$script_dir"


###########################
## Setup Vivado Project  ##
###########################

## Parsing vcs file ##

source "${tool_dir}/parse_vcs_list.tcl"
set vlist [parse_vcs_list "${vcs_file}"]

set vsources_list  [lindex $vlist 0]
set vincludes_list [lindex $vlist 1]
set vdefines_list  [lindex $vlist 2]

# Create new project
set project_name "project_1"
create_project $project_name $project_name -force -part $device_part

# Set project board part as ZCU102
set_property BOARD_PART xilinx.com:zcu102:part0:3.4 [current_project]


###################
# Verilog defines #
###################

set_property verilog_define $vdefines_list [current_fileset]


###############
# Add sources #
###############

add_files -norecurse -verbose -fileset sources_1 ${vsources_list}


###################
# Add constraints #
###################

add_files -norecurse -verbose -fileset constrs_1 "$script_dir/top.xdc"


##############
# Import IPs #
##############

set repo_path [file normalize "ip_repo"]
set_property ip_repo_paths [list $repo_path] [current_project]
update_ip_catalog

# create fpu ip
if {[info exists ::env(FPU_IP)]} {
  set ip_dir $::env(FPU_IP)
  set ::argv [list $ip_dir $device_part]
  set ::argc 2
  puts $device_part
  source ${tool_dir}/xilinx_ip_gen.tcl
}

# add fpu ip
if {[info exists ::env(FPU_IP)]} {
  set ip_dir $::env(FPU_IP)

  set xci_list [list \
      "${ip_dir}/xil_fma/xil_fma.xci" \
      "${ip_dir}/xil_fdiv/xil_fdiv.xci" \
      "${ip_dir}/xil_fsqrt/xil_fsqrt.xci" \
      "${ip_dir}/xil_fmul/xil_fmul.xci" \
      "${ip_dir}/xil_fadd/xil_fadd.xci"
  ]

  import_ip $xci_list
}


######################
# Project properties #
######################

set project_dir [get_property directory [current_project]]


##########################
# Creating block diagram #
##########################

source soc_bd.tcl

set_property GENERATE_SYNTH_CHECKPOINT "1"   [get_files design_1.bd]
set_property SYNTH_CHECKPOINT_MODE "Hierarchical" [get_files design_1.bd]

# Set top level module
set_property top "design_1_wrapper" [current_fileset]

# register compilation hooks
set_property STEPS.OPT_DESIGN.TCL.PRE ${script_dir}/pre_opt_hook.tcl [get_runs impl_1]

# Generate compilation order
update_compile_order -fileset sources_1


##############
# Simulation #
##############

set testbench "testbench"
import_files -fileset sim_1 $project_dir/src/$testbench.v
set_property top testbench [get_filesets sim_1]


##################
# Apri il the BD #
##################

open_bd_design [get_files design_1.bd]

puts "==========================================================="
puts " Progetto creato e block design costruito."
puts " Nessuna sintesi eseguita."
puts "==========================================================="
