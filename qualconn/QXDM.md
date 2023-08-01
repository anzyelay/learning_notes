# QUALCOMM

## qcom log

1. use QXDM to generater mask cfg file named Diag.cfg
1. mkdir /sdcard/diag_logs/
1. push Diag.cfg to /sdcard/diag_logs/
1. run diag_mlog, it will output logs in /sdcard/diag_logs/xxx/xxx.qmdl
1. pull diag_***.qmdl file to local place
1. use QXDM to open the qmdl file

可以查看[qxdm脚本](../script/qxdm_log.sh)


## at

at command | descript
-|-
AT+CFUN=n | n为下值<li>0:最少功能模式, 不断电情况下，此模式下，射频和(U)SIM 卡不工作。 <li>4: 飞行模式 , 此模式下射频不工作。 <li> 1: 全功能模式
AT+QPOWD=n | <li> 0: 立即关机 <li> 1: 正常关机
