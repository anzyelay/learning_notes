# OpenWrt

- openwrt是一个针对嵌入式设备的linux发行版。
- openwrt中的包管理器是opkg, 软件包是ipk。
- 所有独立编译好的包都会被模拟安装到一个临时目录，然后压缩成**[SquashFS](https://en.wikipedia.org/wiki/SquashFS)**（一个压缩只读的linux文件系统）分区的形式添加到一个设备固件中去
- 内核在openwrt中也是一个包，它会以每个设备的bootloader所能识别的方式被添加到固件镜像中去。

    注释:

    ***GNU/Linux 发行版:　是一个创建和维护程序包的项目，与 Linux 内核一起使用以创建适合用户需求的 GNU/Linux 操作系统。***

## 一、各模块简介

模块|全称|作用
|:-:|:-|:-|
[uci](https://openwrt.org/docs/guide-user/base-system/uci)| unified configuration interface| 其目标是将系统中各种服务原本需以不同形式的命令且处于不同目录的配置项，以**统一**的接口形式（相同的配置目录/etc/config, 统一的命令行控制如：uci show wireless）归集操控起来，用来完成各项服务的配置，而不用单独使用各服务的配置项，比如配置iptables, ifconfig, uhttpd等。
luCI |　a web interface for UCI，base on LUA and its openwrt | 提供一个Web页面来实现uci的配置，使用LUA编写
opkg | a package manager in OpenWrt |  a program that downloads/opens/installs/uninstalls the packages. 

## 二、[BASE SYSTEM](https://openwrt.org/docs/guide-user/base-system/start)

### 2.1 basic configuration

- uci： 使用命令行的形式配置
- luci: 使用web页面的形式配置
- classic Linux config： 基本的linux配置方式，不是所有的功能都兼容到uci里，需要单独配置

------------

## 三、 [构建系统要点](https://openwrt.org/docs/guide-developer/toolchain/buildsystem_essentials)

> 构建系统是一组 Makefile 和补丁，可自动构建交叉编译工具链，然后使用它来构建 Linux 内核、根文件系统以及在一个特定的设备上运行 OpenWrt 所需的其他软件（例如 uboot）。  一个典型的工具链包括：
>
> - 一个编译器, 如gcc
> - 二进制基础设施（binary utilities） 如 汇编器和链接器；类似binutils
> - 一个C标准库, 如 glibc, musl, uClibc or dietlibc

### 目录总览

#### root目录

dir       | 说明
----------|------------------
dl        | 所有要下载的包的存放目录
package   | openwrt第三方包的配置目录，在openwrt固件中一切都是ipk。
tools     | needs to be written about
toolchain | 指向用于构建固件镜像的编译器，C库和通用工具, 编译时会生成两个新的目录: <li> toolchain_build_\<arch\> : 用于构建指定架构工具链的临时目录 <li>staging_dir_\<arch\> : 工具链安装的目录<br>除非要添加新版本交叉工具，否则toolchain目录不需要任何改动
target    | 指向嵌入式平台相关的目录，这里包含了特定嵌入式平台的项目文件，特别要关注的是“target/linux”和“target/image” <li>target/linux : 特定平台的内核配置与补丁文件目录 <li>target/image : 针对特定平台的打包方式目录
bin       | 编译好的固件和包的存放地
build_dir | 包解压缩和编译的目录
stage_dir | 所有编译好的程序的安装目录，为构建其它包或固件镜像做预备的目录

#### build_dir和staging_dir下都包含host, toolchain-.., target-...三个目录
  
dir                        | 说明
---------------------------|---------------------------------
build_dir/host             | 编译的在本机上运行的所有openwrt工具的存放目录. 此区域用于存放只在本主机上运行的编译好的应用.
build_dir/toolchain…       | 用于编译包所需的交叉编译器和C库的存放目录. 此区域用于存放只在本主机上运行的编译工具(比如C交叉编译器等)和链接到目标机运行的库，比如uClibc, libm, pthreads等.
build_dir/target…          | 编译出来的真正运行在目标系统上的包和内核的存放目录
staging_dir/host           | 一个包含了bin/,lib/，etc等的最小linux根，主机编译工具和其余构建系统所需工具加前缀地址前的根安装目录
staging_dir/toolchain…     | 包含bin, lib和其它用于构建固件的交叉编译器的最小linux根. 你可以用它在openwrt外部编译能加载到固件中的简单c程序. C编译器看起来像这样：`staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-uclibc-gcc`.你可以看到编码进编译器的cpu, C库和gcc的版本;因此同时在一个构建目录下有多个目标板是允许的.
staging_dir/target…/root-… | 包含每个目标包的“已安装”版本，再次以 bin/、lib/ 排列，这将成为实际的根目录，通过一些调整将压缩到固件映像中，类似于`root-ar71xx`。  在 staging_dir/target 中还有一些其他文件……主要用于生成包和开发包等。

### 可用的Make目标（Make tagets）

> - 为了包工作流的标准化，openwrt提供了许多的高层make目标
> - 这些Make目标的格式形如：`component/name/action`, 比如： `toolchain/gdb/compile`或 `package/mtd/install`
> - 准备一个包的源码目录树: `package/foo/prepare`
> - 编译一个包: `package/foo/compile`
> - 清除一个包: `package/foo/clean`

### 构建顺序

> 1. tools – automake, autoconf, sed, cmake
> 1. toolchain/binutils – as, ld, …
> 1. toolchain/gcc – gcc, g++, cpp, …
> 1. target/linux – kernel modules
> 1. package – core and feed packages
> 1. target/linux – kernel image
> 1. target/linux/image – firmware image file generation

### [补丁管理](./patch.md)

> - 许多包不能按原样工作，需要补丁才能在目标上工作甚至编译
> - 构建系统集成了`quilt`，便于补丁管理
> - 将包补丁应用到展开的源码目录下的quilt补丁系统中: `make package/foo/prepare QUILT=1`
> - 将源码中的修改更新到quilt管理补丁中: `make package/foo/update`
> - 将quilt补丁自动添加到包补丁中: `make package/foo/refresh`

### 搭建构建系统环境

- 针对debian/ubuntu

    ```sh
    sudo apt install build-essential clang flex g++ gawk gcc-multilib gettext \
    git libncurses5-dev libssl-dev python3-distutils rsync unzip zlib1g-dev \
    file wget
    ```

- 其它参考[build system setup](https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem)
