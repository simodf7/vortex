# The following tcl is used to obtain: 
# - fsbl.elf
# - pmufw.elf 
# - lscript.ld


set build_dir [file normalize [lindex $::argv 0]]
set xsa_dir "~/tesi/builds"
set config $::env(CONFIG)

set platform_name $config
append platform_name "_" "platform"
set domain_name "a53_standalone"
set app_name "lscript_gen"


set xsa_name "design_1_wrapper"
set xsa [file normalize $xsa_dir/$config/$xsa_name.xsa]
set temp_dir $build_dir/tmp
set ws  $temp_dir/ws

setws $ws

# Creating a platform for ZynqMP, Vitis creates the fsbl and pmu firmware as boot components by default
platform create -name $platform_name -hw $xsa -out $ws

domain create -name $domain_name -os standalone -proc psu_cortexa53_0
domain active $domain_name

platform write
platform generate

# Create an Empty application just to obtain the linkerscript
app create -name $app_name -platform $platform_name -domain $domain_name \
           -template {Empty Application(C)}

file copy -force $ws/$platform_name/hw/$xsa_name.bit $build_dir
file copy -force $ws/$platform_name/export/$platform_name/sw/$platform_name/boot/pmufw.elf $build_dir
file copy -force $ws/$platform_name/export/$platform_name/sw/$platform_name/boot/fsbl.elf $build_dir
file copy -force $ws/$app_name/src/lscript.ld $build_dir
file copy -force $ws/$platform_name/export/$platform_name/sw/$platform_name/$domain_name/bsplib/lib/libxil.a $build_dir
file copy -force $ws/$platform_name/export/$platform_name/sw/$platform_name/$domain_name/bspinclude/include $build_dir/include

