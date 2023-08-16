# QUALCOMM

## qcom log

1. use QXDM to generater mask cfg file named Diag.cfg
1. mkdir /sdcard/diag_logs/
1. push Diag.cfg to /sdcard/diag_logs/
1. run diag_mlog, it will output logs in /sdcard/diag_logs/xxx/xxx.qmdl
1. pull diag_***.qmdl file to local place
1. use QXDM to open the qmdl file

可以查看[qxdm脚本](../script/qxdm_log.sh)

## AT

at command | descript
-|-
AT+CFUN=n | n为下值<li>0:最少功能模式, 不断电情况下，此模式下，射频和(U)SIM 卡不工作。 <li>4: 飞行模式 , 此模式下射频不工作。 <li> 1: 全功能模式
AT+QPOWD=n | <li> 0: 立即关机 <li> 1: 正常关机

## WIFI

### How to collect FW/cnss_host logs for MDM9x40/9x50/9x07

1. check if the "gEnablefwlog=1" and "gMulticastHostFwMsgs=1" appeared in "WCNSS_qcom_cfg.ini".

    ```txt
    #Enable firmware log
    gEnablefwlog=1
    # Enable broadcast logging to the userspace entities
    gMulticastHostFwMsgs=1
    ```

2. pls make sure the Data.msc is matched.

    1. Take "Data.msc" from the folder "\<FW source path\>/cnss_proc/wlan/fw/target/.output/AR6320/hw.3/bin/" whose FW version matches the FW version used in the device.

    1. adb push Data.msc /firmware/image/

3. enable fw log output from adb shell

    ```sh
    dmesg -c > /dev/null
    iwpriv wlan0 dl_report 1
    iwpriv wlan0 dl_type 1 //if no fw logs collect, pls try to modify to "iwpriv wlan0 dl_type 3"
    iwpriv wlan0 dl_loglevel 0
    ```

4. use cnss_diag CMD to collect fw log

    ```sh
    cnss_diag -c >/data/fw_log.txt
    ```

Please use "adb shell ls -al /data/fw_log.txt" CMD to check if fw log

in addition, this issue should be not related with wlan fw, you can set gEnablefwlog=0 and skip step2/3

**<<WCNSS_qcom_cfg.ini>> 是高通wlan驱动的配置文件**

### iwpriv 命令可以直接设置获取驱动运行参数

