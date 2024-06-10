set_param project.enableReportConfiguration 0
load_feature core
current_fileset
xsim {work.verilog_task_testbench_tb} -autoloadwcfg -tclbatch {xsim_gui_cmd.tcl}
