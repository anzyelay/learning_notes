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

## TC 流控

### AM62X mqprio 应用

```sh
#!/bin/bash
TOTAL_RATE=$(ethtool eth1 | grep Speed | awk '{ print int($2) }')
TX_NUM=3
PRI_MAP_TC=( 0 1 2 )
TC_MAP_TXQ=( 1@0 1@1 1@2 )
SINGLE_RATE=$((${TOTAL_RATE}/${TX_NUM}))
for((i=0;i<${TX_NUM};i++));do
        rate=${1:-$SINGLE_RATE}
        MINRATE+=( ${rate}mbit )
        total_min_rate=$((total_min_rate+rate))
        shift
done
echo ${MINRATE[@]}
for((i=0;i<${TX_NUM};i++));do
        rate=${1:-$SINGLE_RATE}
        MAXRATE+=( ${rate}mbit )
        total_max_rate=$((total_max_rate+rate))
        shift
done
echo ${MAXRATE[@]}
let total_max_rate=0
let total_min_rate=0
for((i=0;i<${TX_NUM};i++));do
        max_rate=${MAXRATE[$i]%mbit}
        min_rate=${MINRATE[$i]%mbit}
        echo "$i : [$min_rate,$max_rate]"
        if [ $max_rate -lt $min_rate ];then
                echo "error: the max rate($max_rate) is less than min rate($min_rate), exit"
                exit 0
        fi
        let total_max_rate+=max_rate
        let total_min_rate+=min_rate
done
echo "total rate: [$total_min_rate,$total_max_rate]"
if [ ${TOTAL_RATE} -lt $total_max_rate ];then
        echo the total rate is bigger than $TOTAL_RATE
        exit 0
fi

ip link set eth1 down
ip link set eth0 down
# 默认就是8个,可以不设
ethtool -L eth1 tx 8 && ethtool --set-priv-flags eth1 p0-rx-ptype-rrobin off
ip link set eth0 up
ip link set eth1 up

tc qdisc show dev eth1 clsact | grep clsact > /dev/null && {
        echo delete clsact
        tc qdisc del dev eth1 clsact
}
tc qdisc show dev eth1 root | grep mqprio > /dev/null && {
        tc qdisc del dev eth1 root
}

sleep 1
num=$(ls -d /sys/class/net/eth1/queues/tx-* | wc -l)
if [ ${num} -lt ${TX_NUM} ]; then
        echo "the number of tx($num) is less than ${TX_NUM}"
        exit 0
fi
ethtool eth1 | grep Speed

tc qdisc add dev eth1 handle 100: root mqprio num_tc ${TX_NUM} \
        map ${PRI_MAP_TC[*]} \
        queues ${TC_MAP_TXQ[*]} \
        hw 1 mode channel \
        shaper bw_rlimit \
        min_rate ${MINRATE[*]} \
        max_rate ${MAXRATE[*]}

tc qdisc show dev eth1

echo add clsact
tc qdisc add dev eth1 clsact
echo add filter
tc filter add dev eth1 egress protocol ip prio 1 \
        u32 match ip dport 5202 0xffff \
        action skbedit priority 1
tc filter add dev eth1 egress protocol ip prio 1 \
        u32 match ip dport 5203 0xffff \
        action skbedit priority 2

tc filter show dev eth1 egress

#如果一次也没设置过，设置时必须先高后低设置, 可能过下面的接口限制不同tx-ch的速率
for((ch=${TX_NUM}, ch--;ch>=0;ch--)); do
        #echo  1000 > /sys/class/net/eth1/queues/tx-${ch}/tx_maxrate
        ret=$(cat /sys/class/net/eth1/queues/tx-${ch}/tx_maxrate)
        echo $ch maxrate is $ret
done

# debug informations in bellow
echo PRI_MAP:
devmem2  0x08023018
echo '===CIR(Committed Information Rate Value) and EIR(Excess Informatoin Rate Value)==='
cir_base_reg=0x08023140
eir_base_reg=0x08023160
for((i=0;i<${TX_NUM};i++));do
        echo "$i: cir & eir"
        reg=$(printf "0x%08X" $((cir_base_reg+i*4)))
        devmem2 $reg | grep $reg

        reg=$(printf "0x%08X" $((eir_base_reg+i*4)))
        devmem2 $reg | grep $reg
done

```

mqprio中的opt struct

```c
struct tc_mqprio_qopt {
        __u8	num_tc; // num_tc: value
        __u8	prio_tc_map[TC_QOPT_BITMASK + 1]; // map: value 16位
        __u8	hw; // 是否启用 mqprio_enable_offload 标识
        __u16	count[TC_QOPT_MAX_QUEUE]; // queues: count@offset, offset是tx队列的偏移值，count表示此tc占用几个tx queues队列
        __u16	offset[TC_QOPT_MAX_QUEUE]; // queues: count@offset
};
```

**以下为以map第0位的值和queue两个为变量，配置后导致的数据在的哪个tx_priX和parent 100:x的变化**
具体查看指令为：`ethtool -S eth1 | grep tx | grep -v ": 0" ; tc -s qdisc  show dev eth1`

  map X   |   queue         |   tx_priX   |   parent 100:X
  ------  |  -------------  |  ---------  |  -------------
  2       |   1@2 1@1 1@0   |   2         |   1
  1       |   1@2 1@1 1@0   |   1         |   2
  0       |   1@2 1@1 1@0   |   0         |   3
  0       |   1@0 1@1 1@2   |   0         |   1
  1       |   1@0 1@1 1@2   |   1         |   2
  2       |   1@0 1@1 1@2   |   2         |   3

1. map的x所在的位置序号number表了skb->priority对应的level值，一共16位,分别代表priority: `0~15`， 这个level值用来在filter里设置`action skbedit priority`
1. map中某个位的**值**决定了该位的`priority`的数据流映射到`tx_pri`的哪个队列**TC-n**.
1. queues的位置序号就是**TC-n**对应的number（即TC队列从小到大在queues中排列），其位置上值`(x:y)`就表示该**TC-n**使用`qdisc pfifo_fast`中的`parent 100:x+1`到`parent 100:y+1`队列。比如：`queues 2@0 3@2 3@5` --show--> `queues:(0:1) (2:4) (5:7)`
1. min/max rate中的位置序号同样就是代表TC-n的队列的排序，对应的数据就是对应TC-n的速率
1. 单条流速率最大值可达；两条流的速率最大值由较小的流的最大值限制；三条流有一条流会被饿死，其它两条流的速率由较小流的最大值限制。

以下为表格中最后一行的配置查看数据,表示priority为0的数据映射到TC2，TC2使用的是(2:2)即`parent 100:3`那一队规。其最大速率限制在1Gbit以内

```sh
tx_pri0: 35820060
tx_pri1: 21423063
tx_pri2: 44897959
tx_pri0_bcnt: 439859550
tx_pri1_bcnt: 2444897170
tx_pri2_bcnt: 3703653317
qdisc mqprio 100: root tc 3 map 2 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0
             queues:(0:0) (1:1) (2:2)
             mode:channel
             shaper:bw_rlimit   min_rate:10Mbit 30Mbit 60Mbit   max_rate:20Mbit 50Mbit 1Gbit
             fp: E E E E E E E E E E E E E E E E

 Sent 5776165054 bytes 3817033 pkt (dropped 0, overlimits 0 requeues 105229)
 backlog 0b 0p requeues 105229
qdisc pfifo_fast 0: parent 100:3 bands 3 priomap 1 2 2 2 1 2 0 0 1 1 1 1 1 1 1 1
 Sent 5776164530 bytes 3817027 pkt (dropped 0, overlimits 0 requeues 105229)
 backlog 0b 0p requeues 105229
qdisc pfifo_fast 0: parent 100:2 bands 3 priomap 1 2 2 2 1 2 0 0 1 1 1 1 1 1 1 1
 Sent 0 bytes 0 pkt (dropped 0, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0
qdisc pfifo_fast 0: parent 100:1 bands 3 priomap 1 2 2 2 1 2 0 0 1 1 1 1 1 1 1 1
 Sent 524 bytes 6 pkt (dropped 0, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0
qdisc clsact ffff: parent ffff:fff1
 Sent 5776165054 bytes 3817033 pkt (dropped 0, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0
```

**总结：**

1. num_tc表示使用几个TC队列，其值不能大于`real_num_tx_queues`,比如AM62最大支持8个发送队列
1. map决定了`skb->priority`与TC队列的映射关系, 其值不能大于`num_tc`设置值
1. queues行从前往后排依次表示TC队列`TC0`到`TCx`的占用tx硬件队列，以`(start:end)`形式显示，以`count@offset`为设置标准。其中`offset+count<real_num_tx_queues`， TCx之间配置的tx队列不可overlap
1. rate行从前往后依次表示队列TC0到TCx的速率值, 单位是**bit**, 换算成**byte**要整除8
1. 速率有两条以上流时以最小流的max_rate为最大值，三条流会饿死最小的流。
1. 所有速率（所有max_rate或所有min_rate）加起来不能超过link_rate，如果超过配置无效。

### HTB 应用

```sh
#!/bin/sh
INTERFACE=eth1
TOTAL_RATE=800Mbit

# === 1. Clean up existing TC configuration ===
tc qdisc del dev "$INTERFACE" root 2>/dev/null

# === 2. Add root HTB qdisc (total bandwidth capped at 700M) ===
tc qdisc add dev "$INTERFACE" root handle 1: htb default 10

# === 3. Define HTB classes (lower 'prio' = higher scheduling priority) ===
# High-priority class: dst_port 5203 �� classid 1:30 (prio 0)
tc class add dev "$INTERFACE" parent 1: classid 1:30 htb \
    rate 200Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 0 quantum 2000

# Medium-priority class: dst_port 5202 �� classid 1:20 (prio 1)
tc class add dev "$INTERFACE" parent 1: classid 1:20 htb \
    rate 100Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 1 quantum 2000

# Default (low-priority) class: all unmatched traffic �� classid 1:10 (prio 2)
tc class add dev "$INTERFACE" parent 1: classid 1:10 htb \
    rate 50Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 2 quantum 1500

# === 4. Add u32 filters to classify traffic by destination port ===
# Match dst_port = 5203 (0x1453) �� send to high-priority class 1:30
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 1 \
    u32 match ip dport 5203 0xffff \
    flowid 1:30

# Match dst_port = 5202 (0x1452) �� send to medium-priority class 1:20
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 2 \
    u32 match ip dport 5202 0xffff \
    flowid 1:20

# Note: All other traffic automatically falls into default class 1:10 (via 'default 10')

# === 6. Verification and usage instructions ===
echo "? TC QoS configuration applied successfully!"
echo "   - High priority: dst_port=5203 �� 200M guaranteed (HTB prio 0)"
echo "   - Medium priority: dst_port=5202 �� 100M guaranteed (HTB prio 1)"
echo "   - Default priority: all other traffic �� 50M guaranteed (HTB prio 2)"
echo "   - Total egress bandwidth limited to 700M"
echo ""
echo "?? Example test commands (run on client):"
echo "   iperf3 -c <SERVER_IP> -p 5203 -t 30  # High-priority stream"
echo "   iperf3 -c <SERVER_IP> -p 5202 -t 30  # Medium-priority stream"
echo ""
echo "?? Check real-time statistics:"
tc -s class show dev "$INTERFACE" | grep -E "(class|Sent|rate)"
```

测试发现该平台不支持clsact, 所以在添加过滤规则前不能加
`tc qdisc add dev "$INTERFACE" clsact`, 所以如下脚本无效

```sh
root@fvt-5g-tbox:/tmp# diff valid.sh invalid.sh
--- valid.sh
+++ invalid.sh
@@ -3,6 +3,7 @@
 TOTAL_RATE=800Mbit

 # === 1. Clean up existing TC configuration ===
+tc qdisc del dev "$INTERFACE" clsact 2>/dev/null
 tc qdisc del dev "$INTERFACE" root 2>/dev/null

 # === 2. Add root HTB qdisc (total bandwidth capped at 700M) ===
@@ -22,15 +23,18 @@
 tc class add dev "$INTERFACE" parent 1: classid 1:10 htb \
     rate 50Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 2 quantum 1500

-# === 4. Add u32 filters to classify traffic by destination port ===
+# === 4. Attach clsact qdisc to enable egress filters ===
+tc qdisc add dev "$INTERFACE" clsact

+# === 5. Add u32 filters to classify traffic by destination port ===
+
 # Match dst_port = 5203 (0x1453) �� send to high-priority class 1:30
-tc filter add dev "$INTERFACE" protocol ip parent 1: prio 1 \
+tc filter add dev "$INTERFACE" egress protocol ip prio 1 \
     u32 match ip dport 5203 0xffff \
     flowid 1:30

 # Match dst_port = 5202 (0x1452) �� send to medium-priority class 1:20
-tc filter add dev "$INTERFACE" protocol ip parent 1: prio 2 \
+tc filter add dev "$INTERFACE" egress protocol ip prio 2 \
     u32 match ip dport 5202 0xffff \
     flowid 1:20

```

使用上述无效脚本时, 发现无效, 使用如下命令查看**TC**规则根本没有生效 —— 流量根本没进入你定义的 class，而是走了默认路径或被其他机制限制。

`tc -s class show dev eth1 | grep -E "(class|Sent|rate)"`

 **结论**：`TC egress` 规则未匹配任何包！
 **修正方案**：移除 `clsact`，改用 `root` 下的 `u32 filter`, 因为 `htb root` 本身支持 `protocol ip u32 match ... flowid`，无需 `clsact`！

1. **问题一：为什么 去掉 clsact、改用传统 root 下的 u32 filter 就能命中，而 clsact egress 不行?**

    **根本原因：流量路径（packet path）不同**
    Linux 内核中，网络包有两条主要出口路径：

    |流量类型 | 路径 | 是否经过 clsact egress|
    |-|-|-|
    |转发流量（Forwarded）| PREROUTING → FORWARD → POSTROUTING | ✅ 经过 clsact egress|
    |本地生成流量（Locally generated）| LOCAL_OUT → POSTROUTING | ❌ 可能绕过 clsact egress|

    在大多数标准 Linux 发行版（如 Ubuntu）中，clsact egress 确实能捕获本地流量,但在某些嵌入式系统, 由于
    - 内核配置裁剪（如 CONFIG_NET_SCH_CLSACT 功能不全）
    - 网络驱动特殊处理（如 CPSW 驱动 bypass 软件队列）
    - 内核版本较旧（< 5.4）

    `clsact egress` 只对转发流量生效，本地流量直接走 `dev_queue_xmit()`，跳过 `TC egress`

     **clsact vs 传统 root filter 对比**

    | 特性 | clsact (ingress/egress)| 传统 root HTB + u32|
    |-|-|-|
    | 支持 BPF/flower 高级匹配 |	✅ 是 |❌ 否（仅 u32/flowid）|
    | 可同时做 ingress + egress |	✅ 是 | ❌ egress only|
    | 对本地流量的兼容性 |	⚠️ 依赖内核/驱动（AM62 常失效）|	✅ 100% 可靠|
    | 硬件 offload 支持 |	✅ 更好（如 XDP） |	❌ 较弱|
    | 适用场景	| 现代服务器、完整内核 |	嵌入式、裁剪系统、AM62|

1. **问题二、不能动态共享带宽，HTB 单流无法超过 rate，即使链路空闲，不能动态共享剩余带宽**

使用命令查看数据，发现每个class都能看到sent数据在涨，但**borrowed**就是指的借用共享带宽的数据，如果为0说明没有借用，ceil没起作用, 但overlimits>0（说明尝试调度）

```sh
root@tbox:/tmp# tc -s class show dev eth1
class htb 1:10 root prio 2 rate 50Mbit ceil 800Mbit burst 1600b cburst 1600b
 Sent 668567661 bytes 447560 pkt (dropped 0, overlimits 19375 requeues 0)
 backlog 68Kb 2p requeues 0
 lended: 25914 borrowed: 0 giants: 0
 tokens: -158692 ctokens: -9686

class htb 1:20 root prio 1 rate 100Mbit ceil 800Mbit burst 1600b cburst 1600b
 Sent 2188086736 bytes 1445246 pkt (dropped 0, overlimits 33506 requeues 0)
 backlog 136260b 2p requeues 0
 lended: 34182 borrowed: 0 giants: 0
 tokens: -85072 ctokens: -10396

class htb 1:30 root prio 0 rate 200Mbit ceil 800Mbit burst 1600b cburst 1600b
 Sent 2894667648 bytes 1911946 pkt (dropped 0, overlimits 42545 requeues 0)
 backlog 136260b 2p requeues 0
 lended: 42559 borrowed: 0 giants: 0
 tokens: -42397 ctokens: -10396
 ```

这意味着：

- 每个`class`只使用了自己的`guaranteed rate`
- 从未向父类（root）借用额外带宽
- 所以即使链路空闲，也无法超过`rate`

**正常 HTB 行为（Linux 主线内核）：**

- 当一个 class 的令牌桶（tokens）耗尽，但 ceil 未达上限 → 会尝试 borrow from parent
- 成功后：borrowed 计数增加，速率可超过 rate

**但当前系统行为：**

- HTB borrowing 机制被禁用或 broken
- TI 定制内核（如 SDK 8.x / 9.x）对 HTB 做了裁剪
- CPSW 驱动与 TC 软件队列不兼容
- 内核配置缺少 CONFIG_NET_SCH_HTB 完整支持（虽然能加载，但 borrowing 逻辑缺失）

> 这在 TI 社区和 Yocto 用户中是已知问题。AM62 的 CPSW 子系统更倾向于硬件 QoS（如 CBS、ETS），而非软件 HTB borrowing。

**这是内核/驱动限制,不是配置错误, 如下数据可以证明：**

- 流量正确分类（Sent > 0）
- overlimits > 0（说明尝试调度）
- 但 borrowed = 0（borrowing 失败）
