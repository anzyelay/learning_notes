# TI AM62LP

## dfu-util烧写

1. `dfu-util.exe -l`

   ```sh
    C:\Users\F1339899\AppData\Local\Temp\_MEI55042>dfu-util.exe -l
    dfu-util 0.9
    Found DFU: [0451:6165] ver=0200, devnum=28, cfg=1, intf=0, path="1-8.1", alt=1, name="SocId", serial="01.00.00.00"
    Found DFU: [0451:6165] ver=0200, devnum=28, cfg=1, intf=0, path="1-8.1", alt=0, name="bootloader", serial="01.00.00.00"
   ```

   - **name**: 列出可烧写区，后面用于烧写时`-a`指定
   - **path**: 后面用于烧写时`-p`指定
   - **alt**: 值表示后面要写入区域的顺序

2. how to flash by dfu-util

    ```bat

    # first
    dfu-util.exe -a bootloader -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\tiboot3.bin
    dfu-util.exe -a tispl.bin -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\tispl.bin -R
    dfu-util.exe -a u-boot.img -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\u-boot.img -R
    dfu-util.exe -a uEnv.txt -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\uEnv.txt -R

    # list the dict name with used by -a bellow
    C:\Users\F1339899\AppData\Local\Temp\_MEI55042>dfu-util.exe -l
    dfu-util 0.9

    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=7, name="rootfs.3", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=6, name="u-boot.2", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=5, name="tispl.1", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=4, name="tiboot3.0", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=3, name="rootfs", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=2, name="u-boot", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=1, name="tispl", serial="0000000000000000"
    Found DFU: [0451:6165] ver=0223, devnum=15, cfg=1, intf=0, path="1-8.1", alt=0, name="tiboot3", serial="0000000000000000"

    # second
    dfu-util.exe -p "1-8.1" -a tiboot3 -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\new\tiboot3-am62x-hs-evm.bin
    dfu-util.exe -p "1-8.1" -a tispl -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\FVT-5G-TBOX.R032\tispl.bin
    dfu-util.exe -p "1-8.1" -a u-boot -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\FVT-5G-TBOX.R032\u-boot.img
    dfu-util.exe -p "1-8.1" -a rootfs -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\FVT-5G-TBOX.R032\rootfs.img -R

    ```

## image create

```sh
# args1: append file.tar to make rootfs
ex_create_image()
{
    local need_remove=0
    if [ $# -eq 0 ] ;then
            TARGET=/working_dir/build/arago-tmp-default-glibc/work/am62xx_lp_evm-oe-linux/tisdk-base-image/1.0/rootfs
    else
            TARGET=$1
    fi
    if [ ! -d $TARGET ];then
            # 根文件目录地址
            TARGET_ROOTFS_DIR=./tmprootfs
            [ -d $TARGET_ROOTFS_DIR ] || {
                    mkdir $TARGET_ROOTFS_DIR
            }
            tar xf $TARGET -C $TARGET_ROOTFS_DIR || {
                    rm -rf $TARGET_ROOTFS_DIR
                    return
            }
            need_remove=1
    else
            TARGET_ROOTFS_DIR=$TARGET
    fi

    # 镜像名
    ROOTFSIMAGE=rootfs.img
    EXTRA_SIZE_MB=100
    IMAGE_SIZE_MB=$(( $(du -sh -m ${TARGET_ROOTFS_DIR} | cut -f1) + ${EXTRA_SIZE_MB} ))

    echo Making rootfs!

    if [ -e ${ROOTFSIMAGE} ]; then
            rm ${ROOTFSIMAGE}
    fi

    dd if=/dev/zero of=${ROOTFSIMAGE} bs=1M count=0 seek=${IMAGE_SIZE_MB}

    fakeroot mkfs.ext4 -d ${TARGET_ROOTFS_DIR} ${ROOTFSIMAGE}
    [ $need_remove -eq 1 ] &&  rm -rf $TARGET_ROOTFS_DIR

    echo Rootfs Image: ${ROOTFSIMAGE}
}
```

## GPIO

### AM62的GPIO组成

- MAIN DOMAIN
  - GPIO0: 0~91 (eg: GPIO0_0, GPIO0_9)
  - GPIO1: 0~51 (eg: GPIO1_0, GPIO1_51)
- MCU DOMAIN
  - MCU_GPIO0: 0~23

### userspace write/read

- tools

  tool         |   function
  -----------  |  -------------------------------------------------------------------------------------
  gpiodetect   |   List GPIO chips, print their labels and number of GPIO lines.
  gpioset      |   gpioset -c chip line=value
  gpioget      |   gpioget -c chip line
  gpioinfo     |   Lines are specified by name, or optionally by offset if the chip option is provided.

- 通过`gpiodetect`查看每个gpiochip的lines数:

    ```sh
    root@fvt-5g-tbox:~# gpiodetect
    gpiochip0 [tps65219-gpio] (3 lines)
    gpiochip1 [1-0023] (24 lines)
    gpiochip2 [600000.gpio] (92 lines)
    gpiochip3 [601000.gpio] (52 lines)
    gpiochip4 [1-0022] (24 lines)
    ```

    根据[AM62的GPIO组成](#am62的gpio组成)可知，GPIO0_X组有92个GPIO，所以是gpiochip2, 同理GPIO1_X为gpiochip3

- 配置输出：`devmem2 gpio_address 0x00010007`

- 拉高: `gpioset -c chip line=1`

## uboot cmdline传参

在/boot/uEnv.txt中加入`setenv optargs "initcall_debug ignore_loglevel"`
