# 创建 Debian 最小系统 LiveCD

参考：<https://blog.csdn.net/chiwa2270/article/details/100608775>

## 准备工作  

安装必须的软件包  

```sh
apt-get install live-build
apt-get  install syslinux cpio squashfs-tools mkisofs
```

## 准备工作目录

```sh
mkdir ~/live-build
cd ~/live-build
lb config
```

## 修改配置文件

- ~/live-build/config/chroot【跳过】

  ```sh
  LB_LINUX_FLAVOURS="generic"
  LB_LINUX_PACKAGES="linux-image-3.13.0-11"
  LB_PACKAGE_LISTS="standard"
  ```

- ~/live-build/config/common【跳过】

  ```sh
  LB_INITSYSTEM="systemd"
  ```

- ~/live-build/config/bootstrap【换成国内的源】

  ```sh
    LB_ARCHITECTURES="amd64"                        
    LB_DISTRIBUTION="trusty"                         
    LB_PARENT_DISTRIBUTION="trusty"                  
    LB_PARENT_DEBIAN_INSTALLER_DISTRIBUTION="trusty" 
    LB_PARENT_MIRROR_BOOTSTRAP="http://mirrors.ustc.edu.cn/ubuntu/"        
    LB_PARENT_MIRROR_CHROOT="http://mirrors.ustc.edu.cn/ubuntu/"           
    LB_PARENT_MIRROR_CHROOT_VOLATILE="http://mirrors.ustc.edu.cn/ubuntu/"  
    LB_PARENT_MIRROR_BINARY="http://mirrors.ustc.edu.cn/ubuntu/"           
    LB_PARENT_MIRROR_BINARY_SECURITY="http://security.debian.org/"
    LB_PARENT_MIRROR_BINARY_VOLATILE="http://mirrors.ustc.edu.cn/ubuntu/"  
    LB_PARENT_MIRROR_DEBIAN_INSTALLER="http://mirrors.ustc.edu.cn/ubuntu/" 
    LB_MIRROR_BOOTSTRAP="http://mirrors.ustc.edu.cn/ubuntu/"               
    LB_MIRROR_CHROOT="http://mirrors.ustc.edu.cn/ubuntu/"                  
    LB_ARCHIVE_AREAS="main restricted universe multiverse"                 
    LB_PARENT_ARCHIVE_AREAS="main restricted universe multiverse"
  ```

  配置文件无误后，在 ~/live-build/ 目录内执行

  ```sh
  lb build
  ```

  一个核心系统的的基本系统的 livecd 就开始构建了，后续你你可以基于这个 livecd 完成更多的个性化定制。

## 基于livecd(ISO)的个性化定制

### 1. 挂载ISO，复制出需要的文件

```sh
#mkdir ~/ISOBUILD
#mount -o loop ubuntu-mini-remix-12.04-amd64.iso /media
#cp -av /media/*     ~/ISOBUILD
#cp -av /media/.disk ~/ISOBUILD
 
# #如果你使用的是ubuntu官方标准的livecd 或 Mini-LiveCD为例，则需要如下处理，如果没有UUID的限制，下面的步骤可以跳过！
#rm -f ~/ISOBUILD/.disk/casper-uuid-generic
 
#umount /media
```

### 2. intrd.lz 的解压重打包

如果你使用的是ubuntu官方标准的livecd 或 Mini-LiveCD为例，则需要如下处理，如果没有UUID的限制，下面的步骤可以跳过！

```sh
#mkdir ~/INTRD && cd ~/INTRD
#cp  ~/ISOBUILD/casper/initrd.lz ~/initrd.lzma  
#lzma -dkf ~/initrd.lzma
#cpio -idv < ~/initrd
#rm conf/uuid.conf
#find | cpio -H newc -o | lzma > ～/initrd-new.lz
#cp ~/initrd-new.lz ~/ISOBUILD/casper/initrd.lz
```

or

```sh
/sbin/live-new-uuid ~/ISOBUILD/casper/initrd.lz
```

### 3. squashfs的重新封装

```sh
#cd ~/
#unsquashfs ~/ISOBUILD/casper/filesystem.squashfs
#mount --bind /dev ~/squashfs-root/dev
#mount -t proc proc ~/squashfs-root/proc
#mount -t sysfs sys ~/squashfs-root/sys
#cp /etc/resolv.conf ~/squashfs-root/etc/
#cp /etc/apt/source.list ~/squashfs-root/etc/apt/
#chroot squashfs-root
 
$ #各种自定义开始
$ apt-get install xxx --no-install-suggests
...
$ #定制结束
$ exit

#mksquashfs squashfs-root ~/ISOBUILD/casper/filesystem-new.squashfs
#mv ~/ISOBUILD/casper/filesystem.squashfs /~
#mv ~/ISOBUILD/casper/filesystem-new.squashfs ~/ISOBUILD/casper/filesystem.squashfs
```

### 4. 重新生成 filesystem.manifest

```sh
#chroot squashfs-root dpkg-query -W --showformat='${Package} ${Version}\n' > ～/filesystem.manifest
# cp -v ～/filesystem.manifest ~/ISOBUILD/casper/filesystem.manifest
```

### 5. 更加完善这个LIVECD：生成各个文件的md5值

```sh
#cd ~/ISOBUILD/ & find . -type f -print0 | xargs -0 md5sum > md5sum.txt
```

### 6. 生成最终的ISO

```sh
#mkisofs -R -J -l -V 'ubuntu-12.04-live' -cache-inodes -b isolinux/isolinux.bin -c isolinux/boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table -o ~/ubuntu-12.04-live.iso ~/ISOBUILD
```

### 关键点

```sh
~/ISOBUILD/.disk/casper-uuid-generic
conf/uuid.conf
```

这两处一定要删除，不然启动的时候 initrd检测的UUID信息和这里定义的不一致，导致filesystem.squashfs挂载失败，肯定启动不了，这是很多网上的文章都没提到，害得我好苦，但是有一点疑惑 我还不知道这个UUID到底是谁的UUID，我没搞懂，欢迎大神指点迷津！

## hybird格式化ISO

很多LIVECD都支持这格式。直接dd就可以把一个设备变成一个可启动的livecd了，dd命令是字节拷贝，相当于磁盘对拷，不同于普通的文件复制。

- 第一步 使用命令处理ISO

  ```sh
  # isohybrid your.iso
  ```

- 用dd命令装iso到硬盘上,sdx为U盘设备

  ```sh
  # dd if=your.iso of=/dev/sdx bs=10M
  ```

## 其他

- 利用 Grub2 启动引导 LiveCD

  ```txt
    menuentry “ubuntu_iso” {
        loopback loop (hd0,1)/natty-desktop-i386.iso
        linux (loop)/casper/vmlinuz boot=casper iso-scan/filename=/home/riku/natty-desktop-i386.iso locale=zh_CN.UTF-8 noprompt noeject
        initrd (loop)/casper/initrd.lz
    }
  ```
