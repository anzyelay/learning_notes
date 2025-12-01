# TI AM62LP

## dfu-util烧写

### 烧写实战

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

    # first flash the tiboot3(r5) + tispl+u-boot.img (A53) to RAM to enable the device running for downloading later
    dfu-util.exe -a bootloader -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\tiboot3.bin
    dfu-util.exe -a tispl.bin -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\tispl.bin -R
    dfu-util.exe -a u-boot.img -D C:\Users\F1339899\AppData\Local\Temp\_MEI55042\u-boot.img -R
    # must exe cmd in uboot to complete, set dfu_alt_info environment and do 'dfu 0 ram <dev>', the reference as u-boot-2024.04/doc/usage/dfu.rst
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

    # second download the FW to the device memory permanently
    dfu-util.exe -p "1-8.1" -a tiboot3 -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\new\tiboot3-am62x-hs-evm.bin
    dfu-util.exe -p "1-8.1" -a tispl -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\FVT-5G-TBOX.R032\tispl.bin
    dfu-util.exe -p "1-8.1" -a u-boot -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\FVT-5G-TBOX.R032\u-boot.img
    dfu-util.exe -p "1-8.1" -a rootfs -D E:\image_bin\5GTBOX\FVT-5G-TBOX.R032\FVT-5G-TBOX.R032\rootfs.img -R

    ```

### dfu烧录方法说明

1. 编译支持**DFU**的**U-Boot**
1. 在UBOOT命令行或启动脚本中配置 DFU Alternate Settings
   1. 在环境变量中定义不同分区：

        ```sh
        setenv dfu_alt_info \
                'boot part 0 1;rootfs part 0 2;kernel fat 0 1'
        ```

        表示 DFU 支持烧录 boot 分区、rootfs 分区和 kernel 文件系统

   1. 主机端工具会显示这些 alt settings，用户选择对应分区进行烧录

1. 进入 DFU 模式(**被动等待主机操作模式**)
   1. 在 U-Boot 命令行执行：

        ```sh
        dfu 0 mmc 0
        ```

        表示通过 USB DFU 烧录到 mmc0（即 eMMC/SD）。

   2. 其他示例：

        ```sh
        dfu 0 nand 0 → 烧录 NAND
        dfu 0 ram 0 → 烧录到 RAM
        dfu 0 mmc 1 → 烧录到另一张 SD 卡
        ```

        - 进入 DFU gadget：U-Boot 会根据当前的 dfu_alt_info 环境变量，把可烧录的分区/区域暴露给主机。

        - 等待主机操作：设备本身不会主动传输数据，而是保持在 DFU 状态，等待 PC 端工具（如 dfu-util）发起下载或上传请求。

        - 数据交互：
                主机端执行 dfu-util -a \<alt> -D \<image> → 设备接收数据并写入目标存储。
                主机端执行 dfu-util -a \<alt> -U \<file> → 设备把数据读出并传回 PC。

        - 退出 DFU：完成传输后，通常需要手动退出 DFU 或复位设备，才能继续正常启动。

1. 主机端操作
   1. 安装 dfu-util 工具。
   2. 烧录命令示例：

        ```sh
        dfu-util -a boot -D u-boot.img
        dfu-util -a kernel -D zImage
        dfu-util -a rootfs -D rootfs.ext4
        ```

        其中 -a 指定 DFU alt setting，-D 指定镜像文件。

将上述步骤了聚合起来，无需在uboot中手动执行，可将boot的默认执行参数`CONFIG_BOOTCOMMAND`更改为如下参数:

```config
CONFIG_BOOTCOMMAND="
  setenv dfu_alt_info_flashenv uEnv.txt ram 0x82000000 0x10000000;
  setenv dfu_alt_info ${dfu_alt_info_flashenv};
  dfu 0 ram 0;
  env import -t ${loadaddr} $filesize;
  run user_commands;
"
```

解释：

1. 设置 DFU alt 信息

    - 定义一个 DFU alternate setting，名字是 uEnv.txt，目标存储介质是 RAM。
    - 地址范围：0x82000000 起始，大小 0x10000000（256MB）。
    - 这意味着主机端可以通过 DFU 将一个文件（如 uEnv.txt）下载到 RAM 指定区域。

1. 应用 DFU alt 信息, 将刚才定义的 alt 信息赋值给 dfu_alt_info，供 DFU 命令使用
1. 进入 DFU 模式, 启动 DFU gadget，编号为 0，目标是 RAM, 此时主机端可以用 dfu-util 下载文件到 RAM。
1. 导入环境变量,从 RAM 中加载刚才下载的 `uEnv.txt` 文件，并导入为 U-Boot 环境变量, `-t`表示文本格式导入
1. 执行 `uEnv.txt` 中定义的脚本或命令, 命令最终执行`dfu 0 mmc 0`来烧录固件到EMMC中。

```uEnv.txt
user_commands=mmc dev 0;mmc rst-function 0 1;setenv partitions "name=ab_msg,start=0,size=128MiB;name=mfg,size=128MiB;name=rootfs_a,size=512MiB;name=rootfs_b,size=512MiB;name=recovery,size=512MiB;name=data,size=512MiB;name=cache,size=-;";if test  $? -eq 0;then mmc erase 0 10000;gpt write mmc 0 $partitions;if test $? -ne 0;then exit;fi;fi;mmc part;mmc partconf 0;mmc partconf 0 1 1 1;mmc bootbus 0 2 0 0;mmc rescan;mmc part;mmc dev 0 0;setenv dfu_alt_info "tiboot3 raw 0x0 0x400 mmcpart 1;tispl raw 0x400 0x1000 mmcpart 1;u-boot raw 0x1400 0xc00 mmcpart 1;rootfs raw 0x80022 0x100000 mmcpart 0;tiboot3.0 raw 0x0 0x400 mmcpart 2;tispl.1 raw 0x400 0x1000 mmcpart 2;u-boot.2 raw 0x1400 0xc00 mmcpart 2;rootfs.3 raw 0x180022 0x100000 mmcpart 0";dfu 0 mmc 0;mw 0x04518170 0x0000006f 0x4;
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

## dd flash TI emmc

uboot加载地址：am62x_evm_a53_defconfig

```defconfig
CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR=0x1400
```

cmd:

```sh
echo 0 > /sys/devices/platform/bus@f0000/fa10000.mmc/mmc_host/mmc0/mmc0:0001/block/mmcblk0/mmcblk0boot0/force_ro
dd if=/tmp/u-boot.img of=/dev/mmcblk0boot0 seek=5120
dd if=/tmp/tispl.img of=/dev/mmcblk0boot0 seek=1024
dd if=/tmp/tiboot3 of=/dev/mmcblk0boot0 seek=0
```

## ti device tree struct

```yaml
- k3-am62-lp-sk.dts
    - k3-am62x-sk-common.dtsi
        - k3-am625.dtsi
            - k3-am62.dtsi
                - k3-pinctrl.h
                - k3-am62-thermal.dtsi
                - k3-am62-main.dtsi
                - k3-am62-mcu.dtsi
                - k3-am62-wakeup.dtsi

```

## yocto编译dfu烧录第一阶段所需tiboot3

[Boot Flow](https://docs.u-boot.org/en/v2024.10/board/ti/am62x_sk.html)

tiboot3-->R5
tispl/uboot --> A53

### 两个阶段 DFU 烧录的区别

1. **第一阶段（Bootloader DFU）**

- 目标：让板子具备基本启动能力，并能通过 DFU 接口继续烧录。
烧录内容：
        tiboot3.bin → R5 SPL（初始化 DDR、启动 A53）
        tispl.bin → A53 SPL（加载 U-Boot）
        u-boot.img → 完整 U-Boot
        env.txt → U-Boot 环境变量（可选）

- 特点：

    这些文件通常烧录到 RAM 或临时存储，用于启动 U-Boot。
    目的是让 U-Boot运行并暴露 DFU接口，方便后续烧录 rootfs。

1. **第二阶段（系统 DFU）**

- 目标：将完整系统写入持久存储（eMMC、SD、QSPI）。
烧录内容：

    tiboot3.bin → 固化到 Boot Partition
    tispl.bin → 固化到 Boot Partition
    u-boot.img → 固化到 Boot Partition
    rootfs → 写入 eMMC/SD 的 Linux 分区

- 特点：

    这一步是最终部署，系统可以独立启动。

### **mc:k3r5** 是什么？

mc = multiconfig  (在不写mc下是使用的mc:default默认值)
k3r5 = 针对 R5 核心的配置（通常对应 am62xx-lp-evm-k3r5.conf）

可以看到如下目录：

```sh
sources/meta-ti/meta-ti-bsp/conf/
├── layer.conf
├── machine
│   ├── am62xx-lp-evm.conf
│   ├── am62xx-lp-evm-k3r5.conf
|   |
|   ......
└── multiconfig
    └── k3r5.conf
```

因为在local.conf中有`MACHINE ?= am62xx-lp-evm`，所以默认是用的am62xx-lp-evm.conf,
如果编译时加了`mc:k3r5`,则使用am62xx-lp-evm-k3r5.conf来编译
这样就使用了不同的uboot配置，如`UBOOT_MACHINE = "am62x_lpsk_a53_defconfig"`和`UBOOT_MACHINE = "am62x_lpsk_r5_defconfig"`
一个针对A53,一个针对R5

### **为什么要用 multiconfig？**

AM62x 平台有 多核启动链：

- R5 → A53 → Linux。
- 所以需要同时编译：
  - R5 SPL（tiboot3.bin）
  - A53 U-Boot（tispl.bin + u-boot.img）
- Yocto通过 multiconfig 可以在一个构建中处理这两个目标，而不是分两次 build。

k3r5.conf内容如下：

```conf
TCLIBC = "baremetal"
TI_TMPDIR_APPEND ?= "-k3r5"
TMPDIR:append = "${TI_TMPDIR_APPEND}"
```

k3r5.conf 内容说明了 multiconfig的核心作用：它为 R5 构建定义了独立的 TMPDIR、部署路径和编译环境（baremetal），这样不会和 A53 的 Linux 构建冲突。
r5的构建目录定位到`build/arago-tmp-default-baremetal-k3r5/build/`下

### 增加DFU USB烧录功能配置，重新编译固件

1. way 1:

   1. 在local.conf中增加配置如下：

        ```local.conf
        MACHINE ?= "am62xx-lp-evm"

        # A53 U-Boot DFU fragment（可选）
        UBOOT_CONFIG_FRAGMENTS = "am62x_a53_usbdfu.config"

        # R5 U-Boot DFU fragment for multiconfig
        UBOOT_CONFIG_FRAGMENTS:k3r5 = "am62x_r5_usbdfu.config"

        ```

   2. 编译tiboot3

        ```sh
        ./build.sh try mc:k3r5:u-boot-staging
        ```

   3. 编译tispl, uboot

        ```sh
        ./build.sh try u-boot-staging
        ```

1. way 2:

   ```sh
   echo 'UBOOT_CONFIG_FRAGMENTS = "am62x_a53_usbdfu.config "' > dfu.conf
   echo 'UBOOT_CONFIG_FRAGMENTS:k3r5 = "am62x_r5_usbdfu.config"' >> dfu.conf
   bitbake -fc cleansstate "mc:k3r5:u-boot-ti-staging u-boot-ti-staging"
   bitbake -R dfu.conf mc:k3r5:u-boot-ti-staging u-boot-ti-staging
   rm dfu.conf
   ```
