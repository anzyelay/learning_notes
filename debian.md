- [参考](#参考)
- [环境变量](#环境变量)
- [打包命令](#打包命令)
  - [1. 生成debian目录](#1-生成debian目录)
  - [2. 修改changelog文件](#2-修改changelog文件)
  - [3. 修改control文件](#3-修改control文件)
  - [4. 打包文件](#4-打包文件)
- [help](#help)
- [关于deb包的常用命令](#关于deb包的常用命令)
- [dpkg-buildpackage error信息](#dpkg-buildpackage-error信息)
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
## 打包命令
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

      - Depends: 
      此软件包仅当它依赖的软件包均已安装后才可以安装。此处请写明你的程序所必须的软件包，如果没有要求的软件包该软件便不能正常运行（或严重抛锚）的话。 一般添加"${shlibs:Depends}, ${misc:Depends}"即可

      - Recommends:
      这项中的软件包不是严格意义上必须安装才可以保证程序运行。当用户安装软件包时，所有前端工具都会询问是否要安装这些推荐的软件包。aptitude 和 apt-get 会在安装你的软件包的时候**自动安装推荐**的软件包(用户可以禁用这个默认行为)。dpkg 则会忽略此项。

      - Suggests:
      此项中的软件包可以和本程序更好地协同工作，但不是必须的。当用户安装程序时，所有的前端程序可能不会询问是否安装建议的软件包。aptitude 可以被配置为安装软件时自动安装建议的软件包，但这不是默认。dpkg 和 apt-get 将忽略此项。

      - Conflicts:
      仅当所有冲突的软件包都已经删除后此软件包才可以安装。当程序在某些特定软件包存在的情况下根本无法运行或存在严重问题时使用此项。

      - Replaces:
      当你的程序要替换其他软件包的某些文件，或是完全地替换另一个软件包(与 Conflicts 一起使用)。列出的软件包中的某些文件会被你软件包中的文件所覆盖。

      - Breaks:
      此软件包安装后列出的软件包将会受到损坏。通常 Breaks 要附带一个“版本号小于多少”的说明。这样，软件包管理工具将会选择升级被损坏的特定版本的软件包作为解决方案。

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


## help
```
man deb-control

debchange // 修改日志 
man deb-changelog
```

## 关于deb包的常用命令
1. 查看deb包含有哪些文件
    ```
        $ dpkg -c xxx.deb # 安装前根据deb文件查看
        $ dpkg -L debname # 安装后根据包名查看
    ```

1. 安装deb包,还可指明安装路径
    ```
        $ dpkg -i xxx.deb
    ```
1. 查看某个文件属于哪个deb包
    ```
        $ dpkg -S filefullpath
    ```
1. 卸载deb包
    ```
        $ dpkg -r debname
        $ dpkg -P debname # 完全卸载deb包（包含配置文件)
    ```
1. 下载源码包
    ```
        $ apt-get source packagename
    ```

1. 根据软件包名模糊检索
    ```
        $ dpkg -l|grep xxx #在已安装的软件包列表中搜索
        $ apt-cache search xxx #在源中的所有软件包列表中搜索
    ```


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
