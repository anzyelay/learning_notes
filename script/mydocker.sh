#!/bin/bash

target=$1
[ -z "$target" ] && {
	echo "Err: Need a target..."
	exit
}

target_image="9x07"
image_ver="2.1"
mount_dir="-v /home/ann/share/T99W444:/home/docker/T99W444"
target="mdm9x07_kaga_$target"

#-e USER_ID=$(id -u) -e USER_NAME=$(id -un) -e GROUP_ID=$(id -g) -e GROUP_NAME=$(id -gn) \

container_ret=$(docker ps -a | grep "$target")

if [ -z "$container_ret" ]; then
	docker run --init -it --net=host \
		-e USER_ID=0 -e USER_NAME=root -e GROUP_ID=0 -e GROUP_NAME=root \
		-e HOME_PATH=$HOME \
		-v $HOME:$HOME -v /etc/localtime:/etc/localtime $mount_dir \
		--name "$target" "$target_image$([ -z $image_ver ] || echo :$image_ver)" /bin/bash
else
	echo "Container $target exist"
	ret=$(docker ps -a | grep "$target" | grep Up)
	if [ -z "$ret" ]; then
		echo "Container $target is Off, trying to start it..."
		docker start "$target"
	else
		echo "Container $target is On"
	fi
	echo "Tryint to connect container $target"
	docker exec -it -u $(whoami) "$target" /bin/bash
fi
