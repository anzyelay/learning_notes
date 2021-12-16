
## 一、 What is D-Bus ?
### 1.1 为什么需要？
它是专门为满足安全和系统内IPC需求而量身定制的简单、统一的协议，旨在用来取代该领域中其它IPC框架的竞争者。
>It is intended to replace the amalgam of competing IPC frameworks in this domain with a single, unified protocol that is tailored specifically for meeting the needs of secure, intra-system IPC.  参考:[DBus Overview](https://pythonhosted.org/txdbus/dbus_overview.html)


### 1.2 D-bus是什么 ？
- 狭义讲：(man dbus-daemon)
    >D-Bus is first a
        library that provides one-to-one communication between any two applications, dbus-daemon is an application that uses this library to implement a message
        bus daemon. Multiple programs connect to the message bus daemon and can exchange messages with one another.

- 广义上： 原文见[参考1](https://freedesktop.org/wiki/Software/dbus/)

    1. D-Bus是一种消息总线系统，是应用程序相互通信(IPC)的一种简单方式。
    2. 用于协调进程生命周期,简化编写“单实例”应用程序或守护程序，使得在需要服务时按需启动应用程序和守护程序变得简单而可靠。

        *对针2说明:*
        ```sh
        ann@dell:~$ cat /usr/share/dbus-1/services/io.elementary.files.Filemanager1.service 
        [D-BUS Service]
        Name=org.freedesktop.FileManager1
        Exec=/usr/bin/io.elementary.files-daemon
        ```

### 1.3 提供了哪些通信方式 ?
通信方式|解释
|--|--|
|one-to-one通信|应用间直接点对点通信，无需消息总线守护进程
|D-Bus消息总线|依靠支持RPC（remote procedure calling）和发布订阅机制的守护进程来实现的通信

消息总线daemon的两个主要用例：
1. system daemon：通常在系统init脚本中启动， 广泛用于广播系统事件，如:"new hardware device added"或"printer queue changed"
1. per-user-login daemon：针对用户应用间的一般通信需求,或者说桌面环境下的应用间通信

![diagram](https://dbus.freedesktop.org/doc/diagram.svg)

### 1.4 有哪些实现库 ？
#### 1.4.1 low-level libdbus reference library
> libdbus is part of dbus, and is the reference implementation of the D-Bus protocol.
**特点：除依赖XML解析器外，没有其它必需的依赖项。**
#### 1.4.2 binding for libdbus参考实现(Higher-level API)
- QtDBus
- Eldbus
- dbus-cxx
- dbus-c++
- DBus-GLib（已废弃）
- ...
#### 1.4.3 非基于libdbus参考,重写实现D-Bus protocol的
- GDBus: part of GNOME's GLib library
- sd-bus: part of libsystemd
- ...
   

-------------
## 二、 [GDBus](https://docs.gtk.org/gio/migrating-gdbus.html)
### GDBus vs dbus-glib
||GDBus|dbus-glib|
|-|-|-|
基于libdbus|no<br>依赖 GIO 流作为传输层，并有自己的D-Bus连接设置<br>和身份验证实现|yes<br>存在多线程问题
函数参数和<br>返回值类型系统|GVariant type system(match D-Bus types) |GObject type system
models|D-Bus interfaces<br>objects|D-Bus interfaces
native support|org.freedesktop.DBus.Properties<br>org.freedesktop.DBus.ObjectManager | no
从XML内省数据生成粘贴代码工具| gdbus-codegen（支持生成接口文档） | dbus-binding-tool
方便的own/watch API|g_bus_own_name()<br>g_bus_watch_name()|no
对XML的协作支持|yes|no
独立的单元测试|yes<br>GTestDBus|no

### 具体使用
主要针对vala中的用法如下：



## DBus Overview


-----
## 参考
1. [https://freedesktop.org/wiki/Software/dbus/](https://freedesktop.org/wiki/Software/dbus/)
