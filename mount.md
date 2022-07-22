## 不同类型文件系统的制作
1. 计算出所需制件的文件的大小size
   ```sh
   du -s rootfs | cut -f1
   ```
2. 划出对应大小的img文件
   ```SH
   dd if=/dev/zero of=rootfs.img bs=1024 count=size
   ```
3. 格式化此img文件为你所需要的格式，如ext4
   ```sh
   #不同类型使用不同的命令，mkfs.cramfs,mkfs.jffs2,mkfs.yaffs
   mkfs.ext4 rootfs.img
   ```
4. mount到某个目录如root
   ```sh
   mount -o loop rootfs.img root/
   ```
5. 拷贝文件到此目录
   ```sh
    cp -av rootfs/* root/
    # or
    rsync -HPavz -q rootfs/ root/
   ```
6. umount, 压缩,自动生成rootfs.img.gz
   ```sh
   umount root
   gzip -v9 rootfs.img
   ```
7. 格式化为成对应boot程序识别的格式
   ```sh
    # uboot
    mkimage -n "ramdisk" -A arm -O linux -T ramdisk -C gzip -d rootfs.img.gz ramdisk.img

    # isolinux
    mkisofs
   ```

## fs.img 格式  
1. 一般使用 `mount -o loop fs.img tmp` 即可，如果不行尝试如下方式  
2. fdisk查看并计算偏移，mount的时候加上偏移，原始参考：http://my.oschina.net/toyandong/blog/65002

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

