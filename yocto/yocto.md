# YOCTO

1. 对已知的**TASKS**和**VARIABLES**的说明文件在

   - sources/oe-core/meta/conf/documentation.conf

1. 标准目标文件系统的路径变量和构建过程的变量定义的文件所在(Standard target filesystem paths)

   - sources/oe-core/meta/conf/bitbake.conf

1. 常见错误和警告参考文档

   - https://docs.yoctoproject.org/ref-manual/qa-checks.html

## yocto

bb/bbclass中的变量说明,未说到的可以参数前一章节第1点的说明：

### 变量解释

变量名 | 含义
-|-
WORKDIR | 当前配方的工作目录，由 BitBake 自动生成, `${TMPDIR}/work/${MULTIMACH_TARGET_SYS}/${PN}/${EXTENDPE}${PV}-${PR}`
S | 源码包在构建目录下的解压出来的目录（S[doc] = "The location in the Build Directory where unpacked package source code resides."）
B | 构建目录，默认等于 `S`，但可以单独设置
PN | package name, PN指的是OpenEmbedded构建系统使用的文件上下文中的配方名(recipe name)，作为创建包的输入。它指的是OpenEmbedded构建系统创建或生成的文件上下文中的包名
PV | 配方版本，例如 2.10
DEPENDS | 后面接一个recipe name，或者一个recipe name list, Build time dependency, foo构建时的依赖, 只有依赖包先编译成功后才能编译foo
RDEPENDS | 后面接一个recipe name，或者一个recipe name list, Run time dependency: foo运行时依赖, 表示该依赖包被正常安装后foo才能正常运行
PROVIDES | 主要是为了起别名

  比如foo.bb内容如下

  ```bitbake
  # foo的编译依赖bar，所以先编译bar
  DEPENDS = "bar"
  # 带native表示需要依赖构建主机上的程序
  DEPENDS = "bar-native"
  # package version? 在编译时会在构建recipe-name目录下创建version-dir目录
  PV = "version-dir"
  ```

### 在BB中加调试信息

- bbnote：用来打印
- bbwarn：用来打印
- bbfatal：用来打印
- eval:用来执行语句

```bb
  bbnote ${DESTDIR:+DESTDIR=${DESTDIR} }${CMAKE_VERBOSE} cmake --build '${B}' --target test -- ${EXTRA_OECMAKE_BUILD}
  eval ${DESTDIR:+DESTDIR=${DESTDIR} }${CMAKE_VERBOSE} cmake --build '${B}' --target test -- ${EXTRA_OECMAKE_BUILD}
```

### 编译的情况

一个recipe真正构建的目录所在大致在： build/xxx/work/xxx-platform-info/recipe-name/version/temp

- recipe-name构建目录下紧跟一个与version对应相关的目录
- 在这个version目录下存放着`build,temp,image,pkgdata,package,package-split,pseudo,*.patch`等一堆目录或文件，version由变量**PV**指定
- build下为构建输出的目录
- "recipe-sysroot[-native]": 为YOCTO编译环境所需的目标平台的库、头文件和配置等存放的目录，在`DEPENDS`变量里的库都会拷贝到此，native则是DEPENDS中加了-native后缀的本机的库
- temp为bitbake运行的task和对应任务运行日志的目录，命名以`run.do_task`,`log.do_task`及后缀pid号的文件

### bitbake

假设在 **cwd** 或 **BBPATH** 中有一个可用的`conf/bblayers.conf`配置文件，它将提供layer、BBFILES和其他配置信息。

- 指令用法说明

    ```sh
    General options:
    recipename/target     为指定的recipe目标(.bb files)执行特定任务(默认为 build 任务).
    -s, --show-versions   显示所有recipes的 **当前** 和 **首选** 版本
    -e, --environment     显示全局或每个recipe的环境变量，此环境变量包含变量在哪被设置或修改的完整信息
    -g, --graphviz        以dot语法格式保存指定目标的依赖树信息
    -u UI, --ui UI        The user interface to use (knotty, ncurses, taskexp_ncurses or teamcity - default knotty).

    Task control options:
    -f, --force           强制执行指定目标/任务(使任何已存的stamp(戳记)文件失效).
    -c CMD, --cmd CMD     指定运行某个task. 可用的确切选项取决于元数据. 比如task可以是'compile'或'populate_sysroot'或'listtasks'.
    -C INVALIDATE_STAMP, --clear-stamp INVALIDATE_STAMP
                            Invalidate the stamp for the specified task such as 'compile' and then run the default task for the
                            specified target(s).
    --runall RUNALL       Run the specified task for any recipe in the taskgraph of the specified target (even if it wouldn't
                            otherwise have run).
    --runonly RUNONLY     Run only the specified task within the taskgraph of the specified targets (and any task dependencies
                            those tasks may have).
    --no-setscene         Do not run any setscene tasks. sstate will be ignored and everything needed, built.
    --skip-setscene       Skip setscene tasks if they would be executed. Tasks previously restored from sstate will be kept,
                            unlike --no-setscene.
    --setscene-only       只运行setscene任务，不运行任何实际任务

    Execution control options:
    -n, --dry-run         不执行，只是走过场
    -p, --parse-only      解析完BB配方后退出.
    -k, --continue        在出现错误后尽可能多的继续编译下去. 当目标构建失败，依赖于它的其它目标也不可能被构建，在错误目标停止构建前尽可能多的编译其它可构建的目标
    -P, --profile         配置命令并保存报告
    -S SIGNATURE_HANDLER, --dump-signatures SIGNATURE_HANDLER
                            Dump out the signature construction information, with no task execution. The SIGNATURE_HANDLER
                            parameter is passed to the handler. Two common values are none and printdiff but the handler may
                            define more/less. none means only dump the signature, printdiff means recursively compare the dumped
                            signature with the most recent one in a local build or sstate cache (can be used to find out why tasks
                            re-run when that is not expected)
    --revisions-changed   Set the exit code depending on whether upstream floating revisions have changed or not.
    -b BUILDFILE, --buildfile BUILDFILE
                            Execute tasks from a specific .bb recipe directly. WARNING: Does not handle any dependencies from
                        log.do_fetch.47910    other recipes.

    Logging/output control options:
    -D, --debug           Increase the debug level. You can specify this more than once. -D sets the debug level to 1, where
                            only bb.debug(1, ...) messages are printed to stdout; -DD sets the debug level to 2, where both
                            bb.debug(1, ...) and bb.debug(2, ...) messages are printed; etc. Without -D, no debug messages are
                            printed. Note that -D only affects output to stdout. All debug messages are written to
                            ${T}/log.do_taskname, regardless of the debug level.
    -l DEBUG_DOMAINS, --log-domains DEBUG_DOMAINS
                            Show debug logging for the specified logging domains.
    -v, --verbose         Enable tracing of shell tasks (with 'set -x'). Also print bb.note(...) messages to stdout (in addition
                            to writing them to ${T}/log.do_<task>).
    -q, --quiet           Output less log message data to the terminal. You can specify this more than once.
    -w WRITEEVENTLOG, --write-log WRITEEVENTLOG
                            Writes the event log of the build to a bitbake event json file. Use '' (empty string) to assign the
                            name automatically.

    Server options:
    -B BIND, --bind BIND  The name/address for the bitbake xmlrpc server to bind to.
    -T SERVER_TIMEOUT, --idle-timeout SERVER_TIMEOUT
                            Set timeout to unload bitbake server due to inactivity, set to -1 means no unload, default:
                            Environment variable BB_SERVER_TIMEOUT.
    --remote-server REMOTE_SERVER
                            Connect to the specified server.
    -m, --kill-server     Terminate any running bitbake server.
    --token XMLRPCTOKEN   Specify the connection token to be used when connecting to a remote server.
    --observe-only        Connect to a server as an observing-only client.
    --status-only         Check the status of the remote bitbake server.
    --server-only         Run bitbake without a UI, only starting a server (cooker) process.

    Configuration options:
    -r PREFILE, --read PREFILE
                            Read the specified file before bitbake.conf.
    -R POSTFILE, --postread POSTFILE
                            Read the specified file after bitbake.conf.
    -I EXTRA_ASSUME_PROVIDED, --ignore-deps EXTRA_ASSUME_PROVIDED
                            Assume these dependencies don't exist and are already provided (equivalent to ASSUME_PROVIDED). Useful
                            to make dependency graphs more appealing.
    ```

- 常用指令：

    1. 只运行recipe中的某个特定任务, 会把对应recipe依赖的其它recipe也运行

        ```sh
        bitbake -c task recipe-name
        ```

        常见task:
            下载（fetch）、解包（unpack）、打补丁（patch）、配置（configure）、编译（compile）、安装（install）、打包（package）、staging、做安装包（package_write_ipk）、构建文件系统， 发布SDK包（populate_sdk）等,可以在对应的构建目录下的temp中看到一堆的run.do_xxx, xxx就是task。
        不指定任务则是默认build

        eg:
        - `bitbake -fc cleansstate recipe-name`: 清包
        - `bitbake -c clean recipe-name`: 会把下载包也清了

    1. 可针对某个recipe寻找变量信息

       `bitbake -e recipe-name | grep -e '\bKERNEL_VERSION\b'`

### [从基础出发构建自己的recipes](https://wiki.yoctoproject.org/wiki/Building_your_own_recipes_from_first_principles)

1. Build an example package based on a git repository commit
1. Build an example package based on a remote source archive
1. Build an example package based on a local source archive

  ```sh
  # 前提：
  # 1. 假设bb文件所以:   yocto/poky-jethro-14.0.0/meta-example/recipes-example/bbexample/bbexample-lt_1.0.bb
  # 2. 假设包所在：      yocto/poky-jethro-14.0.0/meta-example/recipes-example/bbexample/bbexample-lt-1.0/bbexample-v1.0.tar.gz

  # 指定本地包
  SRC_URI = "file://bbexample-${PV}.tar.gz"
  # 指定bitbake搜索路径, 使用_prepend会报new bitbake不兼营错误
  FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}-${PV}:"
  # 确保构建的源目录与tarball中的目录结构匹配, 即跟压缩包中的目录名一样
  S = "${WORKDIR}/bbexample-${PV}"

  ```

   1. Make sure our source directory (for the build) matches the directory structure in the tarball
   2. We provide a search path to ensure bitbake can find the archive
   3. There is no SRC_REV here or check-sum for the local archive.



### 修改kernel使用本地文件

新建目录，在其中新建对应内核的bb的bbapend文件。内容如下

1. 如果内核编译的recipe为`linux-ti-staging_6.6.bb`，则在自己目录下新建`recipes-kernel`目录，下放`files`目录和`linux-ti-staging_6.6.bbappend`文件,
内核代码linux-6.6放到files目录下。

  ```sh
  ti_yocto$ tree sources/meta-demo/recipes-kernel/linux/  -L 2
  sources/meta-demo/recipes-kernel/linux/
  ├── files
  │   └── linux-6.6
  └── linux-ti-staging_6.6.bbappend

  2 directories, 1 file
  ```

1. linux-ti-staging_6.6.bbapend内容如下，将files加到搜寻目录变量中去，以便可以找到, "**:**"不要丢，跟PATH变量一样以“:”分隔追加的

  ```sh
  FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
  ```

1. 修改linux-ti-staging_6.6.bb, 也可以bbappend中修改它， 一定要将**S**的名字改成与**SRC_URL**一致的名称，否则没有链接过去，编译报错

  ```sh

  S = "${WORKDIR}/linux-6.6"

  SRC_URI += "file://linux-6.6"
  ```

### CMAKE的使用

示例`powervr-graphics_5.11.1.bb`内容如下：

```bb
PR = "r0"

DESCRIPTION = "Imagination PowerVR SDK binaries/examples"
HOMEPAGE = "https://docs.imgtec.com"

COMPATIBLE_MACHINE = "am62xx|am62pxx"

SRC_URI = " \
    gitsm://github.com/powervr-graphics/Native_SDK.git;protocol=https;branch=${BRANCH} \
    file://0001-PATCH-use-library-so-names-for-linking.patch \
"

BRANCH = "master"
SRCREV = "3576dbd5c651c4c92517af83dbd9fdf394a16b22"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=402476d9302b00251cc699d23264b191"

S = "${WORKDIR}/git"
SRC_DIR = "arm"
SRC_DIR:k3 = "armv8_64"

inherit cmake pkgconfig

export http_proxy
export https_proxy

EXTRA_OECMAKE += " -DPVR_WINDOW_SYSTEM=Wayland -DCMAKE_LIBRARY_PATH= -DPVR_BUILD_OPENGLES_EXAMPLES=On -DPVR_BUILD_VULKAN_EXAMPLES=Off"

do_install () {
    CP_ARGS="-Prf --preserve=mode,timestamps --no-preserve=ownership"

    install -d ${D}${bindir}/SGX/demos/Wayland/

    cp ${CP_ARGS} ${WORKDIR}/build/bin/Assets_OpenGLESSkinning ${D}${bindir}/SGX/demos/Wayland/
    cp ${CP_ARGS} ${WORKDIR}/build/bin/OpenGLESSkinning ${D}${bindir}/SGX/demos/Wayland/

    cp ${CP_ARGS} ${WORKDIR}/build/bin/Assets_OpenGLESBinaryShaders ${D}${bindir}/SGX/demos/Wayland/
    cp ${CP_ARGS} ${WORKDIR}/build/bin/OpenGLESBinaryShaders ${D}${bindir}/SGX/demos/Wayland/
}

DEPENDS = "wayland wayland-native wayland-protocols"
RDEPENDS:${PN} = "wayland libegl wayland-protocols"

FILES:${PN} += " \
    /opt/img-powervr-sdk/* \
    ${bindir}/SGX/demos/Wayland/* \
"

```

### systemd

1. 添加systemd服务和自动启动, 参考wpa-supplicant

  ```sh

  S = "${WORKDIR}/${BPN}"

  SRC_URI = " file://pvr.service "

  inherit systemd

  do_install () {

    install -d ${D}/${sysconfdir}/dbus-1/system.d
    install -m 644 ${S}/wpa_supplicant/dbus/dbus-wpa_supplicant.conf ${D}/${sysconfdir}/dbus-1/system.d
    install -d ${D}/${datadir}/dbus-1/system-services
    install -m 644 ${S}/wpa_supplicant/dbus/*.service ${D}/${datadir}/dbus-1/system-services

    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
      install -d ${D}/${systemd_system_unitdir}
      install -m 644 ${S}/wpa_supplicant/systemd/*.service ${D}/${systemd_system_unitdir}
    fi

    install -d ${D}/etc/default/volatiles
    install -m 0644 ${WORKDIR}/99_wpa_supplicant ${D}/etc/default/volatiles

    install -d ${D}${includedir}
    install -m 0644 ${S}/src/common/wpa_ctrl.h ${D}${includedir}

    if [ -z "${DISABLE_STATIC}" ]; then
      install -d ${D}${libdir}
      install -m 0644 wpa_supplicant/libwpa_client.a ${D}${libdir}
    fi
  }

  FILES:${PN} += "${datadir}/dbus-1/system-services/* ${systemd_system_unitdir}/*"

  SYSTEMD_SERVICE:${PN} = "pvr.service"
  SYSTEMD_AUTO_ENABLE:${PN} = "enable"

  ```

1. 禁用某项启动项

  ```sh
  # 1. way
  SYSTEMD_AUTO_ENABLE:${PN} = "disable" # 如果有多个服务项，只想禁用某一个，此法无法针对某一个服务项禁用

  # 2. 使用如下方式禁某一个服务

  do_install:append() {
    rm -rf ${D}${sysconfdir}/systemd/system/sysinit.target.wants/systemd-timesyncd.service
  }

  # 3. 这个可用，上面一个无效
  PACKAGECONFIG:remove = "timesyncd"
  ```

### 手动下载源码包

1. 先手动下载源码包，然后放到`dl`目录下。
2. 再执行`bitbake -c fetch package`, 生成应对包的packagename.done文件。(也可手动创建空白done文件)
3. 这样就可以避免下载源码包了

注: 如果有*.tmp文件，需要手动删除它，不然即使有下载文件，也会报fecth错误。

#### 使用 BB_NO_NETWORK 强制离线构建

如果你已经有完整的源码包，可以在 local.conf 中添加：

```sh
BB_NO_NETWORK = "1"
```

#### 使用PREMIRRORS 或 LOCAL_MIRROR镜像加速下载

```sh
PREMIRRORS:prepend = " \
  git://.*/.* https://mirrors.tuna.tsinghua.edu.cn/git/yocto/ \n \
  https://.*/.* https://mirrors.tuna.tsinghua.edu.cn/yocto/sources/ \n \
"
```

### 修改hostname

在build/conf/local.conf中添加

```sh
#Set the hostname, the value will be written to /etc/hostname in target device
hostname:pn-base-files = "fvt-5g-tbox"
```

### 添加用户密码或免密码登录

1. debug-tweaks作用

在local.conf中设置如下：

```sh
EXTRA_IMAGE_FEATURES += "debug-tweaks"
```

功能|说明
-|-
empty-root-password | 设置 root 用户密码为空，允许无密码登录
allow-empty-password | 允许 Dropbear/OpenSSH 接受空密码登录
post-install-logging | 启用 postinstall 脚本的日志记录，日志保存在 /var/log/postinstall.log
安装调试工具| 包括一些调试工具包，如 tools-debug、dbg-pkgs 等（视配置而定）

1. extrausers作用

构建镜像时添加或修改系统用户和用户组

在local.conf中设置如下：

```sh
INHERIT += "extrausers"
EXTRA_USERS_PARAMS = "\
    usermod -P devpass root; \
    useradd -P dev123 developer; \
    groupadd devgroup; \
    usermod -a -G devgroup developer; \
"
```

命令|说明
-|-
useradd -P <密码> <用户名>|添加用户并设置密码
usermod -P <密码> <用户名>|修改用户密码
groupadd <组名>|添加用户组
usermod -a -G <组名> <用户名>|将用户添加到指定组
userdel <用户名>|删除用户
groupdel <组名>|删除用户组

### overlay-etc

在build/local.conf中添加

```conf
EXTRA_IMAGE_FEATURES += "read-only-rootfs"
EXTRA_IMAGE_FEATURES += "overlayfs-etc"

#  Set configs for overlayfs /etc, under the read only rootfs
OVERLAYFS_ETC_MOUNT_POINT = "/run"
OVERLAYFS_ETC_DEVICE = "tmpfs"
OVERLAYFS_ETC_FSTYPE = "tmpfs"
OVERLAYFS_ETC_EXPOSE_LOWER = "1"
```
