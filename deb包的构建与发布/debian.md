- [参考](#参考)
- [环境变量](#环境变量)
- [从源码开始打包](#从源码开始打包)
  - [1. 生成debian目录](#1-生成debian目录)
  - [2. 修改changelog文件](#2-修改changelog文件)
  - [3. 修改control文件](#3-修改control文件)
  - [4. 打包文件](#4-打包文件)
- [修改已有deb包](#修改已有deb包)
- [包改名](#包改名)
  - [过渡包方法:](#过渡包方法)
- [help](#help)
- [关于deb包的常用命令](#关于deb包的常用命令)
- [dpkg-buildpackage error信息](#dpkg-buildpackage-error信息)
- [appstream](#appstream)
  - [如何生成appstream data](#如何生成appstream-data)
- [错误集](#错误集)
## 参考
- [第 4 章 debian 目录中的必需内容](https://www.debian.org/doc/manuals/maint-guide/dreq.zh-cn.html)

## 环境变量
- 设置 dh_make的邮箱，名称
    ```sh
    $ cat >>~/.bashrc <<EOF
    DEBEMAIL="your.email.address@example.org"
    DEBFULLNAME="Firstname Lastname"
    export DEBEMAIL DEBFULLNAME
    EOF
    $ . ~/.bashrc
    ```
## 从源码开始打包
### 1. 生成debian目录 
```sh
dh_make  --copyright gpl3 --email leixa@jideos.com --createorig -s
dh_make  --copyright gpl3 --email leixa@jideos.com --createorig -l
```
> s:代表单一二进制包， i:代表独立于体系结构的软件包， m:代表多个二进制包， l:代表共享库文件包，k:代表内核模块包， n:代表内核补丁包， b:代表 cdbs 软件包
### 2. 修改changelog文件
```sh
debchange  # 添加说明，修改版本号
```
### 3. 修改control文件
1. 查找依赖添加到Build-Depends中
   - 自动(有configure文件)
        ```sh
        $ dpkg-depcheck -d ./configure
        ```
   - 手工
     - 查看依赖 ： `objdump -p /usr/bin/foo | grep NEEDED`
     - 依据库名找包名 ： `dpkg -S libfoo.so`
     - 将找出的包名的-dev版本放入Build-Depends中
2. 修改软件包关系
   - keywords:
      - Pre-Depends:  
      此项中的依赖强于 Depends 项。软件包仅在预依赖的软件包已经安装并且 正确配置 后才可以正常安装。在使用此项时应 非常慎重，仅当在 debian-devel@lists.debian.org 邮件列表讨论后才能使用。记住：根本就不要用这项。 :-)

      - Depends:（必须安装）   
      此软件包仅当它依赖的软件包均已安装后才可以安装。此处请写明你的程序所必须的软件包，如果没有要求的软件包该软件便不能正常运行（或严重抛锚）的话。 一般添加"${shlibs:Depends}, ${misc:Depends}"即可
 
      - Recommends:（推荐自动安装）  
      这项中的软件包不是严格意义上必须安装才可以保证程序运行。当用户安装软件包时，所有前端工具都会询问是否要安装这些推荐的软件包。aptitude 和 apt-get 会在安装你的软件包的时候**自动安装推荐**的软件包(用户可以禁用这个默认行为)。dpkg 则会忽略此项。

      - Suggests:（建议但不自动安装）  
      此项中的软件包可以和本程序更好地协同工作，但不是必须的。当用户安装程序时，所有的前端程序可能不会询问是否安装建议的软件包。aptitude 可以被配置为安装软件时自动安装建议的软件包，但这不是默认。dpkg 和 apt-get 将忽略此项。

      - Conflicts:（删除冲突包）   
      仅当所有冲突的软件包都已经删除后此软件包才可以安装。当程序在某些特定软件包存在的情况下根本无法运行或存在严重问题时使用此项。

      - Replaces:（替换软件包文件）  
      当你的程序要**替换其他软件包的某些**文件，或是**完全地替换另一个软件包(与 Conflicts 一起使用)**。列出的软件包中的某些文件会被你软件包中的文件所覆盖。

      - Breaks:（强制升级软件包）  
      此软件包安装后列出的软件包将会受到损坏。通常 Breaks 要附带一个“版本号小于多少”的说明。这样，软件包管理工具将会选择**升级被损坏的特定版本的软件包**作为解决方案。

      - Provides:  
      某些类型的软件包会定义有多个备用的虚拟名称。你可以在 virtual-package-names-list.txt.gz文件中找到完整的列表。如果你的程序提供了某个已存在的虚拟软件包的功能则使用此项。

    - 包版本限定：<<、<=、=、>= 和 >>，分别代表严格小于、小于或等于、严格等于、大于或等于以及严格大于,eg如下
        ```sh
        Depends: foo (>= 1.2), libbar1 (= 1.3.4)
        Conflicts: baz
        Recommends: libbaz4 (>> 4.0.7)
        Suggests: quux
        Replaces: quux (<< 5), quux-foo (<= 7.6)
        ```






### 4. 打包文件
- 打包 ：  `dpkg-buildpackage -rfakeroot -tc --no-sign`

## 修改已有deb包
- 解包到指定目录dirname：`dpkg-deb -x name.deb dirname`
- 修改内容
- 重新打包指定目录dirname：`fakeroot dpkg-deb -b dirname name.deb`


## [包改名](https://wiki.debian.org/RenamingPackages)
### 过渡包方法: 
在新包的控制文件中定义一个与旧包同名的二进制虚拟包, 假设旧包“oldname”的最后一个上游版本是 1.5，并且包被重命名为 2.0 版的“newname”,过滤包定义如下：
```debian
Package: oldname
Depends: newname, ${misc:Depends}
Architecture: all
Priority: optional
Section: oldlibs
Description: transitional package
This is a transitional package. It can safely be removed.
```
新包定义时如下：
```debian
 Package: newname
 Replaces: oldname (<< 2.0-1~)
 Breaks: oldname (<< 2.0-1~)
 …
```

## help
```
man deb-control

debchange // 修改日志 
man deb-changelog
```

## 关于deb包的常用命令
1. 查看deb包含有哪些文件
    ```sh
        $ dpkg -c xxx.deb # 安装前根据deb文件查看
        $ dpkg -L debname # 安装后根据包名查看
    ```
1. 查看某个文件属于哪个deb包
    ```sh
        $ dpkg -S /path/to/file
    ```
1. 下载包
    ```sh
        $ apt-get download packagename # 下载包
        $ apt-get source packagename # 下载源码包
    ```
1. 模拟安装远程deb包
    ```sh
        $ apt install -s debname
    ```
1. 安装远程deb包,还可指定版本
    ```sh
        $ sudo apt-get install package=version
    ```
1. 安装本地deb包,还可指明安装路径
    ```sh
        $ sudo dpkg -i xxx.deb
        $ sudo apt-get install -f #如果远程仓库有可用的依赖包,则安装它们
        $ sudo gdebi xxx.deb # 自动安装依赖项
    ```
1. 卸载deb包
    ```sh
        $ sudo apt remove/purge debname #
        $ sudo dpkg -r debname
        $ sudo dpkg -P debname # 完全卸载deb包（包含配置文件)
    ```
1. 根据软件包名模糊检索
    ```sh
        $ apt search xxx #在源中的所有软件包列表中搜索
        $ dpkg -l|grep xxx #在已安装的软件包列表中搜索
        $ apt list --installed |grep xxx #在已安装的软件包列表中搜索
        $ dpkg --get-selections # 所有安装包和已删但未删除配置的包的列表 
        $ dpkg --get-selections | awk '$2 ~ /^install/' # awk 中用!~ 取反
    ```
1. 修改添加源
   ```sh
    sudo apt edit-source [xxx.list]
    sudo apt-add-repository url
   ```
1. 找依赖
   ```sh
    apt depends name.deb
    apt rdepends name.deb
    apt-get build-dep project-dir
   ```
1. 标识为hold, unhold
   ```sh
   apt-mark hold xxx
   apt-mark unhold xxx
   ```
2. 更多命令请看[Ubuntu and Debian Package Management Essentials](https://www.digitalocean.com/community/tutorials/ubuntu-and-debian-package-management-essentials)

## dpkg-buildpackage error信息
1. error: Please specify the compatibility level in debian/compat

    resoled: 修改 debian/control 或 新增debian/compat文件内容即为数字12
    ```sh
    # Build-Depends: debhelper (>= 10.5.1), //error      
    Build-Depends: debhelper-compat (= 12),
    ```

1. error: 正试图覆盖 /usr/share/locale/aa/LC_MESSAGES/*.mo

    ```sh
    dpkg: 处理归档 settingboard-plug-network_1.0.0-1_amd64.deb (--install)时出错：
    正试图覆盖 /usr/share/locale/aa/LC_MESSAGES/networking-plug.mo，它同时被包含于软件包 switchboard-plug-networking 2.4.0+r1159+pkg848~daily~ubuntu6.1
    在处理时有错误发生：
    settingboard-plug-network_1.0.0-1_amd64.deb

    ```
    resoled: 新工程中用到了旧工程同名的翻译文件，检测翻译文件名**GETTEXT_PACKAGE**, **GLib.Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR)** 使用到的名称，如同确实雷同，则考虑在control中添加 Conlicts: uninstall-package,uninstall-package在本包安装时先被remove掉，避免冲突， 否则使用`dpkg -i --force-overwrite`强制安装


1.  error: can't build with source format '3.0 (quilt)'
    ```sh
    dpkg-buildpackage: info: 源码包 settingboard-plug-applications
    dpkg-buildpackage: info: 源码版本 1.0.0-2
    dpkg-buildpackage: info: source distribution unstable
    dpkg-buildpackage: info: 源码修改者 leixa <ann@dell>
    dpkg-buildpackage: info: 主机架构 amd64
    dpkg-source --before-build .
    fakeroot debian/rules clean
    dh clean
    dh_clean
    dpkg-source -b .
    dpkg-source: 错误: can't build with source format '3.0 (quilt)': no upstream tarball found at ../settingboard-plug-applications_1.0.0.orig.tar.{bz2,gz,lzma,xz}
    dpkg-buildpackage: 错误: dpkg-source -b . subprocess returned exit status 255
    ```
    resoled: 更改debian/source/format中的内容为native
    ```diff
    --- a/debian/source/format
    +++ b/debian/source/format
    @@ -1 +1 @@
    -3.0 (quilt)
    +3.0 (native)
    ```

## appstream
 此软件包提供了用于生成、维护和查询已安装和可用软件的 AppStream 数据池的工具，并支持与 APT 软件包管理器的集成。
 > The 'appstreamcli' tool can be used for accessing the software component pool as well as for working with AppStream metadata directly, including validating it for compliance with the specification. 
### 如何生成appstream data


- pkcon : packagekit
- appstreamcli : appstream
- appstream-generator


## 错误集
1. preinst,postinst,prerm, postrm等打包时不会添加到deb包里
    > hese scripts are the control information files preinst, postinst, prerm and postrm. They must be proper executable files; if they are scripts (which is recommended), they must start with the usual #! convention. They should be readable and executable by anyone, and must not be world-writable.
    >           
    >转自[Package maintainer scripts and installation procedure](https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html)
    
    必须是可执行文件！！, 文件名比如是preinst.ex，必须改为preinst
2. gpg: 找不到有效的 OpenPGP 数据
    