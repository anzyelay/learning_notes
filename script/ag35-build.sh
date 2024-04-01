#!/bin/bash
fun=$1

ex_help()
{
	echo "the funs list as bellow:"
	sed -rn 's/^ex_(.*)\(\)$/\t\1/p' $0
}

ex_all()
{
	make prepare/clean
	make prepare
	source ql-ol-crosstool/ql-ol-crosstool-env-init && make
}

ex_rootfs()
{
	make prepare
	source ql-ol-crosstool/ql-ol-crosstool-env-init && make rootfs
}

ex_flash()
{
	adb.exe reboot bootloader
	sleep 1
	target=${1:-'target'}
	fastboot.exe flash aboot ${target}/appsboot.mbn || exit 0
	fastboot.exe flash boot ${target}/mdm9607-boot.img || exit 0
	fastboot.exe flash system ${target}/mdm9607-sysfs.ubi
	fastboot.exe reboot

}

main(){

	has_fun=$(grep "^ex_$fun()$" $0)
	if [ 0 -ne $? ];then
		ex_help
		exit 1
	fi

	call_fun=${has_fun%"()"}

	shift 1
	$call_fun $@
}

main $@

exit 0
