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
