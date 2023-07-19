#!/bin/sh
# QXDM Logging Tool - On device logging in external USB
# DIAG_fromWeb.cfg 参考qulcomm/QXMD.md
# bat script as bellow
# REM @Install QXDM CFG and script file on Device
#
# adb push qxdm_log.sh /tmp
# adb shell "chmod 777 /tmp/qxdm_log.sh"
# 
# adb push DIAG_Default.cfg /tmp
# adb shell "chmod 777 /tmp/DIAG_Default.cfg"
#
# pause

case "$1" in
  on)
      echo "QXDMLogging.sh ON" > /dev/kmsg

      # Change to host mode for USB storage
      echo host > /sys/devices/platform/a600000.ssusb/mode

      # umount usb storage to setup new parameter
      umount /mnt/sdcard

	  # Add some delay for umount
      sleep 2

      # mount usb storage new parameter
      mount -o rw,nosuid,nodev,noexec,relatime,fmask=0000,dmask=0000,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro /dev/sda1 /mnt/sdcard

	  # Add some delay for mount
      sleep 2

      # Start diag_mdlog to collect QXDM log
	  # -s 200: 200MB / -n 5: Total 5 files / -c : loop recording with 5 files

      if [ -f "/tmp/DIAG_fromWeb.cfg" ]; then
          echo "File /tmp/DIAG_fromWeb.cfg exists." > /dev/kmsg
          mv /tmp/DIAG_fromWeb.cfg /foxusr/
          sync;sync
          diag_mdlog -f /foxusr/DIAG_fromWeb.cfg -o /mnt/sdcard -s 200 -n 5 -c &
      else
          echo "File /tmp/DIAG_fromWeb.cfg does not exists." > /dev/kmsg
          diag_mdlog -f /usr/web-script/DIAG_Default.cfg -o /mnt/sdcard -s 200 -n 5 -c &
      fi

      ;;
  off)
      echo "QXDMLogging.sh OFF" > /dev/kmsg
      # Create foldor
      FOLDOR_NAME=/mnt/sdcard/$(date +"%Y%m%d_%H%M%S")

      if [ -d $FOLDOR_NAME ]; then
          echo "Directory /path/to/dir exists." > /dev/kmsg
          rm -rf $FOLDOR_NAME
      fi

      echo $FOLDOR_NAME > /dev/kmsg
      mkdir -p $FOLDOR_NAME
      mkdir -p $FOLDOR_NAME/foxusr
      mkdir -p $FOLDOR_NAME/foxusr/log
      mkdir -p $FOLDOR_NAME/foxusr/tmp

      #Backup system log
      dmesg > $FOLDOR_NAME/dmesg.log
      ps > $FOLDOR_NAME/ps.log
      free > $FOLDOR_NAME/mem.log
      cp -R /foxusr/* $FOLDOR_NAME/foxusr
      cp -R /var/volatile/log/* $FOLDOR_NAME/foxusr/log
      cp -R /tmp/* $FOLDOR_NAME/foxusr/tmp
      cp -R /firmware/image/Ver_Info.txt $FOLDOR_NAME/Ver_Info.txt

      echo "/sys/class/oem/hw/product_name" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/hw/product_name >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/hw/hw_ver" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/hw/hw_ver >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log      
      echo "/sys/class/oem/hw/rf_card_type" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/hw/rf_card_type >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/hw/mmw_card_type" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/hw/mmw_card_type >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/sw/apps_ver" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/sw/apps_ver >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/sw/amss_ver" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/sw/amss_ver >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/sw/sbl_ver" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/sw/sbl_ver >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/sw/uefi_ver" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/sw/uefi_ver >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/oem/sw/rf_cfg" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/oem/sw/rf_cfg >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      echo "/sys/class/power_supply/battery/uevent" >> $FOLDOR_NAME/sys_info.log
      cat /sys/class/power_supply/battery/uevent >> $FOLDOR_NAME/sys_info.log
      echo "" >> $FOLDOR_NAME/sys_info.log
      #cp -R -f $FOLDOR_NAME /mnt/sdcard/

      # Stop diag_mdlog
      kill -9 $(pidof diag_mdlog)
      sleep 1
      sync
      mv -f /mnt/sdcard/*.qdb $FOLDOR_NAME
      mv -f /mnt/sdcard/*.qmdl $FOLDOR_NAME
      mv -f /mnt/sdcard/*.xml $FOLDOR_NAME
      mv -f /mnt/sdcard/mdm $FOLDOR_NAME
      sync

      # umount usb storage to setup new parameter
      umount /mnt/sdcard
      sleep 1

      # Change to peripheral mode for adb command
      echo peripheral > /sys/devices/platform/a600000.ssusb/mode

      ;;
  *)
    #invalid event

      ;;
esac
