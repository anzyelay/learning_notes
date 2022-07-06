# [How Do I](https://wiki.gnome.org/HowDoI)
## 1. [设置应用程序为D-Bus启动](https://wiki.gnome.org/HowDoI/DBusApplicationLaunching)
前提，使用gtk.application的应用。  
其机制有别于fork/exec的方式创建进程来启动，应用程序的启动是通过发送一个D-Bus消息通知已知的应用，从而激活它(D-Bus activation)来完成的，如果应用已经启动，则它可以通过打开一个新窗口或者提升现有的窗口来响应启动消息。
### 1.1 原因
- 确保每个应用程序在其自己的原始环境中启动，作为会话的直接后代，而不是在其父对象的环境中
- 是使用持久通知服务（[GNotification](https://wiki.gnome.org/HowDoI/GNotification)）的先决条件

### 1.2 D-Bus activation激活的两种方式
1. 使用单独的命令行启动器（可能编写为 shellscript）创建专用服务进程（不推荐）
1. 任何使用标准命令行解析机制（即：不覆盖 local_command_line()）的 GApplication 现在都接受`--gapplication-service`开关选项。使用此选项启动时，应用程序将处于服务模式，准备接收导致其激活的消息。
   
### 1.3 实现方法  
#### 1.3.1 没有使用 `local_command_line()`处理命令行参数  
#### 1.3.2 安装一个D-Bus服务文件  
为了让D-Bus识别如何激活你的服务，你必须要安装一个D-Bus服务文件到dbus目录, 以如下一个Makefile文件为例
```Makefile
dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.gnome.clocks.service

org.gnome.clocks.service: Makefile
        $(AM_V_GEN) (echo '[D-BUS Service]'; \
                     echo 'Name=org.gnome.clocks'; \
                     echo 'Exec=${bindir}/gnome-clocks --gapplication-service') > $@.tmp && \
                    mv $@.tmp $@
```
- **Name=**: 必须与你的应用id一样
- **Exec=**: 行必须指向已安装应用程序的位置，并包含`--gapplication-service`开关选项。
#### 1.3.3 修改desktop文件  
完成以上后，尽管以经支持D-Bus激活应用，但任何方式的启动它仍然是使用fork/exec的诞生(spawn)它。你必须直接在desktop文件中使用关键字`DBusActivatable=true`来告之系统此应用支持D-Bus激活方式, 此后启动此应用时将忽略Exec=这一行，而直接发送D-Bus消息来启动，修改方法如下：
- 修改Desktop文件名application_id.desktop
- 在desktop文件中添加`DBusActivatable=true`
  
