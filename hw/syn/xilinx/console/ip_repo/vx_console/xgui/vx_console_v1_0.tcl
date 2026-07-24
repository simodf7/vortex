# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "AXI_ADDR_W" -parent ${Page_0}
  ipgui::add_param $IPINST -name "BAUD_RATE" -parent ${Page_0}
  ipgui::add_param $IPINST -name "CLK_FREQ_HZ" -parent ${Page_0}
  ipgui::add_param $IPINST -name "COUT_BASE_ADDR" -parent ${Page_0}
  ipgui::add_param $IPINST -name "LINE_LOG2" -parent ${Page_0}
  ipgui::add_param $IPINST -name "NUM_HARTS" -parent ${Page_0}


}

proc update_PARAM_VALUE.AXI_ADDR_W { PARAM_VALUE.AXI_ADDR_W } {
	# Procedure called to update AXI_ADDR_W when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.AXI_ADDR_W { PARAM_VALUE.AXI_ADDR_W } {
	# Procedure called to validate AXI_ADDR_W
	return true
}

proc update_PARAM_VALUE.BAUD_RATE { PARAM_VALUE.BAUD_RATE } {
	# Procedure called to update BAUD_RATE when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.BAUD_RATE { PARAM_VALUE.BAUD_RATE } {
	# Procedure called to validate BAUD_RATE
	return true
}

proc update_PARAM_VALUE.CLK_FREQ_HZ { PARAM_VALUE.CLK_FREQ_HZ } {
	# Procedure called to update CLK_FREQ_HZ when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.CLK_FREQ_HZ { PARAM_VALUE.CLK_FREQ_HZ } {
	# Procedure called to validate CLK_FREQ_HZ
	return true
}

proc update_PARAM_VALUE.COUT_BASE_ADDR { PARAM_VALUE.COUT_BASE_ADDR } {
	# Procedure called to update COUT_BASE_ADDR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.COUT_BASE_ADDR { PARAM_VALUE.COUT_BASE_ADDR } {
	# Procedure called to validate COUT_BASE_ADDR
	return true
}

proc update_PARAM_VALUE.LINE_LOG2 { PARAM_VALUE.LINE_LOG2 } {
	# Procedure called to update LINE_LOG2 when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.LINE_LOG2 { PARAM_VALUE.LINE_LOG2 } {
	# Procedure called to validate LINE_LOG2
	return true
}

proc update_PARAM_VALUE.NUM_HARTS { PARAM_VALUE.NUM_HARTS } {
	# Procedure called to update NUM_HARTS when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.NUM_HARTS { PARAM_VALUE.NUM_HARTS } {
	# Procedure called to validate NUM_HARTS
	return true
}


proc update_MODELPARAM_VALUE.CLK_FREQ_HZ { MODELPARAM_VALUE.CLK_FREQ_HZ PARAM_VALUE.CLK_FREQ_HZ } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.CLK_FREQ_HZ}] ${MODELPARAM_VALUE.CLK_FREQ_HZ}
}

proc update_MODELPARAM_VALUE.BAUD_RATE { MODELPARAM_VALUE.BAUD_RATE PARAM_VALUE.BAUD_RATE } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.BAUD_RATE}] ${MODELPARAM_VALUE.BAUD_RATE}
}

proc update_MODELPARAM_VALUE.AXI_ADDR_W { MODELPARAM_VALUE.AXI_ADDR_W PARAM_VALUE.AXI_ADDR_W } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.AXI_ADDR_W}] ${MODELPARAM_VALUE.AXI_ADDR_W}
}

proc update_MODELPARAM_VALUE.COUT_BASE_ADDR { MODELPARAM_VALUE.COUT_BASE_ADDR PARAM_VALUE.COUT_BASE_ADDR } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.COUT_BASE_ADDR}] ${MODELPARAM_VALUE.COUT_BASE_ADDR}
}

proc update_MODELPARAM_VALUE.NUM_HARTS { MODELPARAM_VALUE.NUM_HARTS PARAM_VALUE.NUM_HARTS } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.NUM_HARTS}] ${MODELPARAM_VALUE.NUM_HARTS}
}

proc update_MODELPARAM_VALUE.LINE_LOG2 { MODELPARAM_VALUE.LINE_LOG2 PARAM_VALUE.LINE_LOG2 } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.LINE_LOG2}] ${MODELPARAM_VALUE.LINE_LOG2}
}

