connect
fpga "bare/design_1_wrapper.bit"
targets -set -nocase -filter {name =~ "*PSU*"}
mask_write 0xFFCA0038 0x1C0 0x1C0
targets -set -nocase -filter {name =~ "*MicroBlaze PMU*"}
dow "bare/pmufw.elf"
con 
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor -clear-registers
dow "bare/fsbl.elf"
con 
after 4000
stop
puts "Ready to run an application elf, leaving hw_server running"
