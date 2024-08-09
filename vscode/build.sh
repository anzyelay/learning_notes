#!/bin/bash

path=$PWD
project=$(basename ${PWD})
mountdir=/working_dir/ql-ol-extsdk/example/foxconn/${project}

# build in a docker
docker run --rm --privileged -it --net=host \
        -v ${path}:${mountdir} \
        ag35:3.0 \
        bash -c "source ql-ol-crosstool/ql-ol-crosstool-env-init && cd ${mountdir} &&  make -f Makefile debug"

# forward 
adb.exe forward tcp:9090 tcp:9090

# push the app to remote target
adb.exe shell "killall ${project}"
adb.exe push $project /tmp
adb.exe shell "chmod +x /tmp/${project}"

# invoke gdbserver to wait a request of connecting from local gdb
adb.exe shell "gdbserver :9090 /tmp/GBT32960"

exit 0
