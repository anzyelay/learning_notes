#!/bin/bash -e

# 根文件目录地址
# 注意rootfs文件的所属用户和组为非root可能导致文件系统挂载失败
TARGET_ROOTFS_DIR=./rootfs
# 镜像名
ROOTFSIMAGE=rootfs.img
EXTRA_SIZE_MB=300
IMAGE_SIZE_MB=$(( $(sudo du -sh -m ${TARGET_ROOTFS_DIR} | cut -f1) + ${EXTRA_SIZE_MB} ))


echo Making rootfs!

if [ -e ${ROOTFSIMAGE} ]; then
	rm ${ROOTFSIMAGE}
fi

# sudo ./post-build.sh ${TARGET_ROOTFS_DIR}

dd if=/dev/zero of=${ROOTFSIMAGE} bs=1M count=0 seek=${IMAGE_SIZE_MB}

#sudo mkfs.ext4 -d ${TARGET_ROOTFS_DIR} ${ROOTFSIMAGE}
# fakeroot 能保证image中的文件所有者为root
fakeroot mkfs.ext4 -d ${TARGET_ROOTFS_DIR} ${ROOTFSIMAGE}

echo Rootfs Image: ${ROOTFSIMAGE}
