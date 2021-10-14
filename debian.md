- [打包命令](#打包命令)
- [help](#help)
- [关于deb包的常用命令](#关于deb包的常用命令)
- [dpkg-buildpackage error信息](#dpkg-buildpackage-error信息)
## 打包命令
```sh
dh_make  --copyright gpl3 --email leixa@jideos.com --createorig -s
dpkg-buildpackage -rfakeroot -tc --no-sign
```
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

1. 安装deb包
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
    resoled: 新工程中用到了旧工程同名的翻译文件，检测翻译文件名**GETTEXT_PACKAGE**, **GLib.Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR)** 使用到的名称，如同确实雷同，则考虑在control中添加 Conlicts: uninstall-package,uninstall-package在本包安装时先被remove掉，避免冲突


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