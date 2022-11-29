#!/system/bin/sh

ENVFILE=/data/misc/fac.env
function saveenv() {
	echo "AP2G_IP=$1" > $ENVFILE
	echo "AP5G_IP=$2" >> $ENVFILE
	echo "AP2G_SSID=$3" >> $ENVFILE
	echo "AP5G_SSID=$4" >> $ENVFILE
	echo "BURNIN_TIME=$5" >> $ENVFILE
}
function getenv() {
	if [ ! -f "$ENVFILE" ]; then
		saveenv 192.168.2.2 192.168.5.5 T99H316-2G T99H316-5G 240
	fi
	source $ENVFILE
	echo "AP2G_IP=$AP2G_IP"
	echo "AP5G_IP=$AP5G_IP"
	echo "AP2G_SSID=$AP2G_SSID"
	echo "AP5G_SSID=$AP5G_SSID"
	echo "BURNIN_TIME=$BURNIN_TIME"
}
WPA_CLI="wpa_cli -iwlan0 -p /data/vendor/wifi/wpa/sockets"
commands="
prepare
switch_to_user_firmware [ota-file.zip] [ota-file-md5]
get_env
set_env AP_2G_IP AP_5G_IP AP_2G_ssid AP_5G_ssid BurnInTime(min)
set_mac
get_version [all]
get_wired_mac
get_wifi_mac
get_bt_mac
get_wifi_scan
get_wifi_rssi
check_rs232
test_wifi_2g (connect to AP, do throughput test)
test_wifi_5g
test_wifi_2g_with_ping (connect to AP, do ping, then do throughput test)
test_wifi_5g_with_ping
test_wifi_2g_connect (just connect to AP)
test_wifi_5g_connect
bt_throughput_server [server-name, default is T99H316-BT]
bt_throughput_client [-R] [target-name, default is T99H316-BT]
test_ethernet_giga
check_bt
set_led_on
set_led_off
set_led_blink
button [up|down|both]
test_ddr [size in MBytes, default is 256, i.e. 256MB]
test_flash [size in MBytes, default is 100, i.e. 100MB]
get_hw_current [5] default is 5 seconds
get_wvkey_sha256
get_hdcp14_sha256
check_hdcp14 (check auth)
check_hdcp22 (check auth)
check_hdcp22_key (check key sha256)
check_prpriv (check key sha256)
check_prpub (check key sha256)
check_as_key (check key sha256)
set_as_key
set_playready_pub
set_playready_priv
set_deviceid
get_deviceid
set_serial
get_serial
set_ctei_sn
get_ctei_sn
set_prod
set_bt_mac
set_burnin
check_burnin
set_stress
check_stress
check_usb
check_sdcard
get_hdmi_res
get_hdmitx_edid
play_video [video-path]
play_audio
stop_video
stop_audio
test_btrcu
nand {read|write}
enable_rx
check_audio_record
check_channelkey {up|down}
check_remote
get_antenna_rssi
check_remote_ext"

WIFI_2G_LOG=/data/misc/fac2g.log
WIFI_5G_LOG=/data/misc/fac5g.log
decoded_chars='A-Za-z0-9\n _;.:#+*/"!'
encoded_chars='mn0opj\nYZk
_;.rGJX6IHu12tsWVU5ql3a:e#+zFTEDRS4CBAyxwvd*b/f"9gQPONch8!iMKL7'

enable_wifi_timeout=20
iperf3paramTX="--forceflush -i 1 -t 20 -P5 -b0"
iperf3paramRX="--forceflush -i 1 -t 20 -P5 -b0 -R"
pingcount=6

ANDROID_VER=`getprop ro.build.version.release`

# trap ctrl-c and call ctrl_c()
#trap ctrl_c INT

sha256_hdcp22=eb32e59f4c46886a7a863912234815a9bec9841f365bf97ee500427a5f943b29
sha256_prppub=98542f2183349fef235ee559646582e7fb38d4c9428fc5ec408aa3a0c84133ca
sha256_prpriv=7423b11c61b3e3eead10c7fc0be524e944a48fc523824475f582705f3ed6d107
sha256_prpriv_nowrap=9abf1793e605ae7adb64c438d65e597b63efddd56cee8cb3a0f8b6c0432cb222
sha256_as=6bbb35ae31b9397f640080e4f19121e81e4f28efd9ce5d9658c946091f16000b

function usage() {
	echo "factory test script v2.0 2017/01/23"
	echo "Usage: fac commands"
	echo "available commands: $commands"
}

function end_proc() {
	#ip addr del $DUT_IP/24 dev eth0
	#echo "7 4 1 7" > /proc/sys/kernel/printk
	#svc wifi disable
	#ifconfig eth0 down
	#ifconfig eth0 up
}

function ctrl_c() {
	#end_proc
	#exit
}

function kill_bluetooth_scan()
{
	killall com.droidlogic.tv.settings 2> /dev/null
	am force-stop com.fii.bluetooth
}

function gen_wpa_cfg() {
	echo "" > /data/misc/wifi/wpa_supplicant.conf
	echo "ctrl_interface=/data/misc/wifi/sockets" >>/data/misc/wifi/wpa_supplicant.conf
	echo "driver_param=use_p2p_group_interface=1" >>/data/misc/wifi/wpa_supplicant.conf
	echo "update_config=1" >>/data/misc/wifi/wpa_supplicant.conf
	echo "device_name=bd201" >>/data/misc/wifi/wpa_supplicant.conf
	echo "manufacturer=Amlogic" >>/data/misc/wifi/wpa_supplicant.conf
	echo "model_name=bd201" >>/data/misc/wifi/wpa_supplicant.conf
	echo "model_number=bd201" >>/data/misc/wifi/wpa_supplicant.conf
	echo "serial_number=0123456789abcdef" >>/data/misc/wifi/wpa_supplicant.conf
	echo "device_type=10-0050F204-5" >>/data/misc/wifi/wpa_supplicant.conf
	echo "config_methods=physical_display virtual_push_button" >>/data/misc/wifi/wpa_supplicant.conf
	echo "pmf=1" >>/data/misc/wifi/wpa_supplicant.conf
	echo "external_sim=1" >>/data/misc/wifi/wpa_supplicant.conf
	echo "wowlan_triggers=any" >>/data/misc/wifi/wpa_supplicant.conf
	echo "network={" >>/data/misc/wifi/wpa_supplicant.conf
	echo "        ssid=\"$1\"" >>/data/misc/wifi/wpa_supplicant.conf
	echo "        key_mgmt=NONE" >>/data/misc/wifi/wpa_supplicant.conf
	echo "}" >>/data/misc/wifi/wpa_supplicant.conf
}
function enable_wifi_and_wait() {
	echo "==> enable_wifi_and_wait ..."
	x=`dumpsys wifi -s | grep Wi-Fi | awk '{print $3}'`
	if [ x"$x" = xenabled ]; then
		echo "    wifi is already enabled"
		echo "    SUCCESS"
		return
	fi

	svc wifi enable

	cnt=$enable_wifi_timeout
	while [ $cnt -gt 0 ]; do
		x=`dumpsys wifi -s | grep Wi-Fi | awk '{print $3}'`
		if [ x"$x" = xenabled ]; then
			sleep 1
			echo "    wifi is enabled"
			echo "    SUCCESS"
			return
		else
			echo "    waiting...$cnt"
			sleep 1
		fi
		cnt=$((cnt-1))
	done

	echo "    FAIL: enable wifi failed ... exit"
	exit
}

function disable_wifi_and_wait() {
	echo "==> disable_wifi_and_wait ..."
	x=`dumpsys wifi -s | grep Wi-Fi | awk '{print $3}'`
	if [ x"$x" = xdisabled ]; then
		echo "    wifi is already disabled"
		echo "    SUCCESS"
		return
	fi

	svc wifi disable
	cnt=$enable_wifi_timeout
	while [ $cnt -gt 0 ]; do
		x=`dumpsys wifi -s | grep Wi-Fi | awk '{print $3}'`
		if [ x"$x" = xdisabled ]; then
			sleep 1
			echo "    wifi is disabled"
			echo "    SUCCESS"
			return
		else
			echo "    waiting...$cnt"
			sleep 1
		fi
		cnt=$((cnt-1))
	done
	echo "    FAIL: disable wifi failed ... exit"
	exit
}

# usage: wifi_test ssid ap_ip function
function wifi_test() {
	# disable_wifi_and_wait
	# gen_wpa_cfg $1
	svc bluetooth disable
	kill_bluetooth_scan
	enable_wifi_and_wait
	ping_before_tput=0
	if [ "x$1" = "x--with-ping" ]; then
		ping_before_tput=1
		shift
	fi
	echo "==> connecting to $1..."
	set_wifi.sh $1 --no-password wlan0
	if [ "x$3" = "xPING" -o $ping_before_tput -eq 1 ]; then
		echo "PING..."
		sleep 2
		ping -c $pingcount $2
	fi
	if [ "x$3" = "xTPT" ]; then
		[ -z $4 ] && tcp_win="" || tcp_win=" -w $4"
		wifi_tx_cmd="iperf3 $iperf3paramTX$tcp_win -c $2"
		wifi_rx_cmd="iperf3 $iperf3paramRX$tcp_win -c $2"

		echo "Throughput by iperf3, test type: TX"
		echo "Command: $wifi_tx_cmd"
		$wifi_tx_cmd | while read iperf3_output; do echo "Tx: $iperf3_output"; done

		echo "Throughput by iperf3, test type: RX"
		echo "Command: $wifi_rx_cmd"
		$wifi_rx_cmd | while read iperf3_output; do echo "Rx: $iperf3_output"; done
	fi
	if [ "x$3" = "xCONNECT" ]; then
		: # do nothing
	fi

	sleep 1
	echo "wifi_test end"
}
function usb_test() {
	echo "USB Storage Test:"
	if [ "x$ANDROID_VER" = "x6.0.1" ]; then
		var=`mount | grep /dev/block/vold/public:8 | awk '{print $2}'`
	else
		var=`mount | grep /dev/block/vold/public:8 | awk '{print $3}'`
	fi
	if [ "x$var" = "x" ];then
		echo FAIL
	else
		rm -r $var/usbtest
		echo 0123456789 >$var/usbtest
		var1=`cat $var/usbtest`
		if [ "x$var1" = "x0123456789" ];then
			echo SUCCESS
		else
			echo FAIL
		fi
	fi
}
function sdcard_test() {
	echo "SDCard Test:"
	var=`mount | grep /dev/block/vold/public:179 | awk '{print $2}'`
	if [ "x$var" = "x" ];then
		echo FAIL
	else
		rm -r $var/sdcardtest
		echo 0123456789 >$var/sdcardtest
		var1=`cat $var/sdcardtest`
		if [ "x$var1" = "x0123456789" ];then
			echo SUCCESS
		else
			echo FAIL
		fi
	fi
}

function aml_enable_rx() {
	echo clear GPIODV_23 > /sys/class/aml_gpio/gpio_test
	echo clear GPIODV_24 > /sys/class/aml_gpio/gpio_test
}
function aml_get_rx() {
	echo get GPIODV_23 > /sys/class/aml_gpio/gpio_test
	rx1=`cat /sys/class/aml_gpio/gpio_test`
	rx1=$((1-rx1))
	echo get GPIODV_24 > /sys/class/aml_gpio/gpio_test
	rx2=`cat /sys/class/aml_gpio/gpio_test`
	rx2=$((1-rx2))
	echo RX1=$rx1
	echo RX2=$rx2
}
function aml_key_waitandlock() {
	#echo "wait_and_lock"
	var=`cat /sys/class/unifykeys/lock`
	if [ "x$var" = "x0" ]; then
		echo 1 > /sys/class/unifykeys/lock
		return
	fi
	cnt=3
	while [ $cnt -gt 0 ]; do
		var=`cat /sys/class/unifykeys/lock`
		if [ "x$var" = "x0" ]; then
			echo 1 > /sys/class/unifykeys/lock
			return
		else
			echo "    waiting...$cnt"
			sleep 1
		fi
		cnt=$((cnt-1))
	done
	echo "  wait lock=0 timeout"
}
function aml_key_unlock() {
	echo 0 > /sys/class/unifykeys/lock
}
function aml_getmac() {
	echo mac > /sys/class/unifykeys/name
	var=`cat /sys/class/unifykeys/read`
	echo "$var"
}
function aml_getbtmac() {
	echo mac_bt > /sys/class/unifykeys/name
	var=`cat /sys/class/unifykeys/read`
	echo "$var"
}
function aml_getserial() {
	echo usid > /sys/class/unifykeys/name
	var=`cat /sys/class/unifykeys/read`
	echo "$var"
}
function aml_setmac() {
	aml_key_waitandlock
	echo mac > /sys/class/unifykeys/name
	echo $1 > /sys/class/unifykeys/write
	aml_key_unlock
	sync
}
function aml_sethdcp() {
	aml_key_waitandlock
	echo hdcp > /sys/class/unifykeys/name
	echo $1 | xxd -p -r > /sys/class/unifykeys/write
	aml_key_unlock
	sync
}
function aml_set_wvkey() {
	aml_key_waitandlock
	echo widevinekeybox > /sys/class/unifykeys/name
	echo $1 | xxd -p -r > /sys/class/unifykeys/write
	aml_key_unlock
	sync
}
function aml_get_deviceid() {
	echo deviceid > /sys/class/unifykeys/name
	var=`cat /sys/class/unifykeys/read`
	echo "$var"
}
function aml_set_deviceid() {
	aml_key_waitandlock
	echo deviceid > /sys/class/unifykeys/name
	echo $1 > /sys/class/unifykeys/write
	aml_key_unlock
	sync
}
function aml_set_serial() {
	aml_key_waitandlock
	echo usid > /sys/class/unifykeys/name
	echo $1 > /sys/class/unifykeys/write
	echo $1 > /sys/class/unifykeys/write
	aml_key_unlock
	sync
}
function aml_set_btmac() {
	aml_key_waitandlock
	echo mac_bt > /sys/class/unifykeys/name
	echo $1 > /sys/class/unifykeys/write
	aml_key_unlock
	sync
}
function aml_set_playready_pub_dd() {
	aml_key_waitandlock
	echo prpubkeybox > /sys/class/unifykeys/name
	size=`stat -c "%s" $1`
	busybox dd if=$1 of=/sys/class/unifykeys/write bs=$size count=1
	echo "write playready public key $1 $size" > /dev/console
	aml_key_unlock
	sync
}
function aml_set_playready_priv_dd() {
	aml_key_waitandlock
	echo prprivkeybox > /sys/class/unifykeys/name
	dd if=$1 of=/sys/class/unifykeys/write
	echo "write playready private key $1" > /dev/console
	aml_key_unlock
	sync
}
function aml_set_hdcp22_dd() {
	aml_key_waitandlock
	echo hdcp22_fw_private > /sys/class/unifykeys/name
	dd if=$1 of=/sys/class/unifykeys/write
	echo "write hdcp22 $1" > /dev/console
	aml_key_unlock
	sync
}
function aml_gethdcp22() {
	echo hdcp22_fw_private > /sys/class/unifykeys/name
	var=`hexdump -e '16/1 "%02x"' /sys/class/unifykeys/read`
	echo "$var"
}
function aml_gethdcp14() {
	echo hdcp > /sys/class/unifykeys/name
	var=`hexdump -e '16/1 "%02x"' /sys/class/unifykeys/read`
	echo "$var"
}
function aml_getwv() {
	echo widevinekeybox > /sys/class/unifykeys/name
	var=`hexdump -e '16/1 "%02x"' /sys/class/unifykeys/read`
	echo "$var"
}
function aml_getprpriv() {
	echo prprivkeybox > /sys/class/unifykeys/name
	var=`hexdump -e '16/1 "%02x"' /sys/class/unifykeys/read`
	echo "$var"
}
function aml_getprpub() {
	echo prpubkeybox > /sys/class/unifykeys/name
	var=`hexdump -e '16/1 "%02x"' /sys/class/unifykeys/read`
	echo "$var"
}
function aml_getas() {
	echo attestationkeybox > /sys/class/unifykeys/name
	var=`hexdump -e '16/1 "%02x"' /sys/class/unifykeys/read`
	echo "$var"
}
function aml_check_hdcp14() {
	echo hdcp > /sys/class/unifykeys/name
	var=`cat /sys/class/unifykeys/exist`
	echo "==> hdcp 1.4 key exist = $var"

	echo 1 > /sys/class/amhdmitx/amhdmitx0/hdcp_mode
	sleep 1
	var=`cat /sys/module/hdmitx20/parameters/hdmi_authenticated`
	echo "==> hdcp 1.4 authentication = $var"
}
function aml_check_hdcp22() {
	echo hdcp22_fw_private > /sys/class/unifykeys/name
	var=`cat /sys/class/unifykeys/exist`
	echo "==> hdcp 2.2 key exist = $var"

	echo 2 > /sys/class/amhdmitx/amhdmitx0/hdcp_mode
	sleep 1
	var2=`cat /sys/module/hdmitx20/parameters/hdmi_authenticated`
	echo "==> hdcp 2.2 authentication = $var2"
}

function key_write() {
	echo "Read keylist from data..."
	if [ -f /data/keylist ]; then
		cat /data/keylist | busybox tr "$encoded_chars" "$decoded_chars" > /data/keylist.dec
		keylistfile=/data/keylist.dec
	else
		echo "    FAIL: keylist not found"
		return
	fi
	deviceid=`grep $1 $keylistfile | awk '{print $2}'`
	hdcp14=`grep $1 $keylistfile | awk '{print $3}'`
	wvkey=`grep $1 $keylistfile | awk '{print $4}'`

	if [ x"$deviceid" = x ]; then
		echo "    FAIL: MAC/SN not found, invalid device"
	else
		echo "Write Device ID=$deviceid"
		aml_set_deviceid $deviceid

	#HDCP14 key
	echo "Write HDCP14..."
	sha_src=`echo $hdcp14 | xxd -p -r | sha256sum | awk '{print $1}'`
	echo "SRC HDCP14 sha256sum=$sha_src"
	aml_sethdcp $hdcp14
	sha_dst=$(aml_gethdcp14)
	if [ x$sha_src = x$sha_dst ]; then
		echo "DST HDCP14 sha256sum=$sha_dst, MATCH"
	else
		echo "DST HDCP14 sha256sum=$sha_dst, NOT MATCH"
		echo "    FAIL: HDCP14 read back not match"
		return
	fi

	#Widevine key
	echo "Write Widevine Key..."
	sha_src=`echo $wvkey | xxd -p -r | sha256sum | awk '{print $1}'`
	echo "SRC WVKEY sha256sum=$sha_src"
	aml_set_wvkey $wvkey
	sha_dst=$(aml_getwv)
	if [ x$sha_src = x$sha_dst ]; then
		echo "DST WVKEY sha256sum=$sha_dst, MATCH"
	else
		echo "DST WVKEY sha256sum=$sha_dst, NOT MATCH"
		echo "    FAIL: WVKEY read back not match"
		return
	fi
	fi
}

function switch_to_user_firmware()
{
	filename=$2
	file_md5=$3

	[ -z "$filename" ] && {
		echo "Switch to User Firmware: Error: Filename required!"
		exit 1
	}
	[ -z "$file_md5" ] && {
		echo "Switch to User Firmware: Error: File MD5 checksum required!"
		exit 1
	}
	[ ! -f "/cache/$filename" ] && {
		echo "Switch to User Firmware: Error: File not exist: /cache/$filename"
		exit 1
	}

	echo "Switch to User Firmware: calculate MD5 checksum..."
	md5=$(md5sum "/cache/$filename" | awk '{print $1}')
	[ "x$md5" = "x$file_md5" ] && {
		echo "Switch to User Firmware: MD5 checksum matched"
	} || {
		echo "Switch to User Firmware: Error: MD5 checksum mismatch: $md5 != $file_md5"
		exit 1
	}

	mkdir -p /cache/recovery
	echo "--update_package=/cache/$filename" > /cache/recovery/command
	echo "--wipe_data" >> /cache/recovery/command

	echo "Switch to User Firmware: scheduled!"
	sync
	sleep 2
	reboot recovery
}

function fxBluetooth_client_restart()
{
	# key BACK
	# key BACK
	# killall com.fii.bluetooth
	am force-stop com.fii.bluetooth
	sleep 1
	am start -n com.fii.bluetooth/.MainActivity --es bt-mode client --es target-name $target_name --es reverse-mode $reverse_mode
}

function fxBluetooth_server_restart()
{
	kill_bluetooth_scan
	am start -n com.fii.bluetooth/.MainActivity --es bt-mode server --es server-name $server_name
	sleep 2

	setprop fac.bt.server ""
	kill_bluetooth_scan
	am start -n com.fii.bluetooth/.MainActivity --es bt-mode server --es server-name $server_name

	wait_start=0
	while true; do
		started=$(getprop fac.bt.server)
		name=$(getprop fac.bt.server.name)
		[ -z "$started" ] && {
			[ $wait_start -gt 5 ] && {
				echo "Bluetooth: Server start failed, restart server..."
				return
			}
			echo "Bluetooth: waiting server starting... $wait_start"
			sleep 1
			wait_start=$((wait_start+1))
		} || {
			[ $name = $server_name ] && {
				echo "Bluetooth: Server started: name = $name"
				break
			} || {
				echo "Bluetooth: Server started: but name mismatch: $name != $server_name, restarting..."
				return
			}
		}
	done

	while true; do
		sleep 1;
		killall com.droidlogic.tv.settings 2> /dev/null
	done
}

#--> interactive mode
#while true; do
#    echo -n "Enter Command:"
#    read cmd
#    case "$cmd" in

#--> command mode
if [ $# = 0 ]; then
	usage
fi

case "$1" in
	set_mac)
		echo "Write MAC..."
		if [ $# = 2 ]; then
			if [ `echo $2 | egrep "^([0-9A-F]{2}:){5}[0-9A-F]{2}$"` ]; then
				aml_setmac $2
			else
				echo "    FAIL: invalid MAC format"
				return
			fi
		else
			echo "usage: fac set_mac XX:XX:XX:XX:XX:XX"
			return
		fi
		var=$(aml_getmac)
		if [ x"$var" = x"$2" ]; then
			echo "    SUCCESS"
		else
			echo "    FAIL: mac read back not match"
			return
		fi

		var=`getprop ro.product.name`
		if [ $var = "ba201" ] || [ $var = "LUKE" ]; then
			echo "product=$var, dont write keys"
			return
		fi
		: key_write $2
		;;
	get_amlmac)
		aml_getmac
		;;
	set_bt_mac)
		echo "Write BT MAC..."
		if [ $# = 2 ]; then
			if [ `echo $2 | egrep "^([0-9A-F]{2}:){5}[0-9A-F]{2}$"` ]; then
				aml_set_btmac $2
			else
				echo "    FAIL: invalid MAC format"
				return
			fi
		else
			echo "usage: fac set_bt_mac XX:XX:XX:XX:XX:XX"
			return
		fi
		var=$(aml_getbtmac)
		if [ x"$var" = x"$2" ]; then
			echo "    SUCCESS"
		else
			echo "    FAIL: mac read back not match"
			return
		fi
		;;
	set_deviceid)
		if [ $# = 2 ]; then
			echo "Writing... 1"
			aml_set_deviceid $2
			sleep 0.5
			echo "Writing... 2"
			aml_set_deviceid $2
			sleep 1
			echo "Writing... 3"
			aml_set_deviceid $2
		else
			echo "usage: fac set_deviceid id"
		fi
		;;
	get_deviceid)
		echo "Get DevicdID:"
		x=`getprop ro.boot.deviceid`
		if [ x"$x" = x ]; then
			echo "    FAIL"
		else
			echo "    $x"
		fi
		;;
	set_serial)
		if [ $# = 2 ]; then
			echo "Writing... 1"
			aml_set_serial $2
			sleep 0.5
			echo "Writing... 2"
			aml_set_serial $2
			sleep 1
			echo "Writing... 3"
			aml_set_serial $2
		else
			echo "usage: fac set_serial serial"
		fi
		var=$(aml_getserial)
		if [ x"$var" = x"$2" ]; then
			echo "    SUCCESS"
		else
			echo "    FAIL"
		fi

		var=`getprop ro.product.name`
		if [ $var = "ba201" ] || [ $var = "LUKE" ]; then
			: key_write $2
		fi
		;;
	get_serial)
		echo "Get SerialNo:"
		x=`getprop ro.serialno`
		if [ x"$x" = x ]; then
			echo "    FAIL"
		else
			echo "    $x"
		fi
		;;
	set_ctei_sn)
		if [ $# = 2 ]; then
			echo "Writing... 1"
			aml_set_deviceid $2
			sleep 0.5
			echo "Writing... 2"
			aml_set_deviceid $2
			sleep 1
			echo "Writing... 3"
			aml_set_deviceid $2
		else
			echo "usage: fac set_ctei_sn id"
		fi
		var=$(aml_get_deviceid)
		if [ x"$var" = x"$2" ]; then
			echo "    SUCCESS"
		else
			echo "    FAIL"
		fi
		;;
	get_ctei_sn)
		echo "Get CTEI SN:"
		x=`getprop ro.boot.deviceid`
		if [ x"$x" = x ]; then
			echo "    FAIL"
		else
			echo "    $x"
		fi
		;;
	get_hdcp14_sha256)
		var=$(aml_gethdcp14)
		echo $var
		;;
	get_wvkey_sha256)
		var=$(aml_getwv)
		echo $var
		;;
	set_playready_pub)
		if [ $# = 2 ]; then
			echo "fac set_playready_pub_dd $2"
			aml_set_playready_pub_dd $2
		else
			echo "usage: fac set_playready_pub pub_key_path"
		fi
		;;
	set_playready_priv)
		if [ $# = 2 ]; then
			echo "fac set_playready_priv_dd $2"
			aml_set_playready_priv_dd $2
		else
			echo "usage: fac set_playready_priv priv_key_path"
		fi
		;;
	set_hdcp22)
		if [ $# = 2 ]; then
			echo "fac set_hdcp22_dd $2"
			aml_set_hdcp22_dd $2
		else
			echo "usage: fac set_hdcp22 key_path"
		fi
		;;
	set_as_key)
		if [ $# = 2 ]; then
			echo "fac set_as_key_dd $2"
			writeAttestaionkey $2
		else
			echo "usage: fac set_askey key_path"
		fi
		;;
	set_burnin)
		if [ $# = 2 ]; then
			prefs="/data/data/com.foxconn.fxvideo/shared_prefs/prefs.xml"
			if [ $2 = on ]; then
				setprop vendor.led.state none
				killall com.foxconn.fxvideo
				getenv
				time=$((BURNIN_TIME*60*1000))
				echo "Set Burn In Mode, time=$time"
				echo "<?xml version='1.0' encoding='utf-8' standalone='yes' ?>" > $prefs
				echo "<map>" >> $prefs
				echo "    <string name=\"autostart\">ON</string>" >> $prefs
				echo "    <string name=\"history0\">/system/media/4k_test.mp4</string>" >> $prefs
				echo "    <string name=\"history1\">/data/HIN1.mp4</string>" >> $prefs
				echo "    <string name=\"panel_mode\">ON</string>" >> $prefs
				echo "    <string name=\"dbgmsg\">ON</string>" >> $prefs
				echo "    <string name=\"burn_in_time\">$time</string>" >> $prefs
				echo "</map>" >> $prefs
				chmod 777 $prefs
				rm /data/data/com.foxconn.fxvideo/burn_in_start 2> /dev/null
				rm /data/data/com.foxconn.fxvideo/burn_in_finish 2> /dev/null
				sync
				sleep 1
				echo "SUCCESS"
			fi
			if [ $2 = off ]; then
				input keyevent HOME
				killall com.foxconn.fxvideo
				rm $prefs
				sync
				sleep 1
				echo "SUCCESS"
			fi
		else
			echo "usage: fac set_burnin {on|off}"
		fi
		;;
	check_burnin)
		if [ -f /data/data/com.foxconn.fxvideo/burn_in_start ]; then
			echo "Burn In Started? Yes"
		else
			echo "Burn In Started? No"
		fi
		if [ -f /data/data/com.foxconn.fxvideo/burn_in_finish ]; then
			echo "Burn In Finshed? Yes"
		else
			echo "Burn In Finshed? No"
		fi
		x=`grep last_elapsed_time /data/data/com.foxconn.fxvideo/shared_prefs/prefs.xml 2> /dev/null | awk -F'[<>]' '{print $3}'`
		if [ x"$x" = x ]; then
			echo "Burn In Time not exist"
		else
			min=$((x/1000/60))
			echo "Burn In Time= $min mins"
		fi
		;;
	set_prod)
		flagfile=/data/misc/need_update.flag
		touch $flagfile
		echo "copy fw.zip from USB..."

		if [ "x$ANDROID_VER" = "x6.0.1" ]; then
			var=`mount | grep /dev/block/vold/public:8 | awk '{print $2}'`
		else
			var=`mount | grep /dev/block/vold/public:8 | awk '{print $3}'`
		fi

		if [ "x$var" = "x" ];then
			echo "FAIL, USB disk not found"
			rm $flagfile
			return
		else
			if [ -f $var/fw.zip ]; then
				time cp $var/fw.zip /cache/
				echo "check src md5..."
				if [ -f $var/fw.md5 ]; then
					echo "fw.md5 found"
					md5_src=`cat $var/fw.md5`
				else
					echo "fw.md5 not found, caculating..."
					md5_src=`time busybox md5sum -b $var/fw.zip | awk '{print $1}'`
				fi
				echo "md5_src=$md5_src"
				echo "check dst md5..."
				md5_dst=`time busybox md5sum -b /cache/fw.zip | awk '{print $1}'`
				echo "md5_dst=$md5_dst"
				if [ ! $md5_src = $md5_dst ];then
					echo "FAIL, md5 not match"
					rm $flagfile
					return
				fi
			else
				echo "FAIL, fw.zip not found"
				rm $flagfile
				return
			fi
		fi
		chmod 777 /cache/fw.zip
		sync
		sleep 1
		echo "SUCCESS"
		;;
	set_prod974)
		flagfile=/data/misc/need_update.flag
		touch $flagfile
		echo "checking fw.zip in cache..."

		var=/cache
		if [ -f $var/fw.zip ]; then
			echo "check src md5..."
			if [ -f $var/fw.md5 ]; then
				echo "fw.md5 found"
				md5_src=`cat $var/fw.md5`
			fi
			echo "md5_src=$md5_src"
			echo "check dst md5..."
			md5_dst=`time busybox md5sum -b /cache/fw.zip | awk '{print $1}'`
			echo "md5_dst=$md5_dst"
			if [ ! $md5_src = $md5_dst ];then
				echo "FAIL, md5 not match"
				rm $flagfile
				return
			fi
		else
			echo "FAIL, fw.zip not found"
			rm $flagfile
			return
		fi

		chmod 777 /cache/fw.zip
		sync
		sleep 1
		echo "SUCCESS"
		;;
	check_hdcp14)
		aml_check_hdcp14
		;;
	check_hdcp22)
		aml_check_hdcp22
		;;
	check_hdcp22_key)
		var=$(aml_gethdcp22)
		if [ "x$var" = "x$sha256_hdcp22" ]; then
			echo "SUCCESS"
		else
			echo "FAIL"
		fi
		;;
	hdcp)
		var=`cat /sys/module/hdmitx20/parameters/hdmi_authenticated`
		echo "currently auth=$var"
		var=`cat /sys/class/amhdmitx/amhdmitx0/hdcp_mode`
		echo "TX hdcp mode=$var"
		var=`cat /sys/class/amhdmitx/amhdmitx0/hdcp_ver`
		echo "TV hdcp support=\n$var"
		;;
	prepare)
		user=$(id -u)
		[ $user != 0 ] && {
			echo "Error: factory command needs 'root' user, please do 'adb root' firstly"
			return
		}
		mkdir /dev/factory_test 2> /dev/null
		setprop ctl.stop button_ageing
		echo "Prepare setup: ok"
		;;
	switch_to_user_firmware)
		switch_to_user_firmware "$@"
		;;
	get_env)
		getenv
		;;
	set_env)
		if [ $# = 6 ]; then
			saveenv $2 $3 $4 $5 $6
			getenv
		else
			echo "usage: fac set_env AP_2G_IP AP_5G_IP AP_2G_ssid AP_5G_ssid BurnInTime(min)"
		fi
		;;
	get_version)
		#getprop ro.build.display.id
		version=$(getprop ro.fxn.production.version)
		echo Production Firmware Version: $version
		[ "x$2" = "xall" ] && {
			tmp=$(dmesg | grep "\[dhd\]" | grep Driver: | sed 's/.*Driver:[[:space:]]*//g;s/[[:space:]].*//g' | uniq)
			wifi_driver_ver=${tmp:-N/A}
			tmp=$(dmesg | grep "\[dhd\]" | grep Firmware: | sed 's/.*Firmware:.*version[[:space:]]*//g;s/[[:space:]].*//g' | uniq)
			wifi_fw_ver=${tmp:-N/A}
			tmp=$(dmesg | grep "\[dhd\] NVRAM version" | awk -F: '{print $2}' | sed 's/[[:space:]]//g' | uniq)
			wifi_nvram_ver=${tmp:-N/A}
			echo "Wifi Driver Version:   $wifi_driver_ver"
			echo "Wifi Firmware Version: $wifi_fw_ver"
			echo "Wifi NVRAM Version:    $wifi_nvram_ver"
		}
		;;
	get_wired_mac)
		var=`cat /sys/class/net/eth0/address`
		echo $var
		#            dumpsys ethernet | grep "MAC address"
		;;
	get_wifi_mac)
		enable_wifi_and_wait
		cnt=3
		while [ $cnt -gt 0 ]; do
			if [ -f /sys/class/net/wlan0/address ]; then
				echo "    address ready"
				break;
			else
				echo "    waiting wl address ...$cnt"
				sleep 1
			fi
			cnt=$((cnt-1))
		done
		#var=`ifconfig -a | grep wlan0 | awk '{print tolower($5)}'`
		var=`cat /sys/class/net/wlan0/address`
		echo address=$var
		;;
	get_wifi_mac_user)
		var=`getprop vasott.wifi.mac`
		echo address=$var
		;;
	get_bt_mac)
		echo "==> enabling BT..."
		service call bluetooth_manager 6 > /dev/null
		cnt=20
		while [ $cnt -gt 0 ]; do
		    if [ "x$ANDROID_VER" = "x6.0.1" ]; then
		        x=`cat /data/misc/bluedroid/bt_config.conf | grep AddressBlacklist`
		    else
		        x=`cat /data/misc/bluedroid/bt_config.conf | grep "Address ="`
		    fi
		    if [ x"$x" = x ]; then
		        echo "    waiting...$cnt"
		        sleep 1
		    else
		        echo "    enabled"
		        var=`cat /data/misc/bluedroid/bt_config.conf | grep "Address =" | awk '{print $3}'`
		        echo BT_MAC_ADDRESS=$var
		        break
		    fi
		    cnt=$((cnt-1))
		done
		if [ $cnt = 0 ]; then
		    echo "    FAIL: enable BT timeout"
		fi
		service call bluetooth_manager 8 > /dev/null
		;;
	get_bt_mac_user)
		var=`getprop vasott.bt.mac`
		echo BT_MAC_ADDRESS=$var
		;;
	get_wifi_scan)
		getenv
		enable_wifi_and_wait
		cnt=10
		echo "==> start scan"
		while [ $cnt -gt 0 ]; do
			x=$($WPA_CLI scan)
			if [ x"$x" = xOK ]; then
				sleep 1
				echo "    scan OK!"
				break
			else
				echo "    scanning..."
				sleep 1
			fi
			cnt=$((cnt-1))
		done
		var=$($WPA_CLI scan_result | grep -w $AP2G_SSID | awk '{print $3}')
		echo "$AP2G_SSID RSSI=$var"
		var=$($WPA_CLI scan_result | grep -w $AP5G_SSID | awk '{print $3}')
		echo "$AP5G_SSID RSSI=$var"
		;;
	get_wifi_rssi)
		getenv
		enable_wifi_and_wait
		cnt=10
		while [ $cnt -gt 0 ]; do
			x=$($WPA_CLI scan)
			if [ x"$x" = xOK ]; then
				sleep 1
				break
			else
				sleep 1
			fi
			cnt=$((cnt-1))
		done
		var=$($WPA_CLI scan_result | grep -w $AP2G_SSID | awk '{print $3}')
		echo "$AP2G_SSID RSSI=$var"
		var=$($WPA_CLI scan_result | grep -w $AP5G_SSID | awk '{print $3}')
		echo "$AP5G_SSID RSSI=$var"
		;;
	test_wifi_2g)
		getenv
		wifi_test $AP2G_SSID $AP2G_IP TPT | tee $WIFI_2G_LOG
		;;
	test_wifi_5g)
		getenv
		wifi_test $AP5G_SSID $AP5G_IP TPT | tee $WIFI_5G_LOG
		;;
	test_wifi_2g_with_ping)
		getenv
		wifi_test --with-ping $AP2G_SSID $AP2G_IP TPT | tee $WIFI_2G_LOG
		;;
	test_wifi_5g_with_ping)
		getenv
		wifi_test --with-ping $AP5G_SSID $AP5G_IP TPT | tee $WIFI_5G_LOG
		;;
	test_wifi_2g_connect)
		getenv
		wifi_test $AP2G_SSID $AP2G_IP CONNECT | tee $WIFI_2G_LOG
		;;
	test_wifi_5g_connect)
		getenv
		wifi_test $AP5G_SSID $AP5G_IP CONNECT | tee $WIFI_5G_LOG
		;;
	test_ethernet_giga)
		iperf3 -c $2 -t 10
		;;
	get_hw_current)
		timeout=$((5*2))
		times=0
		max=0
		sum=0
		if [ "x$2" != "x" ]; then
			timeout=$2
			timeout=$((timeout*2)) 2> /dev/null
			if [ $? != 0 ]; then
				echo "Error: get_hw_current need a decimal parameter, for example: 5"
				exit 1
			fi
		fi
		while [ $times -lt $timeout ]; do
			cur=$(hw_get_current.sh | grep Current | awk '{print $2}')
			sum=$((sum+cur))
			if [ $max -lt $cur ]; then
				max=$cur
			fi
			sleep 0.5
			times=$((times+1))
		done
		if [ $times -gt 0 ]; then
			average=$((sum/times))
		else
			average="n/a"
		fi
		echo "Hardware highest current: $max mA"
		echo "Hardware average current: $average mA"
		;;
	test_ddr)
		size=$((1024*1024*256))
		if [ "x$2" != "x" ]; then
			size=$2
			size=$((size*1024*1024)) 2> /dev/null
			if [ $? != 0 ]; then
				echo "Error: DDR test size should be a decimal parameter, for example: 256"
				exit 1
			fi
		fi
		echo "DDR test..."
		factory_test_bin file-test --tag "DDR test" --file /dev/ddr_test.tmp --size $size
		;;
	test_flash)
		size=$((1024*1024*100))
		if [ "x$2" != "x" ]; then
			size=$2
			size=$((size*1024*1024)) 2> /dev/null
			if [ $? != 0 ]; then
				echo "Error: flash test size should be a decimal parameter, for example: 100"
				exit 1
			fi
		fi
		echo "Flash test..."
		factory_test_bin file-test --tag "Flash test" --file /cache/emmc_test.tmp --size $size
		;;
	bt_throughput_server)
		# Prepare to keep the ADB mode
		mkdir /dev/factory_test 2> /dev/null
		svc bluetooth enable

		server_name="T99H316-BT"
		if [ "x$2" != "x" ]; then
			server_name="$2"
		fi
		while true; do
			fxBluetooth_server_restart
		done
		;;
	bt_throughput_client)
		svc bluetooth enable
		target_name="T99H316-BT"
		reverse_mode=0
		tx_rx=Tx
		if [ "x$2" = "x-R" ]; then
			reverse_mode=1
			tx_rx=Rx
			shift
		fi
		if [ "x$2" != "x" ]; then
			target_name="$2"
		fi

		fxBluetooth_client_restart
		setprop fac.bt.speed ""
		wait_connect=0
		data_connected=0
		bt_test_sec=0
		bt_timeout=10              # The timeout of Bluetooth throughput test
		while true; do
			killall com.droidlogic.tv.settings 2> /dev/null

			spd=$(getprop fac.bt.speed)
			bps=$(getprop fac.bt.bps)
			avr_bps=$(getprop fac.bt.avr_bps)
			rssi=$(getprop fac.bt.rssi)
			[ -z "$spd" ] && {
				[ $wait_connect -gt 10 ] && {
					echo "Bluetooth: connect failed, restart client..."
					wait_connect=0
					fxBluetooth_client_restart
				}
				data_connected=0
				echo "Bluetooth: waiting connect... $wait_connect"
				wait_connect=$((wait_connect+1))
			} || {
				[ $spd = "disconnected" -a $data_connected -eq 1 ] && {
					echo "Bluetooth: disconnected, exit."
					break;
				}
				data_connected=1
				echo "Bluetooth: $tx_rx: $bt_test_sec-$((bt_test_sec+1)) speed: $spd Bytes/s, $((bps/1024)) Kbps, Average: $((avr_bps/1024)) Kbps, rssi $rssi"
				bt_test_sec=$((bt_test_sec+1))
				[ $bt_test_sec -ge bt_timeout ] && {
					kill_bluetooth_scan
					break;
				}
			}
			sleep 1
		done
		;;
	button)
		case $2 in
			up)
				factory_test_bin button --up-only
				;;
			down)
				factory_test_bin button --down-only
				;;
			both|up-down|--up-down)
				factory_test_bin button --up-down
				;;
			*)
				factory_test_bin button --up-down
				;;
		esac
		;;
	set_led_on)
		led_path=/sys/class/leds/system_state
		echo none > $led_path/trigger
		echo 255 > $led_path/brightness
		echo "Led changed to: ON"
		;;
	set_led_off)
		led_path=/sys/class/leds/system_state
		echo none > $led_path/trigger
		echo 0 > $led_path/brightness
		echo "Led changed to: OFF"
		;;
	set_led_blink)
		led_path=/sys/class/leds/system_state
		echo timer > $led_path/trigger
		echo 100 > $led_path/delay_on
		echo 200 > $led_path/delay_off
		echo "Led changed to: Blink"
		;;
	check_usb)
		usb_test
		;;
	check_sdcard)
		sdcard_test
		;;
	get_hdmi_res)
		hdmi_res=$(cat /sys/class/display/mode)
		echo "HDMI resolution: $hdmi_res"
		;;
	get_hdmitx_edid)
		cat /sys/class/amhdmitx/amhdmitx0/rawedid | md5sum -b
		;;
	play_video)
		[ -z $2 ] && {
			video_path=/system/media/4k_test.mp4
		} || {
			video_path="$2"
		}
		am start -n  com.android.gallery3d/.app.MovieActivity -d "$video_path"
		;;
	play_audio)
		am start -n  com.android.gallery3d/.app.MovieActivity -d "$2"
		;;
	stop_video)
		input keyevent 4
		;;
	stop_audio)
		input keyevent 4
		;;
	enable_rx)
		aml_enable_rx
		aml_get_rx
		;;
	get_rx_hpd)
		aml_get_rx
		;;
	check_channelkey)
		dmesg -c > /dev/null
		cnt=10
		while [ $cnt -gt 0 ]
		do
			if [ $# = 2 ]; then
				if [ $2 = up ]; then
					var=`dmesg | grep -r -e "key 402 up"`
					if [ x"$var" = x ]; then
						echo "==> press CHANNELUP key in $cnt seconds..."
						sleep 1
					else
						echo "    SUCCESS"
						return
					fi
				fi
				if [ $2 = down ]; then
					var=`dmesg | grep -r -e "key 403 up"`
					if [ x"$var" = x ]; then
						echo "==> press CHANNELDOWN key in $cnt seconds..."
						sleep 1
					else
						echo "    SUCCESS"
						return
					fi
				fi
			else
				echo "usage: fac check_channelkey {up|down}"
				echo "    FAIL"
				break
			fi
			cnt=$((cnt-1))
		done
		echo "    FAIL"
		;;
	check_remote)
		echo 1 > /sys/devices/virtual/remote/amremote/debug_enable
		dmesg -c > /dev/null
		cnt=10
		while [ $cnt -gt 0 ]
		do
			var=`dmesg | grep -r -e "receive scancode=0x49" -e "remote: press ircode = 0x06" -e "remote: press ircode = 0xca" -e "meson-remote c8100580.rc: keyup!!"`
			if [ x"$var" = x ]; then
				echo "==> press UP key in $cnt seconds..."
				sleep 1
			else
				echo "    GOT UP KEY"
				echo "    SUCCESS"
				break
			fi
			cnt=$((cnt-1))
		done
		if [ $cnt = 0 ]; then
			echo "    FAIL"
		fi
		;;
	check_remote_ext)
		echo 1 > /sys/devices/virtual/remote/amremote/debug_enable
		dmesg -c > /dev/null
		cnt=10
		while [ $cnt -gt 0 ]
		do
			var=`dmesg | grep -r -e "remote: press ircode = 0x4e" -e "receive scancode=0x4e"`
			if [ x"$var" = x ]; then
				echo "==> press Fixture Button in $cnt seconds..."
				sleep 1
			else
				echo "    GOT VOL+ KEY from IREXT"
				echo "    SUCCESS"
				break
			fi
			cnt=$((cnt-1))
		done
		if [ $cnt = 0 ]; then
			echo "    FAIL"
		fi
		;;
	test_btrcu)
		killall com.vasott.factest
		out=/data/data/com.vasott.factest/rcu.txt
		service call bluetooth_manager 6 > /dev/null
		rm $out 2> /dev/null
		if [ "x$2" = "x" ]; then
			am start -n com.vasott.factest/.RCUKeyTest
		else
			am start -n com.vasott.factest/.RCUKeyTest --ei onekeyexit $2
		fi
		cnt=10
		while [ $cnt -gt 0 ]; do
			if [ -f $out ]; then
				break;
			else
				echo "    waiting rcu.txt ...$cnt"
				sleep 1
			fi
			cnt=$((cnt-1))
		done
		if [ $cnt = 0 ]; then
			echo "    FAIL: wait rcu.txt timeout"
			return
		fi
		while [ -f $out ]; do
			size=`stat -c "%s" $out` 2> /dev/null
			if [ ! $size = 0 ]; then
				cat $out
				echo -n "" > $out
			fi
			usleep 100
		done
		usleep 500
		am broadcast -a vasott.rcutest.finish
		service call bluetooth_manager 8 > /dev/null
		;;
	test_btrcu2)
		killall com.vasott.factest
		out=/data/data/com.vasott.factest/rcu.txt
		rm $out 2> /dev/null
		if [ "x$2" = "x" ]; then
			am start -n com.vasott.factest/.RCUKeyTest_All
		else
			am start -n com.vasott.factest/.RCUKeyTest_All --ei onekeyexit $2 --es rcuname $3
		fi
		cnt=10
		while [ $cnt -gt 0 ]; do
			if [ -f $out ]; then
				break;
			else
				echo "    waiting rcu.txt ...$cnt"
				sleep 1
			fi
			cnt=$((cnt-1))
		done
		if [ $cnt = 0 ]; then
			echo "    FAIL: wait rcu.txt timeout"
			return
		fi
		while [ -f $out ]; do
			size=`stat -c "%s" $out` 2> /dev/null
			if [ ! $size = 0 ]; then
				cat $out
				echo -n "" > $out
			fi
			usleep 100
		done
		usleep 500
		;;
	help)
		usage
		;;
	bye)
		break;
		;;
	fxv)
		am start com.foxconn.fxvideo/com.foxconn.fxvideo.FxVideoActivity
		;;
	setting)
		am start -n com.android.settings/.Settings
		;;
	xmp)
		rmmod remote
		rmmod remote_xmp
		sleep 1
		setprop persist.vasott.remote xmp
		sync
		;;
	nec)
		rmmod remote
		rmmod remote_xmp
		sleep 1
		setprop persist.vasott.remote nec
		sync
		;;
	kmsg)
		if [ $# = 2 ]; then
			if [ $2 = on ]; then
				echo "msg on"
				echo "7 4 1 7" > /proc/sys/kernel/printk
			fi
			if [ $2 = off ]; then
				echo "msg off"
				echo "0 0 0 0" > /proc/sys/kernel/printk
				return
			fi
		else
			echo "usage: fac kmsg {on|off}"
		fi
		;;
	stop_hdcp14)
		echo stop14 > /sys/class/amhdmitx/amhdmitx0/hdcp_ctrl
		usleep 500000
		echo SUCCESS
		;;
	nand)
		if [ $# = 2 ]; then
			if [ $2 = read ]; then
				echo "nand read testing ..."
				while [ true ]; do
					busybox dd if=/dev/block/cache of=/dev/null 2>&1 | grep -v records
				done
			fi
			if [ $2 = write ]; then
				echo "nand write testing ..."
				while [ true ]; do
					busybox dd if=/dev/zero of=/data/tmpfile bs=128M count=1 2>&1 | grep -v records
				done
			fi
		else
			echo "usage: fac nand {read|write}"
		fi
		;;
	check_bt)
		out=/data/data/com.vasott.factest/scan.txt
		service call bluetooth_manager 6 > /dev/null
		rm $out
		sleep 2
		am start -n com.vasott.factest/.ListBT
		cnt=10
		while [ $cnt -gt 0 ]; do
			if [ -f $out ]; then
				echo "    scan.txt ready"
				break;
			else
				echo "    waiting scan.txt ...$cnt"
				sleep 1
			fi
			cnt=$((cnt-1))
		done
		if [ $cnt = 0 ]; then
			echo "    FAIL: wait scan.txt timeout"
			return
		fi
		timeout 10 busybox tail -f $out | grep -v null
		input keyevent 4
		service call bluetooth_manager 8 > /dev/null
		;;
	check_prpriv)
		var=$(aml_getprpriv)
		if [ "x$var" = "x$sha256_prpriv" ]; then
			setprop persist.playready.private 1
			echo "SUCCESS"
		else
			setprop persist.playready.private 0
			echo "FAIL"
		fi
		;;
	check_prpub)
		var=$(aml_getprpub)
		if [ "x$var" = "x$sha256_prppub" ]; then
			setprop persist.playready.public 1
			echo "SUCCESS"
		else
			setprop persist.playready.public 0
			echo "FAIL"
		fi
		;;
	check_as_key)
		var=$(aml_getas)
		if [ "x$var" = "x$sha256_as" ]; then
			echo "SUCCESS"
		else
			echo "FAIL"
		fi
		;;
	stress)
		echo "stress test started..."
		if [ $# = 2 ]; then
			stress --cpu 4 --io 4 --vm 2 --vm-bytes 256M --timeout $2
		else
			stress --cpu 4 --io 4 --vm 2 --vm-bytes 256M --timeout 10s
		fi
		echo "stress test end"
		;;
	set_stress)
		if [ $# = 2 ]; then
			rm -f /data/misc/stress_start
			rm -f /data/misc/stress_finish
			sync
			if [ $2 = off ]; then
				echo "clear stress"
				setprop persist.vasott.stress off
			else
			echo "set stress, time=$2"
				setprop persist.vasott.stress $2
				sync
			fi
		else
			echo "usage: fac set_stress {time: eg 20s | off }"
		fi
		;;
	check_stress)
		if [ -f /data/misc/stress_start ]; then
			if [ -f /data/misc/stress_finish ]; then
				cat /data/misc/stress_finish
			else
				cat /data/misc/stress_start
				var=`ps | grep stress`
				if [ "x$var" = "x" ]; then
					echo "Stress: FAILED"
				else
					echo "Stress: INPROGRESS, check later"
				fi
			fi
		else
			var=`getprop persist.vasott.stress`
			if [ ! "x$var" = "x" ] && [ ! "x$var" = "xoff" ]; then
				echo "Stress: START ON NEXT BOOT, time=$var"
			else
				echo "Stress: NOT STARTED"
			fi

		fi
		;;
	get_antenna_rssi)
		ant0_rssi=$(wl -i wlan0 phy_rssi_ant | awk '{print $2}')
		ant1_rssi=$(wl -i wlan0 phy_rssi_ant | awk '{print $4}')

		echo "ANT0=$ant0_rssi"
		echo "ANT1=$ant1_rssi"
		;;
	'')
		;;
	*)  echo "unknown command, type 'help' for commands"
		;;
esac
#done

end_proc
