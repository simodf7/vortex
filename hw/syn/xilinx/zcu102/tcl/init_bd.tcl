if { $::argc != 3 } {
    puts "ERROR: Program \"$::argv0\" requires 3 arguments!\n"
    puts "Usage: $::argv0 <device_part> <vcs_file> <config_dir>\n"
    exit
}

set device_part [lindex $::argv 0]
set vcs_file    [lindex $::argv 1]
set config_dir  [lindex $::argv 2] 

set tool_dir   $::env(TOOL_DIR)
set script_dir [ file dirname [ file normalize [ info script ] ] ]
set ip_repo $script_dir/ip_repo
set config_dir [ file normalize $config_dir] 

puts "Using device_part=$device_part"
puts "Using vcs_file=$vcs_file"
puts "Using tool_dir=$tool_dir"
puts "Using script_dir=$script_dir"
puts "Using ip_repo=$ip_repo"
puts "Using config_dir=$config_dir"

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
set project_name "project"
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

set repo_path [file normalize $ip_repo]
set_property ip_repo_paths [list $repo_path] [current_project]
update_ip_catalog

source $script_dir/import_ip.tcl

######################
# Project properties #
######################

set project_dir [get_property directory [current_project]]


##########################
# Creating block diagram #
##########################

source $script_dir/soc_bd.tcl

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
import_files -fileset sim_1 $config_dir/src/$testbench.v
set_property top testbench [get_filesets sim_1]


##################
# Apri il the BD #
##################

open_bd_design [get_files design_1.bd]

puts "==========================================================="
puts " Progetto creato e block design costruito."
puts " Nessuna sintesi eseguita."
puts "==========================================================="
