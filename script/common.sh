#!/bin/bash
# set -x
fun=$1

ex_help()
{
	echo "the funs list as bellow:"
	sed -rn 's/^ex_(.*)\(\)$/\t\1/p' $0
}

#Precentage function
ex_untar_progress()
{
    TARBALL=$1;
    DIRECTPATH=$2;
    BLOCKING_FACTOR=$(($(xz --robot --list ${TARBALL} | grep 'totals' | awk '{print $5}') / 51200 + 1));
    tar --blocking-factor=${BLOCKING_FACTOR} --checkpoint=1 --checkpoint-action='ttyout=Written %u%  \r' -Jxf ${TARBALL} -C ${DIRECTPATH}
}

cp_progress()
{
    CURRENTSIZE=0
    TOTALSIZE=$1;
    while [ $CURRENTSIZE -lt $TOTALSIZE ]
    do
        TOHERE=$2;
        CURRENTSIZE=`sudo du -c $TOHERE | grep total | awk {'print $1'}`
        echo -e -n "$CURRENTSIZE /  $TOTALSIZE copied \r"
        sleep 1
    done
}

ex_cp()
{
    LANG=C
    SRC=$1
    DST=$2
    TOTALSIZE=`sudo du -c ${SRC} | grep total | awk {'print $1'}`
    cp -rf ${SRC} ${DST} &
    cp_progress $TOTALSIZE ${DST}
    sync
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
