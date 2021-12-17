- [语法](#语法)
  - [类型定义](#类型定义)
  - [属性的定义](#属性的定义)
  - [几个修饰符的说明](#几个修饰符的说明)
    - [ownership](#ownership)
- [Gtk](#gtk)
  - [一 窗口属性](#一-窗口属性)
    - [label & Granite.Widget.AlerView](#label--granitewidgetalerview)
    - [其它](#其它)
  - [二 对象](#二-对象)
  - [调试](#调试)
    - [运行时使能调试输出](#运行时使能调试输出)
  - [网络](#网络)
    - [NetworkMonitor 参见appcenter](#networkmonitor-参见appcenter)
  - [文件](#文件)
  - [进程相关](#进程相关)
  - [command line args](#command-line-args)
  - [信号事件](#信号事件)
  - [apt 安装(参考语言插件中的aptd-client.vala)](#apt-安装参考语言插件中的aptd-clientvala)
# 语法
## 类型定义
- vala中的enum, struct申明和c/c++相同，但没有typedef一说，使用时无需加enum,struct修饰符
  ```vala
    public struct LoginDisplay {
        string session;
        GLib.ObjectPath objectpath;
    }

    [DBus (name = "org.freedesktop.login1.User")]
    interface LoginUserManager : Object {
        public abstract LoginDisplay? display { owned get; }
    }

    [CCode (type_signature = "u")]
    enum PresenceStatus {
        AVAILABLE = 0,
        INVISIBLE = 1,
        BUSY = 2,
        IDLE = 3
    }

    [DBus (name = "org.gnome.SessionManager.Presence")]
    interface SessionPresence : Object {
        public abstract PresenceStatus status { get; }
        public signal void status_changed (PresenceStatus new_status);
    }
  ```
## 属性的定义
- set,get
    ```vala
    public string description {
        get {
            return description;
        }
        set {
            description = value; // value是关键字，代表传进来的设置值，可以先判断是否为空
        }
    }
    ```
## 几个修饰符的说明
### [ownership](https://wiki.gnome.org/Projects/Vala/Tutorial#Ownership)
1. Unowned References: 
   > 通常在 Vala 中创建对象时，返回给你的是对它的引用。  具体来说，这意味着除了传递一个指向内存中对象的指针外，还在对象本身中记录了该指针的存在。  类似地，每当创建针对该对象的另一个引用时，对象本身也会记录此引用。  由于一个对象知道有多少对它的引用，它可以在需要时自动删除。  这是 Vala 内存管理的基础。

   强烈建议看下[Vala's Memory Management Explained](https://wiki.gnome.org/Projects/Vala/ReferenceHandling)中的Unowned References
    > Vala类的所有对象和大部分gobject-based libraries对象是引用计数的（are reference counted）,但是，Vala 也允许您使用默认情况下不支持引用计数的非基于 gobject 的 C 库类。  这些类称为紧凑类（用 [Compact] 属性注释）。
    > 非引用计数对象可能只有一个强引用（将其视为“owned”引用）。当此引用超出范围时，对象将被释放。所有其他引用必须是unowned引用。当这些引用超出范围时，对象将不会被释放。
    >
    > 因此，当您调用一个方法返回一个无主引用（这在方法的返回类型上用 unowned 关键字标记）到您感兴趣的对象时，您有两种选择：要么复制对象，如果它有复制方法，那么你可以有自己对新复制对象的单一强引用，要么将引用分配给一个用 unowned 关键字声明的变量，Vala会知道它在你这一则不应该释放引用的对象。
    >
    > Vala阻止您将无主引用分配给强（即非无主）引用。 但是，您可以使用 (owned) 将原先拥有者的所有权转让给另一个引用（注:原拥有者变为无主引用不再处理对象的释放）

   - 无主引用(unowned references)
     - 不会在其引用对象中存留引用记录
     - 引用超出定义范围,引用对象不被释放;
   - 有主引用(owned references,或强引用)：
     - 引用记录会在引用对象中存留
     - 引用超出定义范围,引用对象将被释放掉;

    ```mermaid
    graph BT
    ca(("被引用对象<br>引用计数：1"))
    cb(("被引用对象<br>引用计数：4"))

    a[unowned] 
    b[unowned] 
    c[unowned]
    d[owned]

    f[owned]
    g[owned]
    h[owned]
    i[owned]
    a & b & c  -.->ca
    d  -->ca

    f & g & h & i -->cb

    ```

2. Methods ownership:
    > 无主引用反过来不会记录在它们引用的对象中。这允许在逻辑上应该删除对象时删除它，而不管可能仍然存在对它的引用的情况。实现此目的的常用方法是使用定义为返回无主引用的方法

    自己的理解：一般的普通函数返回的是有主引用。
3. Properties ownership:
    > 与普通方法相反，属性总是返回无主的值
    自己的理解： 属性的拥有者必须是对象本身，所以get方法里不可能new一个属性返回
4. Ownership Transfer:
    - 作为参数类型的前缀，它意味着对象的所有权被转移到这个代码上下文中。
    - 作为类型转换运算符，它可以用来避免复制（duplicate）非引用计数类，这在 Vala 中通常是不可能的。例如
        ```vala
        Foo foo = (owned) bar;//这意味着bar将被设置为null,并且foo继承对象bar引用的引用/所有权。
        ```
        ```mermaid
        graph RL
        o((object))

        bar -- X --> o
        foo --> o

        ```

# Gtk 
## 一 窗口属性
### label & Granite.Widget.AlerView
    在AlerView中的title和description的label如果不设置width_request，即使wrap=true了，高度依然会非常高。

### 其它
1. pass_through：输入事件可否过滤到下层窗口

2. 窗口大小相关
```
get_position (out root_x, out root_y);获取窗口位置
set_size_request (width,height);//设置最小需求，包涵margin,alignment区域，如果后面几种设置方法设置的大小比这个小，那么get_allocation获取的值就是此值，否则就以其它设置方法的大小为准
set_default_size (rect.width,rect.height);//设置main_window窗口初始大小。
set_allocation(rect);//main_window设置子窗口区域的大小，设置值可能被子窗口的虚方法adjust_size_allocation校准，（默认是去掉margins,alignments等），所以此设置可能导致main_window的绘图区变大;
get_allocation (out rect);//获取程序绘图区域的大小,包涵不可见的区域（如margin, alignment）随区域的更改而更改。
get_default_size (out rect.width, out rect.height);//一直是set_default_size设置的值，不随窗口变化而变。
get_size_request;//一直是set_size_request设置的值，不随窗口变化而变。
get_size (out rect.width, out rect.height);//用户真正看到的main_window大小，窗口大小改变，其取值也变化，但比allocation那个要小。
```
设置窗口大小使用如下方式：
```vala
//main_window.set_size_request(x,y);//如何使用了此方法则必须满足x<800,y<600；否则width,height可能会比800，600大一点，但也不是，x,y的值， get_allocation才是它的值
main_window.set_default_size(800, 600);
main_window.get_allocation (out width, out height);//width=898, height=608
main_window.get_size (out width, out height);// width=800, height=600
```        
改变窗口大小：
- 要使resize生效，前提条件是default_size比resize小,即把默认值当最小值看待，无法变小了。
- 呈现出的窗口大小以主窗口和所有子控件（包括grid等）的最大值为显示值。如果要改变大小，必须把比它大的所有控件改小
```vala
//确保没有子窗口的大小大于800*600
main_window.set_default_size(800, 600);//先改变默认值,只能比resize的尺寸小
main_window.resize(800, 600);//
```

**重载get_prefered_width等等可以控制窗体大小**


3. 控件在布局容器中自适应填充扩大的解决方法，设置下align属性，默认为Align.FILL,所以会填充满
4. 获取焦点：`grab_focus()`
5. 拖动信号: `drag_data_received`
    
    
## 二 对象
1. Gtk.Overlay： 是一个容器，它包含一个主子项，它可以在其上放置“覆盖”小部件。（相当于背景与前景）
2. GLib.DesktopAppInfo,AppInfo:lanch方法启动程序
    ```vala
    var initial_setup = AppInfo.create_from_commandline ("io.elementary.initial-setup", null, GLib.AppInfoCreateFlags.NONE);
    initial_setup.launch (null, null);
    ```
3. GLib.Application.open: 
    ```vala        
    public override void open (File[] files, string hint) {
        var file = files[0];
        if (file == null) {
            return;
        }
        warning ("open file --> %s ,and hint:%s", file.get_uri (), hint);

        if (file.get_uri_scheme () == "settings") {
            link = file.get_uri ().replace ("settings://", "");
            if (link.has_suffix ("/")) {
                link = link.substring (0, link.last_index_of_char ('/'));
            }
        } else {
            critical ("Calling Switchboard directly is unsupported, please use the settings:// scheme instead");
        }

        activate ();
    }
    ```
    如何触发open信号？？如下
    - 方法一、通用Gtk.LinkButton：
                ```var kb_settings_label = new Accessibility.Widgets.LinkLabel (_("Keyboard settings…"), "settings://input/keyboard/behavior");```
    - 方法二、AppInfo.launch_default_for_uri ("settings://input/keyboard/layout", null);
            
        
5. download_file
    ```vala
        private async string? download_file (Fwupd.Device device, string uri) {
            var server_file = File.new_for_uri (uri);
            var path = Path.build_filename (Environment.get_tmp_dir (), server_file.get_basename ());
            var local_file = File.new_for_path (path);

            bool result;
            try {
                result = yield server_file.copy_async (local_file, FileCopyFlags.OVERWRITE, Priority.DEFAULT, null, (current_num_bytes, total_num_bytes) => {
                // TODO: provide useful information for user
                });
            } catch (Error e) {
                show_error_dialog (device, "Could not download file: %s".printf (e.message));
                return null;
            }

            if (!result) {
                show_error_dialog (device, "Download of %s was not succesfull".printf (uri));
                return null;
            }

            return path;
        }  
    ```
6. shut down
    ```vala
    [DBus (name = "org.freedesktop.login1.Manager")]
    public interface About.LoginInterface : Object {
        public abstract void reboot (bool interactive) throws GLib.Error;
        public abstract void power_off (bool interactive) throws GLib.Error;
    }

    public class About.LoginManager : Object {
        private LoginInterface interface;

        static LoginManager? instance = null;
        public static LoginManager get_instance () {
            if (instance == null) {
                instance = new LoginManager ();
            }

            return instance;
        }

        private LoginManager () {}

        construct {
            try {
                interface = Bus.get_proxy_sync (BusType.SYSTEM, "org.freedesktop.login1", "/org/freedesktop/login1");
            } catch (Error e) {
                warning ("Could not connect to login interface: %s", e.message);
            }
        }

        public bool shutdown () {
            try {
                interface.power_off (true);
            } catch (Error e) {
                warning ("Could not connect to login interface: %s", e.message);
                return false;
            }

            return true;
        }

        public bool reboot () {
            try {
                interface.reboot (true);
            } catch (Error e) {
                warning ("Could not connect to login interface: %s", e.message);
                return false;
            }

            return true;
        }
    }  
    ```
8. 连接按键不能切换页面的问题
    ```vala
    stack.visible_child = software_grid;//显示 1
    update_button.clicked.connect (() => {
        stack.visible_child = hbox_checking;//显示2
        Timeout.add (100, ()=>{ //如果不用timeout，虽也会进来，但不显示2页面
            check_for_updates.begin((obj, res)=>{
                var result = check_for_updates.end (res);//check_for_updates完成后进入，
                if(result){
                    stack.visible_child = software_grid;//切换回 1
                }
            });
            return false;// 不重复调用
        } );
    });  
    private async bool check_for_updates () {
        GLib.Thread.usleep(100000);
        return true;
    }

    ```
9. list dir
    ```vala
    async void list_dir () {
        var dir = File.new_for_path (Environment.get_home_dir ());
        try {
            var e = yield dir.enumerate_children_async (GLib.FileAttribute.STANDARD_NAME, 0);
            while (true) {
                var files = yield e.next_files_async (10, Priority.DEFAULT, null);
                if (files == null) {
                    break;
                }
                foreach (var info in files) {
                    print ("%s\n", info.get_name ());
                }
            }
        } catch (GLib.Error err) {
            warning ("Error: %s\n", err.message);
        }
    }
    ```

10. Gtk.InfoBar
    - message_type : 消息类型:info,waring,question,error,other
    - revealed: 显示与否
    - get_content_area ().add (default_label); // 显示信息
    - add_button (btn, response_id);
    - response(response_id): button的回应 
    ```vala
    public class AppCenter.Widgets.NetworkInfoBar : Gtk.InfoBar {
        public NetworkInfoBar () {
            Object (
                message_type: Gtk.MessageType.WARNING
            );
        }

        construct {
            string title = _("Network Not Available.");
            string details = _("Connect to the Internet to browse and install apps.");

            var default_label = new Gtk.Label ("<b>%s</b> %s".printf (title, details));
            default_label.use_markup = true;
            default_label.wrap = true;

            get_content_area ().add (default_label);
            // TRANSLATORS: Includes an ellipsis (…) in English to signify the action will be performed in a new window
            add_button (_("Network Settings…"), Gtk.ResponseType.ACCEPT);

            try_set_revealed ();

            response.connect ((response_id) => {
                switch (response_id) {
                    case Gtk.ResponseType.ACCEPT:
                        try {
                            AppInfo.launch_default_for_uri ("settings://network", null);
                        } catch (GLib.Error e) {
                            critical (e.message);
                        }
                        break;
                    default:
                        assert_not_reached ();
                }
            });

            var network_monitor = NetworkMonitor.get_default ();
            network_monitor.network_changed.connect (() => {
                try_set_revealed ();
            });
        }

        private void try_set_revealed (bool? reveal = true) {
            var network_available = NetworkMonitor.get_default ().get_network_available ();
            revealed = reveal && !network_available;
        }
    }  
    ```
1. Gtk.Revealer:显示容器，控制子部件是否显示

1. 单例模式
    ```
    private static GLib.Once<T> instance;
    public static unowned T get_default () {
        return instance.once (() => { return new T (); });
    }  
    ```
1. 默认程序设置 ：GLib.AppInfo  
    - set_as_default_for_type(MINE-TYPE); 
    - get_default_for_type(mine-type)

1. 获取Application
    `unowned SwitchboardApp app = (SwitchboardApp) GLib.Application.get_default ();`

1. 延时处理
    ```vala
    //使用定时器
    async void delay(int usec) {
        Timeout.add (usec, ()=>{
            delay.callback;//
            return false;
        }); 
        yield;//delay until callback be called
    }
    //使用线程
    async void delay(int usec) {
        new Thread<void>(()=>{
            Thread.usleep(usec);
            delay.callback;//
            return;
        }); 
        yield;//delay until callback be called
    }
    
    ```

## 调试
### 运行时使能调试输出
`G_MESSAGES_DEBUG=all io.elementary.appcenter`

`GLib.Environment.get_variable ("G_MESSAGES_DEBUG");`

## 网络
### NetworkMonitor 参见appcenter
`NetworkMonitor.get_default ().get_network_available ();`:是否有网络

## 文件
1. FileUtils
2. KeyFile: 配置文件读写类
- KeyFileDesktop: 用于.desktop文件的宏
- 判断文件是否存在
```vala
var config_dir = Environment.get_user_config_dir ();
var startup_dir = Path.build_filename (config_dir, AUTOSTART_DIR);

if (FileUtils.test (startup_dir, FileTest.EXISTS) == false) {
    var file = File.new_for_path (startup_dir);

    try {
        file.make_directory_with_parents ();
    } catch (Error e) {
        warning (e.message);
    }
}

return startup_dir;   
```
- 写KeyFile文件内容,参见应用插件
`GLib.FileUtils.set_contents (path, keyfile.to_data ());`

- 一般文件的读写：Posix.open/read/fcntl/  监控文件的变化使用 IOChannel.unix_new(fd).add_watch
- example 
  ```vala
      public GtkSettings () {
        keyfile = new GLib.KeyFile ();

        path = GLib.Environment.get_user_config_dir () + "/gtk-3.0/settings.ini";

        if (!(File.new_for_path (path).query_exists ())) {
            return;
        }

        try {
            keyfile.load_from_file (path, 0);
        } catch (Error e) {
            warning ("Error loading GTK+ Keyfile settings.ini: " + e.message);
        }
    }
  ```

1. GLib.Filename 获取文件basename, display_name
2. 获取文件类型
```vala
    private string get_mime_type (string uri) {
    try {
        return ContentType.guess (Filename.from_uri (uri), null, null);
    } catch (ConvertError e) {
        warning ("Error converting filename '%s' while guessing mime type: %s", uri, e.message);
        return "";
    }
}
```
5. 获取/etc/passwd信息
```vala
public uid_t getuid () ;
public unowned Passwd? getpwuid (uid_t uid);

public class Passwd {
    public string pw_dir
    public string pw_gecos
    public gid_t pw_gid
    public string pw_name
    public string pw_passwd
    public string pw_shell
    public uid_t pw_uid 
}
```
6. file monitor
```vala
var file = File.new_for_path (filename);
try {
    var monitor = file.monitor (FileMonitorFlags.NONE, null);
    monitor.changed.connect (() => {
        file_changed (filename);
    });
} catch (Error e) {
    warning ("Failed to monitor %s: %s", filename, e.message);
}
```
7. 文件前后缀名判断
```vala
string filename;
filename.has_suffix (".xml")
filename.has_prefix ("file://")

```

## 进程相关
1. `Posix.getuid ()`
2. 执行进程
   ```vala
   private const string LANGUAGE_CHECKER = "/usr/bin/check-language-support"
    private string[]? get_remaining_packages_for_language (string langcode) {
        string output;
        int status;
        try {
            Process.spawn_sync (null,
                {LANGUAGE_CHECKER, "-l", langcode.substring (0, 2) , null},
                Environ.get (), SpawnFlags.SEARCH_PATH, null,
                out output,
                null,
                out status);
        } catch (Error e) {
            warning ("Could not get remaining language packages for %s", langcode);
        }

        return output.strip ().split (" ");
    }
   ```
3. GLib.Subprocess执行system命令行
    ```vala
    private async string get_partition_name () {
        string df_stdout;
        string partition = "";
        try {
            var subprocess = new GLib.Subprocess (GLib.SubprocessFlags.STDOUT_PIPE, "df", "/");
            yield subprocess.communicate_utf8_async (null, null, out df_stdout, null);
            string[] output = df_stdout.split ("\n");
            foreach (string line in output) {
                if (line.has_prefix ("/dev/")) {
                    int idx = line.index_of (" ");
                    if (idx != -1) {
                        partition = line.substring (0, idx);
                        return partition;
                    }
                }
            }
        } catch (Error e) {
            warning (e.message);
        }

        return partition;
    }  
    ```


## command line args
1. GLib.OptionContext
2. Glib.Application类handle_local_options方法重载
   ```vala
   public class SettingsDaemon.Application : GLib.Application {
        // step 1: define options
        public const OptionEntry[] OPTIONS = {
            { "version", 'v', 0, OptionArg.NONE, out show_version, "Display the version", null},
            { null }
        };
        private Application () {}

        construct {
            application_id = Build.PROJECT_NAME;
            // step 2: add options
            add_main_option_entries (OPTIONS);
        }
        //step 3: handle options
        public override int handle_local_options (VariantDict options) {
            if (show_version) {
                stdout.printf ("%s\n", Build.VERSION);
                return 0;
            }
            return -1;
        }

        public static int main (string[] args) {
            var application = new Application ();
            return application.run (args);
        }

   }
   ```

## 信号事件
1. 属性是可以被监听的
  - 定义： 
    ```vala
    public bool clock_show_seconds { get; set; }
    ```
      **能被监听的属性必须是public类型, 命名时用下划线分隔**

   - 属性的监听: 
        ```vala
        clock_settings.bind ("clock-show-seconds", this, "clock-show-seconds", SettingsBindFlags.DEFAULT);

        notify["clock-show-seconds"].connect (() => {
            add_timeout ();
        });
        ```

       **在信号连接中属性名中的所有的下划线 "_" 都要替换成 "-"**

## apt 安装(参考语言插件中的aptd-client.vala)
```vala
const string APTD_DBUS_NAME = "org.debian.apt";
const string APTD_DBUS_PATH = "/org/debian/apt";

/**
* Expose a subset of org.debian.apt interfaces -- only what's needed by applications lens.
*/
[DBus (name = "org.debian.apt")]
public interface AptdService : GLib.Object {
    public abstract async string install_packages (string[] packages) throws GLib.Error;
    public abstract async string remove_packages (string[] packages) throws GLib.Error;
    public abstract async void quit () throws GLib.Error;
}

[DBus (name = "org.debian.apt.transaction")]
public interface AptdTransactionService : GLib.Object {
    public abstract void run () throws GLib.Error;
    public abstract void simulate () throws GLib.Error;
    public abstract void cancel () throws GLib.Error;
    public signal void finished (string exit_state);
    public signal void property_changed (string property, Variant val);
}

public class AptdProxy : GLib.Object {
    private AptdService _aptd_service;

    public void connect_to_aptd () throws GLib.Error {
        _aptd_service = Bus.get_proxy_sync (BusType.SYSTEM, APTD_DBUS_NAME, APTD_DBUS_PATH);
    }

    public async string install_packages (string[] packages) throws GLib.Error {
        string res = yield _aptd_service.install_packages (packages);
        return res;
    }

    public async string remove_packages (string[] packages) throws GLib.Error {
        string res = yield _aptd_service.remove_packages (packages);
        return res;
    }

    public async void quit () throws GLib.Error {
        yield _aptd_service.quit ();
    }
}

public class AptdTransactionProxy : GLib.Object {
    public signal void finished (string transaction_id);
    public signal void property_changed (string property, Variant variant);

    private AptdTransactionService _aptd_service;

    public void connect_to_aptd (string transaction_id) throws GLib.Error {
        _aptd_service = Bus.get_proxy_sync (BusType.SYSTEM, APTD_DBUS_NAME, transaction_id);
        _aptd_service.finished.connect ((exit_state) => {
            debug ("aptd transaction finished: %s\n", exit_state);
            finished (transaction_id);
        });
        _aptd_service.property_changed.connect ((prop, variant) => {
            property_changed (prop, variant);
        });
    }

    public void simulate () throws GLib.Error {
        _aptd_service.simulate ();
    }

    public void run () throws GLib.Error {
        _aptd_service.run ();
    }

    public void cancel () throws GLib.Error {
        _aptd_service.cancel ();
    }
}

// use like this
    aptd = new AptdProxy ();
    try {
        aptd.connect_to_aptd ();
    } catch (Error e) {
        warning ("Could not connect to APT daemon");
    }

    aptd.install_packages.begin (packages, (obj, res) => {
        try {
            var transaction_id = aptd.install_packages.end (res);

            proxy = new AptdTransactionProxy ();
            proxy.finished.connect (() => {
                // 下载完成
            });
            proxy.property_changed.connect ((prop, val) => {
                if (prop == "Progress") {
                    //下载进度 progress_changed ((int) val.get_int32 ());
                }
                if (prop == "Cancellable") {
                    //取消下载 install_cancellable = val.get_boolean ();
                }
            });

            try {
                proxy.connect_to_aptd (transaction_id);
                proxy.simulate ();
                proxy.run ();
            } catch (Error e) {
                warning ("Could no run transaction: %s", e.message);
            }

        } catch (Error e) {
            warning ("Could not queue downloads: %s", e.message);
        }
    });

```
