# Check arguments
if { $::argc != 1 } {
    puts "ERROR: Program \"$::argv0\" requires 1 argument!"
    puts "Usage: $::argv0 <vivado_project.xpr>"
    exit
}

# Get project path
set project_file [lindex $::argv 0]

if {![file exists $project_file]} {
    puts "ERROR: Project file not found: $project_file"
    exit
}

puts "Opening project: $project_file"

# Open project
open_project $project_file


# Get top module name
set top_module [get_property top [get_filesets sources_1]]

puts "Top module: $top_module"


# Open implementation run
open_run impl_1

# Generate XSA named after top module
set xsa_file "${top_module}.xsa"

puts "Generating XSA: $xsa_file"

write_hw_platform \
    -fixed \
    -include_bit \
    -force \
    -file $xsa_file

close_project

exit
