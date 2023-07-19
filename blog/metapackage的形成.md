# metapackage的形成

## 一、参考工程

- seeds
- metapackages

## 二、命令及文件说明

### 2.1 涉及的命令

- germinate
- germinate-update-metapackage

### 2.2 germinate

此小节对应seeds工程
Germinate 是 Debian 中可用的一个包，它以包列表（称为种子）开始，并将它们发展成一个完整的包列表，包括依赖项和（在附加列表中）每个列表的建议、推荐和来源。
> The minimum set of seeds this author has used is(也可以再排除一些文件):  
       STRUCTURE  
       required  
       minimal  
       standard  
       custom  
       blacklist  
       supported

- 包列表中总一个特殊的文件叫**STRUCTURE**, 该文件的特点如下：
  - 它实际上不是包列表，而是用来说明种子列表以及它们如何相互依赖。
  - 每一行都由一个种子名加“：”号组成，冒号后面是一个空格和一个空格分隔的种子列表，表明当前种子依赖哪些种子构成  
  - 以**include**开头的可以引入其它种子列表,如`include platform.focal`表示引入platform工程的focal分支作为种子列表
  - 以**feature**开头设置了处理种子工程的标识, 当前只定义了一个标识，即‘follow-recommends’,指示germinate把Recommends字段当作Depends处理, 在每个种子文件中又可以‘* Feature: no-follow-recommends’关闭单个种子文件的标识。
- blacklist：一个特殊的种子文件，它总是没有依赖种子，相反，它列出了永远不会包含在germinate输出中的包。
- seed-file种子文件如下:
  - 不以‘*’开头的行都会被忽略
  - pkgname [arch1 arch2 !arch3]: 指明只有在给定的arch1, arch2下才会用到，也可以用"!"取反
  - (pkgname):指示从该种子文件生成的metapackages将该包当作推荐项而不是依赖项处理,后面生成metapackages时会包含在seed-recommmends-arch中
  - !pkgname: 指示此包在seed文件中为backlisted处理
  - snap:pkgname: 说明该包为snap包
  - "key: value" : 一些种子文件的顶部可能会有此格式的字段，在大部分情况在germinate本身不解析它们，
  - %pkgname: 从pkgname扩展出所有二进制包

    ```txt
    Task-Per-Derivative: 1
    Task-Section: user

    # no space on powerpc
    * Languages: es
    * language-pack-${Languages} [i386 amd64 amd64+mac]
    * language-pack-gnome-${Languages} [i386 amd64 amd64+mac]

    == Installer ==

    * bootstrap-base
    * oem-config-gtk
    * oem-config-slideshow-ubuntu
    * uboot-mkimage [armel]
    * partman-iscsi

    == Blacklist ==
    libavcodec cannot be shipped on CDs (cf. Ubuntu technical board resolution 2007-01-02).
    * !libavcodec*
    ```

### 2.3 germinate-update-metapackage

此小节对应metapackages工程
利用seeds(种子包工程)构造更新metapackages, 更新在debian/control指定的二进制包，反映当前种子工程中的最新内容，并且修改debian/changelog的描述信息。 在执行此命令的当前目录下需要一个名为update.cfg的配置文件。配置文件说明如下：

- *注: #字段为解释说明添加，update.cfg里是没有的*  
- %(value)s : 用来指代value的值

```sh
[DEFAULT]
#指定发布名称字段,distribution，每个dist的值在后面都要有一个区段对其说明，如下面[focal]
dist: focal

[focal]
#告之germinator只处理如下seeds的值.
seeds: minimal standard desktop artwork pantheon-shell pantheon live elementary-sdk
#为如下架构生成metapackages
architectures: amd64 arm64

#从如下URL基址中获取seeds工程，多个地址使用空格或分号分隔。此处可以不用指定，在[dist/bzr],[dist/vcs]指定
# seed_dist为seed_base的末尾字段，最右边的.号后面了字符如果是git，则代表是要checkout到哪个branch,
# 如下真实地址为 http://192.168.16.188:8080/patapua/seeds.git,并切换到focal分支获取数据
seed_base: http://192.168.16.188:8080/patapua/
seed_dist: seeds.%(dist)s

# 自定义值, 后面用%()s来获取
patapua_ppas: http://192.168.16.174/patapua/daily/ubuntu/,http://192.168.16.174/patapua/os-patches/ubuntu/,http://192.168.16.174/patapua/stable/ubuntu/
patapua_ports_ppas: http://192.168.16.174/patapua-ports/daily/ubuntu/,http://192.168.16.174/patapua-ports/os-patches/ubuntu/

# 默认的获取包的档案仓库的URL基地址, 与/etc/apt/sources.list一样，或者是debootstrap的MIRROR参数
archive_base/default: http://archive.ubuntu.com/ubuntu/,%(patapua_ppas)s
#archive_base/arch指定获取相应arch的包的档案仓库基地址
archive_base/ports: http://ports.ubuntu.com/ubuntu-ports/,%(patapua_ports_ppas)s
archive_base/arm64: %(archive_base/ports)s

# 从档案仓库哪个组件获取包
components: main restricted universe

#[dist/bzr]或[dist/vcs]区可以覆盖上面的seed_base,seed_dist的值
[focal/vcs]
seed_base: http://192.168.16.188:8080/patapua/
seed_dist: seeds.%(dist)s

```

在配置文件下创建如下执行脚本, eg: update.sh

```sh
#! /bin/sh
if ! which dch >/dev/null; then
 echo >&2 "please install devscripts"
 exit 1
fi

if ! which debootstrap >/dev/null; then
 echo >&2 "please install debootstrap"
 exit 1
fi

exec germinate-update-metapackage --vcs
```

如果seeds工程和应对arch的包都配置正常发，执行上述脚本后即可更新生成metapackages
生成的文件名称构成由seeds和architectures以及是推荐包还是依赖包构成，如下：

- minimal-recommends-arm64 ：minimal种子文件中在“()”内的包都归集到处些
- minimal-arm64 ： minimal种子文件中的依赖包都归集到处些
