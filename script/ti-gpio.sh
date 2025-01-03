#!/bin/bash
# This tool is used to get/set the gpio according to gpio chip and pin offset

GPIOCHIP0_BASE_PATH="/sys/devices/platform/bus@f0000/600000.gpio/gpio/gpiochip*/base"
GPIOCHIP1_BASE_PATH="/sys/devices/platform/bus@f0000/601000.gpio/gpio/gpiochip*/base"
GPIOCHIP_IOEX_NAME="tca6408"
GPIOCHIP_PMIC_NAME="tps65219"

myself=$(basename $0)
subcmd=$1
gpiochip=$2
gpio_pin_offset=$3
gpio_dir=$4
gpio_value=$5
gpiochip_base=""

function usage()
{
	[ ! -z "$1" ] && {
		echo "$myself: $1"
		echo ""
	}
	echo "$myself <subcmd> [gpiochip] [pin offset] [direction] [value]"
	echo "  gpiochip: 0, 1, io-expander, pmic"
	echo "  pin offset: pin offset in the gpiochip"
	echo "  direction: in, out"
	echo "  value: required for 'out', 0 or 1"
	echo "  subcmd: getnumber, export, unexport, direction, getvalue, setvalue, check-gpiochip"
	echo ""
	exit 0
}

case $gpiochip in
	0)
		gpiochip_base=$(cat $GPIOCHIP0_BASE_PATH)
		;;
	1)
		gpiochip_base=$(cat $GPIOCHIP1_BASE_PATH)
		;;
	io-ex*|ioex*|io_ex*)
		gpiodev=$(fgrep -s $GPIOCHIP_IOEX_NAME /sys/class/gpio/gpiochip*/device/name | head -1)
		if [ ! -z "$gpiodev" ]; then
			gpiopath=$(echo $gpiodev | awk -F '/' '{print $5}')
			gpiopath="/sys/class/gpio/$gpiopath/base"
			gpiochip_base=$(cat $gpiopath)
		fi
		;;
	pmic)
		gpiodev=$(fgrep -s $GPIOCHIP_PMIC_NAME /sys/class/gpio/gpiochip*/device/name | head -1)
		if [ ! -z "$gpiodev" ]; then
			gpiopath=$(echo $gpiodev | awk -F '/' '{print $5}')
			gpiopath="/sys/class/gpio/$gpiopath/base"
			gpiochip_base=$(cat $gpiopath)
		fi
		;;
	*)
		gpiochip_base=""
		[ -z "$gpiochip" ] && usage "No gpiochip specified." || usage "Invalid gpiochip: $gpiochip"
		;;
esac

if [ -z $gpiochip_base ]; then
	echo "GPIO: Err: GPIOCHIP-$gpiochip: not exist, please check!"
	exit 1
fi

function get_gpio_number()
{
	[ -z "$gpio_pin_offset" ] && {
		echo "GPIO: Err: pin offset is empty, please check!"
		return
	}
	echo "$(($gpiochip_base + $gpio_pin_offset))"
}

function get_gpio_value()
{
	cat "/sys/class/gpio/gpio$(get_gpio_number)/value"
}

function set_gpio_value()
{
	[ -z "$gpio_value" ] && {
		echo "GPIO: Err: pin value is empty, please check!"
		return
	}
	echo "$gpio_value" > "/sys/class/gpio/gpio$(get_gpio_number)/value"
}

function gpio_dir_path()
{
	echo "/sys/class/gpio/gpio$(get_gpio_number)/direction"
}

function do_gpio_export()
{
	if [ ! -e $(gpio_dir_path) ]; then
		echo $(get_gpio_number) > /sys/class/gpio/export
	fi

	if [ ! -e $(gpio_dir_path) ]; then
		echo "GPIO: Err: GPIO-$(get_gpio_number) [$gpiochip: $gpio_pin_offset] can not be exported, please check!"
		exit 1
	fi
}

function do_gpio_unexport()
{
	if [ ! -e $(gpio_dir_path) ]; then
		return
	fi

	echo $(get_gpio_number) > /sys/class/gpio/unexport
	if [ -e $(gpio_dir_path) ]; then
		echo "GPIO: Err: GPIO-$(get_gpio_number) [$gpiochip: $gpio_pin_offset] can not be unexported, please check!"
		exit 1
	fi
}

function do_gpio_direction()
{
	[ -z "$gpio_dir" ] && {
		echo "GPIO: Err: gpio direction is empty, please check!"
		return
	}
	echo $gpio_dir > $(gpio_dir_path)
}

case $subcmd in
	getnumber)
		get_gpio_number
		;;
	export)
		do_gpio_export
		;;
	unexport)
		do_gpio_unexport
		;;
	direction)
		do_gpio_direction
		;;
	getvalue)
		do_gpio_export
		do_gpio_direction
		get_gpio_value
		;;
	setvalue)
		do_gpio_export
		do_gpio_direction
		set_gpio_value
		;;
	check-gpiochip)
		# Here is only an ok message, the error message is in earlier check..
		# If error happens, we won't get here.
		echo "GPIO: OK: GPIOCHIP-$gpiochip: detected!"
		;;
	*)
		usage "Invalid sub command: $subcmd"
		;;
esac
