## 打包命令
```sh
dh_make  --copyright gpl3 --email leixa@jideos.com --createorig -s
dpkg-buildpackage -rfakeroot -tc --no-sign
```

## dpkg-buildpackage 
1. error: Please specify the compatibility level in debian/compat

    resoled:修改 debian/control 或 新增debian/compat文件内容即为数字12
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
   新工程中用到了旧工程同名的翻译文件，翻译文件的指定地址在appdata.xml中的
    `<translation type="gettext">networking-plug</translation>`字段