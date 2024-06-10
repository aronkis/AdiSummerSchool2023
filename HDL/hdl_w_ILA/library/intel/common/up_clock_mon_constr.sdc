###############################################################################
## Copyright (C) 2017-2023 Analog Devices, Inc. All rights reserved.
### SPDX short identifier: ADIBSD
###############################################################################

set_false_path  -from [get_registers *up_clock_mon:*|d_count_run_m3*] -to [get_registers *up_clock_mon:*|up_count_running_m1*]
set_false_path  -from [get_registers *up_clock_mon:*|up_count_run*]   -to [get_registers *up_clock_mon:*|d_count_run_m1*]
set_false_path  -from [get_registers *up_clock_mon:*|d_count*]        -to [get_registers *up_clock_mon:*|up_d_count*]
