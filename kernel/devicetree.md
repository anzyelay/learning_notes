# device tree

- [device tree](#device-tree)
  - [元素定义作用](#元素定义作用)
    - [属性字段](#属性字段)
    - [特殊节点](#特殊节点)
    - [常见标准前缀列表（用于 aliases）](#常见标准前缀列表用于-aliases)
  - [tool for debug](#tool-for-debug)
    - [使用示例](#使用示例)

## 元素定义作用

```dts

aliases {
    serial0 = &uart0;
};


soc {
    uart0: serial@2800000 {
        compatible = "ti,omap-uart";
        reg = <0x2800000 0x1000>;
        status = "okay";
    };
};

```

  名称               |   类型     |   作用
  -----------------  |  --------  |  -------
  / { ... }          |  根节点    |   "/"标识设备树的起点
  serial@2800000     |   节点名   |   表示设备地址
  uart0:             |   标签     |   给设备节点起别名，便于引用
  &uart0             |   句柄     |   引用该设备节点
  serial0 = &uart0   |   alias    |   给设备一个统一的逻辑名称，供内核获取设备的编号或别名，从而实现统一的初始化逻辑, 保持设备编号稳定 eg:<br> `int id = of_alias_get_id(np, "serial");`<br> - *这会返回 serial0 是哪个设备，从而决定它是 /dev/ttyS0*
  {...} | 节点内容 | 包含属性

- 在 Linux 设备树中，aliases 节点的键名（如 serial0, mmc0, ethernet0 等）并不是随意定义的，而是有一套约定俗成的标准前缀，这些前缀是内核和驱动程序通过函数如 of_alias_get_id() 来识别和使用的。

### 属性字段

属性 | 说明
-|-
compatible | 匹配驱动的字符串
reg | 寄存器地址和长度
status | "okay" 表示启用，"disabled" 表示禁用
interrupts | 中断号
clocks | 时钟源引用
pinctrl-names / pinctrl-0 | 引脚配置
dma-names / dmas | DMA 配置
phandle显| 式句柄（通常自动生成）

### 特殊节点

节点| 作用|
-|-
/aliases | 定义设备逻辑名称
/chosen | 启动参数、内核命令行等
/memory | 定义内存区域
/reserved-memory | 保留内存区域（如 CMA）
/soc | SoC 设备节点容器
/cpus | CPU 核心定义

### 常见标准前缀列表（用于 aliases）

  前缀代表   |   设备类型         |   示例别名 | 对应设备
  ---------  |  ----------------  |  --------------------- |--
  serial     |   串口设备（UART）   |   serial0, serial1    | /dev/ttyS0, /dev/ttyS1
  ethernet   |   网口设备         |   ethernet0, ethernet1  | eth0, eth1
  mmc        |   SD 卡或 eMMC     |   mmc0, mmc1    |/dev/mmcblk0, /dev/mmcblk1
  spi        |   SPI 总线         |   spi0, spi1    | /dev/spidev0.0, /dev/spidev1.0
  i2c        |   I2C 总线         |   i2c0, i2c1    | /dev/i2c-0, /dev/i2c-1
  usb        |   USB 控制器       |   usb0, usb1    | /dev/bus/usb/...
  rtc        |   实时时钟         |   rtc0, rtc1    | /dev/rtc0, /dev/rtc1
  pwm        |   PWM 控制器       |   pwm0, pwm1    | /sys/class/pwm/...
  gpio       |   GPIO 控制器      |   gpio0, gpio1  | /dev/gpiochip0, /dev/gpiochip1
  can        |   CAN 总线         |   can0, can1    | /dev/can0, /dev/can1
  sound      |   音频设备         |   sound0    | /dev/snd/...
  display    |   显示设备         |   display0  | DRM/FB 设备
  camera     |   摄像头设备       |   camera0   | V4L2 设备 /dev/video0

## tool for debug

工具| 主要功能| 输入格式 |输出格式 | 适用场景
-|-|-|-|-
dtc | (Device Tree Compiler)编译/反编译 | DTS 与 DTB | DTS / DTB| DTS / DTB开发阶段查看结构、调试、生成设备树
fdtdump| 以结构化方式打印 DTB 内容| DTB|类似 DTS 的文本结构| 快速查看 DTB 内容，无需反编译
fdtget | 从 DTB 中读取某个节点的属性值| DTB| 属性值（文本）| 脚本中提取设备树参数
fdtput | 修改 DTB 中某个节点的属性值| DTB | 修改后的 DTB| 自动化修改设备树参数（如地址、状态）

### 使用示例

```sh
# 编译 DTS → DTB
dtc -I dts -O dtb -o myboard.dtb myboard.dts

# 反编译 DTB → DTS
dtc -I dtb -O dts -o myboard.dts myboard.dtb

# 输出类似 DTS 的结构，但不是真正的 DTS，可快速查看节点和属性
fdtdump myboard.dtb

# 读取 /memory 节点的 reg 属性值，适合脚本中提取信息
fdtget myboard.dtb /memory reg

# 修改 /chosen 节点的 bootargs 属性，适合自动化部署或测试。
fdtput myboard.dtb /chosen bootargs "console=ttyS0,115200"
```
