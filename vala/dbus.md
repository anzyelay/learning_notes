- [前提概要](#前提概要)
  - [vala中的概念](#vala中的概念)
  - [Type Table](#type-table)
- [client](#client)
  - [编写 Vala D-Bus 接口的规则](#编写-vala-d-bus-接口的规则)
  - [dbus 信号的监听](#dbus-信号的监听)
  - [dbus 方法的引用](#dbus-方法的引用)
  - [dbus 属性的监听](#dbus-属性的监听)
  - [直接使用](#直接使用)
- [server](#server)
  - [编写规则和步骤](#编写规则和步骤)
  - [Dbus服务的属性更改通知](#dbus服务的属性更改通知)
  - [查找一个dbus服务所在](#查找一个dbus服务所在)
    - [找服务脚本](#找服务脚本)
    - [找服务进程](#找服务进程)
- [参考](#参考)
# 前提概要
## vala中的概念
用D-Feet工具查看的对应结构如下：
```
Address:
Name: bus name
Unique name:
|--Match rules
|--Statistics
|--Object path1
|   |--Interfaces
|       |--interface name1
|       |   |--Methods
|       |   |   |--method name1
|       |   |   |--method name2
|       |   |   |--..
|       |   |   |--method nameN
|       |   |--Signals
|       |   |   |--[signal name1]
|       |   |   |--signal name2
|       |   |   |--..
|       |   |   |--signal nameN
|       |   |--Properties
|       |   |   |--[property name1]
|       |   |   |--property name2
|       |   |   |--...
|       |   |   |--property nameN
|       |--interface name2
|       |--...
|       |--inteface nameN
|--Object path2
|--...
|--Object pathN

```
- Bus名： Bus Name (点分隔， eg: org.freedesktop.login1 )
- 对象路径：Object Path (反斜杠分隔， eg: /org/freedesktop/login1 )
- 接口名：interface name （点分隔，eg: org.freedesktop.login1.Manager ）
- 方法名：method name (eg: LockSession (String arg_0) ↦ (), 箭头后面的括号表示返回值和返回类型,前面的括号表示参数类型)

在D-Feet中也可双点方法名在弹出窗口查看上述值, 双击属性得到当前值， 以上四个未说明均是大小写敏感
## Type Table

|D-Bus | Vala | Description | Example | d-feet中使用方法
|:-:|:-:|:-:|-|-|
b | bool | Boolean | | False/True
y |uint8 |	Byte |
i | int | Integer |
u | uint | Unsigned Integer |
n | int16 | 16-bit Integer |
q | uint16 | Unsigned 16-bit Integer |
x| int64| 64-bit Integer |
t| uint64| Unsigned 64-bit Integer |
d| double| Double| | 3.0123
s| string| String| | "strings"
v| GLib.Variant| Variant|  | GLib.Variant("s", "data")、GLib.Variant("i", 3)、GLib.Variant("b", False)等等
o| GLib.ObjectPath| Object Path| 
a| []| Array| ai maps to int[] | eg:ai--> [1,2,3]
a{}| GLib.HashTable<,>| Dictionary| a{sv} maps to HashTable<string, Variant> | eg:a{sv}--> {"key1": GLib.Variant("s", "value"), "key2": GLib.Variant("b", False)} 
()| a struct type| Struct| a(ii) maps to Foo[] where Foo might be defined as struct Foo { public int a; public int b }; A struct representing a(tsav) might look like struct Bar { public uint64 a; public string b; public Variant[] c;} | eg:a(si)-->('sddd',1)

*注：vala中定义struct type后，在使用时不要再加struct修饰符* eg:
```vala
struct DemoStruct {
    string a;
    int b;
    bool c;
}

DemoStruct object;
```

# client
## 编写 Vala D-Bus 接口的规则
- 用 [DBus (name = "...")] 注释接口， name 为 interface name
- 将 DBusCamelCaseNames 转换为 vala_lower_case_names ，（注意连续的大写字母，eg: GetCurrentIM--> get_current_i_m）
- 方法如果要取别名，须在方法上用 [DBus (name = "...")] 注释原DBus中的方法名，（eg: name = "GetCurrentIM"）
- 为每个接口方法添加 throws GLib.Error 或 throws GLib.DBusError, GLib.IOError,信号属性则不用
- 方法，属性，信号必须是public
- [DBus Attribute修饰词](https://wiki.gnome.org/Projects/Vala/Manual/Attributes#DBus_Attribute)

## dbus 信号的监听
- 申明
    ```vala
    [DBus (name = "org.freedesktop.login1.Manager")] //interface name
    interface Manager : Object {
        public signal void prepare_for_sleep (bool sleeping);
    }
    ```
- 创建实例
    ```vala
    Manager manager = Bus.get_proxy_sync (BusType.SYSTEM, "org.freedesktop.login1", "/org/freedesktop/login1");
    ```
- 使用
    ```vala 
    // Listen for the signal that is fired when waking up from sleep, then update time
    manager.prepare_for_sleep.connect ((sleeping) => {
        if (!sleeping) {
            update_current_time ();
            minute_changed ();
            add_timeout ();
        }
    });

    ```
## dbus 方法的引用
- 申明
  ```vala
    [DBus (name = "org.freedesktop.Accounts")] // interface name
    interface FDO.Accounts : Object {
        [DBus (name = "FindUserByName")] // Method name ,可要可不要，当需要取别名时必须要
        public abstract string find_user_by_name (string username) throws GLib.Error;
    }
  ```
- 实例化
    ```vala
    var accounts_service = yield GLib.Bus.get_proxy<FDO.Accounts> (GLib.BusType.SYSTEM,
                                                                    "org.freedesktop.Accounts",
                                                                    "/org/freedesktop/Accounts");
    ```
- 使用
    ```vala
    var user_path = accounts_service.find_user_by_name (GLib.Environment.get_user_name ());
    var proxy = accounts_service as DBusProxy;
    //设置接口默认的超时返回时间。
    proxy.set_default_timeout (300000);
    ```

## dbus 属性的监听
- 申明
    ```vala
    [DBus (name = "io.elementary.pantheon.AccountsService")]
    interface Pantheon.AccountsService : Object {
        public abstract string time_format { owned get; set; }
    }
    ```
- 实例化
  ```vala
  var greeter_act = yield GLib.Bus.get_proxy<Pantheon.AccountsService> (GLib.BusType.SYSTEM,
                                            "org.freedesktop.Accounts",
                                            user_path,
                                            GLib.DBusProxyFlags.GET_INVALIDATED_PROPERTIES);
  ```

- 使用
    ```vala
    ((GLib.DBusProxy) greeter_act).g_properties_changed.connect ((changed_properties, invalidated_properties) => {
        if (changed_properties.lookup_value ("TimeFormat", GLib.VariantType.STRING) != null) {
            bool is_12h = ("12h" in greeter_act.time_format);
        }
    });

    ```
    **注：属性值直接获取时，在获取前必须重新获取下dbus对象，否则只是获取本地的备份值而非真实的代理值**
    ```vala
    var format = greeter_act.time_format; // format="12h"
    ...
    //changed to "24h" on the remote service side
    ...
    format = greeter_act.time_format; // still format="12h"
    greeter_act = yield GLib.Bus.get_proxy<Pantheon.AccountsService> (GLib.BusType.SYSTEM,
                                            "org.freedesktop.Accounts",
                                            user_path,
                                            GLib.DBusProxyFlags.GET_INVALIDATED_PROPERTIES);
    format = greeter_act.time_format; // format="24h"

    ``` 

## 直接使用
```vala
private void on_watch (DBusConnection conn) {
    // Start do something because someone is changing settings
}

private void on_unwatch (DBusConnection conn) {
    // Stop doing 
}

// Listen for the D-BUS server that controls time settings
Bus.watch_name (BusType.SYSTEM, "org.freedesktop.timedate1", BusNameWatcherFlags.NONE, on_watch, on_unwatch);
```
# server
## 编写规则和步骤
1. 定义一个dbus服务类
  - 在类名前加上[DBus (name = "...")] 注明接口名，由于历史遗留原因沿用name
  - 只有修饰为public的属性，信号，方法可导出dbus接口
  - 方法中的GLib.BusName sender形参可在此方法中获取dbus发送者的信息，但在dbus导出接口中不可见
  - 如有不导出某个接口的需求，可用[DBus (visible = false)]修饰;
2. 注册一个服务实例，并启动main loop  
    ```vala
    void on_bus_aquired (DBusConnection conn) {
        try {
            // start service and register it as dbus object
            var service = new DemoService();
            conn.register_object ("/org/example/demo", service);
        } catch (IOError e) {
            stderr.printf ("Could not register service: %s\n", e.message);
        }
    }

    void main () {
        // try to register service name in session bus
        Bus.own_name (BusType.SESSION, "org.example.DemoService", /* name to register */
                    BusNameOwnerFlags.NONE, /* flags */
                    on_bus_aquired, /* callback function on registration succeeded */
                    () => {}, /* callback on name register succeeded */
                    () => stderr.printf ("Could not acquire name\n"));
                                                        /* callback on name lost */

        // start main loop
        new MainLoop ().run ();
    }
    ```
3. 服务按需启动（由另一程序按名称向dbus总线请求来激活服务: service activation）   
- 会话总线（session buses）- [INTEGRATING SESSION SERVICES](https://dbus.freedesktop.org/doc/dbus-daemon.1.html)     
    只需编写"*.service"服务文件；   
    安装到对应目录/usr/share/dbus-1/services,具体有哪些目录可`man dbus-daemon`查看；      
    **服务文件名不强制要求与dbus服务名一样**；    
    **com.example.SessionService1.service** 示例如下：   
    ```service
    [D-BUS Service]
    Name=com.example.SessionService1
    Exec=/usr/bin/example-session-service
    # Optional
    SystemdService=example-session-service
    ```
    如果声明了可选项SystemdService，则必须提供一个systemd的服务文件，示例如下
    ```service
    [Unit]
    Description=Example session service

    [Service]
    Type=dbus
    BusName=com.example.SessionService1
    ExecStart=/usr/bin/example-session-service
    ```
- 系统总线（system buses）- [INTEGRATING SYSTEM SERVICES](https://dbus.freedesktop.org/doc/dbus-daemon.1.html)    
  - 编写策略文件（policy file）    
    >默认情况下，标准系统总线不允许方法调用或拥有众所周知的总线名称，因此有用的 D-Bus 系统服务通常需要配置允许其工作的默认安全策略。  D-Bus 系统服务应在 ${datadir}/dbus-1/service.d 中安装默认策略文件，其中包含使该系统服务正常运行所需的策略规则,一个实践最好的policy文件如下：    
    ```xml
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE busconfig PUBLIC
    "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
    <busconfig>
    <policy user="_example">
        <allow own="com.example.Example1"/>
    </policy>

    <policy context="default">
        <allow send_destination="com.example.Example1"/>
    </policy>
    </busconfig>
    ```
    - *_example*： 是要运行这个服务程序的uid对应的用户名如root;  
    - *com.example.Example1*： 是dbus名称;  
    - 该文件应该命名为: ”com.example.Example1.conf“    

  - 编写服务文件：
    - 同会话总线一样也需要一个服务文件，安装目录/usr/share/dbus-1/system-services，其它目录参见`man dbus-daemon`;
    - DBus名称必须和文件前缀一样，如com.example.Example1的服务文件，则必须将其命名为 com.example.Example1.service。   
        ```service
        [D-BUS Service]
        Name=com.example.Example1
        Exec=/usr/sbin/example-service
        User=_example
        # Optional
        SystemdService=dbus-com.example.Example1.service
        ```
    - 如果声明了可选项SystemdService，则必须提供一个systemd的服务文件，示例如下
        ```service
        [Unit]
        Description=Example service

        [Service]
        Type=dbus
        BusName=com.example.Example1
        ExecStart=/usr/sbin/example-service

        [Install]
        WantedBy=multi-user.target
        Alias=dbus-com.example.Example1.service
        ```
    注： 系统会话的自启动服务用gtk.application会报"org.freedesktop.DBus.Error.Spawn.ChildExited: Launch helper exited with unknown return code 1"的错，改为glib.application就好了

## Dbus服务的属性更改通知
此示例将设置一个 D-Bus 服务，该服务可以发送有关属性更改的通知。  （示例代码部分由 Faheem 提供）,
```vala
[DBus (name = "org.example.Demo")]
public class DemoServer : Object {
  
    public string pubprop { owned get; set; }
  
    private weak DBusConnection conn;
  
    public DemoServer (DBusConnection conn) {
        this.conn = conn;
        this.notify.connect (send_property_change);
    }

    private void send_property_change (ParamSpec p) {
        var builder = new VariantBuilder (VariantType.ARRAY);
        var invalid_builder = new VariantBuilder (new VariantType ("as"));

        if (p.name == "pubprop") {
            Variant i = pubprop;
            builder.add ("{sv}", "pubprop", i);
        }

        try {
            conn.emit_signal (null, 
                              "/org/example/demo", 
                              "org.freedesktop.DBus.Properties", 
                              "PropertiesChanged", 
                              new Variant ("(sa{sv}as)", 
                                           "org.example.Demo", 
                                           builder, 
                                           invalid_builder)
                              );
        } catch (Error e) {
            stderr.printf ("%s\n", e.message);
        }
    }
}

public class NotificationsTest : Object {

    private DemoServer dserver;

    public NotificationsTest () {
        Bus.own_name (BusType.SESSION, "org.example.Demo", BusNameOwnerFlags.NONE,
                      on_bus_acquired, on_name_acquired, on_name_lost);
    }

    private void on_bus_acquired (DBusConnection conn) {
        print ("bus acquired\n");
        try {
            this.dserver = new DemoServer (conn);
            conn.register_object ("/org/example/demo", this.dserver);
        } catch (IOError e) {
            print ("%s\n", e.message);
        }
    }

    private void on_name_acquired () {
        print ("name acquired\n");
    }  

    private void on_name_lost () {
        print ("name_lost\n");
    }

    public void setup_timeout () {
        Timeout.add_seconds (4, () => {
            dserver.pubprop = Random.next_int ().to_string ();
            return true;
        });
    }
}

void main () {
    var nt = new NotificationsTest ();
    nt.setup_timeout ();
    new MainLoop ().run ();
}
```
编译命令如下：
```sh
$ valac --pkg gio-2.0 gdbus-change-notificationst.vala
```
## 查找一个dbus服务所在
### 找服务脚本
/usr/share/dbus-1/services/
### 找服务进程
1. 用D-Feet工具搜索到相应的dbus服务，查看进程pid
2. 查看该进程的执行文件
3. 找出源码包：`dpkg -S  执行文件`

# 参考
1. [Waiting for a DBus service to become available (outdated example)](https://wiki.gnome.org/Projects/Vala/DBusClientSamples/Waiting)
2. [Vala D-Bus Client Examples](https://wiki.gnome.org/Projects/Vala/DBusClientSamples#Waiting_for_a_service_to_become_available_.28outdated_example.29)
3. [DBusServerSample](https://wiki.gnome.org/Projects/Vala/DBusServerSample)
4. [D-Bus_Integration](https://wiki.gnome.org/Projects/Vala/Tutorial#D-Bus_Integration)
5. [dbus-daemon](https://dbus.freedesktop.org/doc/dbus-daemon.1.html)
