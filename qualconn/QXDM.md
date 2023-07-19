# QUALCOMM

## qcom log

1. use QXDM to generater mask cfg file named Diag.cfg
1. mkdir /sdcard/diag_logs/
1. push Diag.cfg to /sdcard/diag_logs/
1. run diag_mlog, it will output logs in /sdcard/diag_logs/xxx/xxx.qmdl
1. pull diag_***.qmdl file to local place
1. use QXDM to open the qmdl file

可以查看[qxdm脚本](../script/qxdm_log.sh)
