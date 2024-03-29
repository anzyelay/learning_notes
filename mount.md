# MOUNT

- [不同类型文件系统的制作](#不同类型文件系统的制作)
  - [规划出目标大小的空白文件系统](#规划出目标大小的空白文件系统)
  - [分区 (不分区则可跳过)](#分区-不分区则可跳过)
  - [格式化成对应的文件系统](#格式化成对应的文件系统)
  - [填充文件系统的内容](#填充文件系统的内容)
  - [压缩为对应boot识别的文件](#压缩为对应boot识别的文件)
  - [汇总脚本](#汇总脚本)
- [fs.img 格式](#fsimg-格式)

## 不同类型文件系统的制作

### 规划出目标大小的空白文件系统

1. 计算出所需制件的文件的大小size

   ```sh
   # 假设rootfs下的文件就是要填充的内容，计算出大小
   du -s rootfs | cut -f1
   ```

1. 划出对应大小的空白img文件

   ```sh
   dd if=/dev/zero of=rootfs.img bs=1024 count=size
   ```

### 分区 (不分区则可跳过)

1. 使用`parted`命令行指定分区, 格式为`parted imgfile cmd`

   ```sh
   parted "rootfs.img" --script -- mklabel msdos
   parted "rootfs.img" --script -- mkpart primary fat32 0 256
   parted "rootfs.img" --script -- mkpart primary ext4 256 -1
   ```

1. 设置安装对应的回环设备

   ```sh
   ann@dell:mynodes$ sudo losetup -f --show  test.img 
   /dev/loop14
   # 以下为断开设备，后面操作完需要断开
   # losetup -d /dev/loop14
   ```

1. 创建设备的映射分区,在目录/dev/mapper下

   ```sh
   ann@dell:mynodes$ sudo kpartx -av /dev/loop14 
   add map loop14p1 (253:0): 0 97656 linear 7:14 1
   add map loop14p2 (253:1): 0 899072 linear 7:14 98304
   ann@dell:mynodes$ ls /dev/mapper/
   control  loop14p1  loop14p2
   # 删除对应的映射, 后面操作完需要断开
   # kpartx -dv /dev/loop14
   ```

1. 汇总脚本

   ```sh
   parted "${imagename}.img" --script -- mklabel msdos
   parted "${imagename}.img" --script -- mkpart primary fat32 0 256
   parted "${imagename}.img" --script -- mkpart primary ext4 256 -1

   # Set the partition variables
   loopdevice=$(losetup -f --show "${basedir}/${imagename}.img")
   device=$(kpartx -va "$loopdevice" | sed -E 's/.*(loop[0-9]+)p.*/\1/g' | head -1)
   device="/dev/mapper/${device}"
   bootp=${device}p1
   rootp=${device}p2
   ```

### 格式化成对应的文件系统

- 未分区，则直接针对file.img文件进行格式化操作:  
  1. 格式化此img文件为你所需要的格式，如ext4

   ```sh
   #如果未分区，不同类型文件系统直接使用不同的命令，mkfs.cramfs,mkfs.jffs2,mkfs.yaffs生成
   mkfs.ext4 rootfs.img

   # 也可以直接格式化的同时填充文件,省去了后面的填充步骤
   # mkfs.ext4 -d rootfs rootfs.img
   ```

- 有分区的文件系统，则需要针对分区设备进行格式化操作：  
   1. 格式化成对应文件系统,设备由分区步骤中获得

      ```sh
      ann@dell:mynodes$ sudo mkfs.vfat -n system-boot /dev/mapper/loop14p1

      ann@dell:mynodes$ sudo mkfs.ext4 -L writable /dev/mapper/loop14p2 
      ```

   1. 查看设备分区详情：`fdisk -l /dev/loop14`

### 填充文件系统的内容  

1. mount到某个目录如root

   ```sh
   mount -o loop rootfs.img root/
   #有分区设备的如下
   mount -t vfat /dev/mapper/loop14p1 /tmp/boot/
   mount /dev/mapper/loop14p2 /tmp/root
   ```

1. 拷贝文件到此目录

   ```sh
    cp -av rootfs/* root/
    # or
    rsync -HPavz -q rootfs/ root/
   ```

1. umount, 压缩,自动生成rootfs.img.gz

   ```sh
   umount root
   gzip -v9 rootfs.img
   ```

1. 如果是有[分区](#分区-不分区则可跳过)步骤，则断开对应设备,否则跳过

   ```sh
   kpartx -dv loopdevice
   losetup -d loopdeivce
   ```

### 压缩为对应boot识别的文件

1. 格式化为成对应boot程序识别的格式

   ```sh
   # uboot
   mkimage -n "ramdisk" -A arm -O linux -T ramdisk -C gzip -d rootfs.img.gz ramdisk.img

   # isolinux
   mkisofs
   ```

### 汇总脚本

- 不用分区的脚本参考[mk-image.sh](./script/mk-image.sh);
- 分区脚本

```sh
rootfsdir=$1
# Free space on rootfs in MiB
free_space="500"
# Calculate the space to create the image.
root_size=$(du -s -B1K ${rootfsdir} | cut -f1)
raw_size=$(($((free_space*1024))+root_size))

# Create the disk and partition it
echo "Creating image file"
dd if=/dev/zero of="${basedir}/${imagename}.img" bs=1024 count=${raw_size}

parted "${imagename}.img" --script -- mklabel msdos
parted "${imagename}.img" --script -- mkpart primary fat32 0 256
parted "${imagename}.img" --script -- mkpart primary ext4 256 -1

# Set the partition variables
loopdevice=$(losetup -f --show "${basedir}/${imagename}.img")
device=$(kpartx -va "$loopdevice" | sed -E 's/.*(loop[0-9])p.*/\1/g' | head -1)
device="/dev/mapper/${device}"
bootp=${device}p1
rootp=${device}p2

# Create file systems
mkfs.vfat -n system-boot "$bootp"
mkfs.ext4 -L writable "$rootp"

# Create the dirs for the partitions and mount them
mkdir -p "${basedir}/bootp" "${basedir}/root"
mount -t vfat "$bootp" "${basedir}/bootp"
mount "$rootp" "${basedir}/root"

mkdir -p ${rootfsdir}/boot/firmware
mount -o bind "${basedir}/bootp/" ${rootfsdir}/boot/firmware

# Copy Raspberry Pi specific files
cp -r "${rootdir}"/rpi/rootfs/system-boot/* elementary-${architecture}/boot/firmware/
echo "Rsyncing rootfs into image file"
rsync -HPavz -q "${basedir}/${rootfsdir}/" "${basedir}/root/"

# Unmount partitions
umount "$bootp"
umount "$rootp"
kpartx -dv "$loopdevice"
losetup -d "$loopdevice"

echo "Compressing ${imagename}.img"
xz -T0 -z "${basedir}/${imagename}.img"
```

## fs.img 格式  

1. 一般使用 `mount -o loop fs.img tmp` 即可，如果不行尝试如下方式  
2. fdisk查看并计算偏移，mount的时候加上偏移，原始参考：<http://my.oschina.net/toyandong/blog/65002>

    命令是这样的：`sudo mount -o loop,offset=$((34816 * 512)) ubuntu-desktop-12.04-1-miniand.com.img /mnt`

    其中的34816和512参考下面的。

    我的操作过程，重点是#注释部分。

    查看信息：

    ```sh
    $ fdisk -l ubuntu-desktop-12.04-1-miniand.com.img

    Disk ubuntu-desktop-12.04-1-miniand.com.img: 4023 MB, 4023386112 bytes

    255 heads, 63 sectors/track, 489 cylinders, total 7858176 sectors
    # 512
    Units = sectors of 1 * 512 = 512 bytes

    Sector size (logical/physical): 512 bytes / 512 bytes

    I/O size (minimum/optimal): 512 bytes / 512 bytes

    Disk identifier: 0x8daf8ee2

    

                                    Device Boot      Start         End      Blocks   Id  System

    ubuntu-desktop-12.04-1-miniand.com.img1            2048       34815       16384   83  Linux
                                                        #34816
    ubuntu-desktop-12.04-1-miniand.com.img2           34816     7858175     3911680   83  Linux
    ```

3. fs.img未格式化,使用`file fs.img`显示是data,无法mount

-----------

# Overlay FS

## 参考

1. [Introduction to the OverlayFS](https://linuxconfig.org/introduction-to-the-overlayfs)

## 详解

overlay fs分为三层：

- merge layer: 混合层， 由lower+upper构成，供使用者实际读写的层，在此层的增删改操作最终“copy up”到可写的upper层去

- upper layer: 覆盖层，可读写, 由upper和work组成
  - 当与lower有相同文件或目录则覆盖使用此层的文件和目录。
  - 混合层增加或删除文件、目录时，会在此层增加或删除对应文件、目录。
  - 但是，**如果删除的是属于lower layer的文件或目录，则会使用一个在此层创建的witheout file或（有trusted.overlay.opaque属性的）目录来模拟执行**，**witheout file**是一个字符设备文件。

- lower layer: 该层的文件为只读，混合层的修改不会改变此层数据，当前层的数据可被upper层存在的同名数据遮挡（或叫掩盖）
  
## 装配介绍

```sh
# step 1
$ sudo mkdir /lower /overlay

$ sudo mount -o ro /dev/sda1 /lower
$ sudo mount /dev/sda2 /overlay

# step 2 
$ sudo mkdir /overlay/{upper,work}

# step 3
$ sudo mount overlay -t overlay -o lowerdir=/lower,upperdir=/overlay/upper,workdir=/overlay/work  /media

# 
$ mount | grep -i overlay
overlay on /media type overlay (rw,relatime,seclabel,lowerdir=/lower,upperdir=/overlay/upper,workdir=/overlay/work)

```

如上：  

- 第一步：将两个分区sda1, sda2分别mount到lower和overlay,并且lower为只读模式，overlay为可读写模式。  
- 第二步：在overlay上准备好创建upper层目录和work目录, 前者用于掌管upper层的文件; 后者用于在交换文件到其它层时，先行在内部准备好要交换的文件,work必须是个空目录，并与upper层目录同处一个文件系统上
- 第三步： 装配并挂载overlay，执行上述脚本中的"step 3"命令,
  - -t： 指定mount的文件系统 --“overlay"
  - -o : 列出mount选项：
    - “lowerdir”： 只读文件系统的目录
    - "upperdir"： 掌管上层文件的可写层目录
    - "workdir"：  “work”的目录
  - /media : 装配的混合层的挂载点
