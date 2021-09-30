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

# Gtk 
- [语法](#语法)
  - [类型定义](#类型定义)
- [Gtk](#gtk)
  - [一 窗口属性](#一-窗口属性)
    - [CSS相关](#css相关)
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
## 一 窗口属性
### CSS相关
```css
.class_name{ 
    color:red; 
}
button#settings_restore {
   color:red; 
}

```
1. Gtk.StyleContext.add_provider_for_screen:为整个窗口设置CSS,然后使用方法 .add_class("class_name") 使用CSS中的某个类名域下的规则。
2. 方法add_provider:为单独一个widget设置CSS
3. 使用markup

示例：
- way1. label设置支持markup才能使用css标签如：
```vala
var logo_text = new Gtk.Label ("<b>Jide Os</b>") {
    use_markup = true,
    name = "settings_restore"
};
//logo_text.set_name("settings_restore") //无效，对控件id操作命名与name无关
```
- way2. derectly use:｀logo_text.set_markup ("\<span weight='bold' color='white' font_desc='16'>Jide OS\</span>")｀
- way3. use #id: `logo_text.get_style_context ().add_provider(xx,xx)`;
- way4. tyle class：   `logo_text.get_style_context ().add_class ("class_name")`;


4. #id:CSS中id为控件的name，而`set_name`，`get_name`是对控件id的操作命名,与name无关。
```vala
var settings_restore_button = new Gtk.Button.with_label (_("Restore Default Settings")){
    name = "settings_restore"
};
settings_restore_button.set_name("id_name");
critical (settings_restore_button.get_name ()); // printf out  id_name
```
settings_restore_button等同如下效果的按钮
```XML
<interface>
  <requires lib="gtk+" version="3.24"/>
    <object class="GtkButton" id="id_name">
      <property name="label" translatable="yes">Restore Default Settings</property>
      <property name="name">settings_restore</property>
      <property name="visible">True</property>
      <property name="can-focus">True</property>
      <property name="receives-default">True</property>
    </object>
</interface>
```
5. 旋转
```CSS
@keyframes spin {
  to { -gtk-icon-transform: rotate(1turn); }
}

image {
  animation-name: spin;
  animation-duration: 1s;
  animation-timing-function: linear;
  animation-iteration-count: infinite;
}
```

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

3. 控件在布局容器中自适应填充扩大的解决方法，设置下align属性，默认为Align.FILL,所以会填充满
4. 获取焦点：`grab_focus()`
5. 拖动信号: `drag_data_received`

    
    
## 二 对象
1. Gtk.Overlay： 是一个容器，它包含一个主子项，它可以在其上放置“覆盖”小部件。（相当于背景与前景）
2. GLib.DesktopAppInfo,AppInfo:lanch方法启动程序
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
            ``` 
            
            ```
            
            
        
4. GLib.Subprocess执行system命令行
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
5. download_file
```
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

7. async function

- 有返回值的，无的话直接调用begin就行。    
```vala
async typeRet funcA(typeArg args...){
    ...
}   
//call as bellow:
    funcA.begin(typeArg args... , (obj, res) => {
        typeRet ret = funcA.end(res);
        //judge ret 
            ...
        

    })；
 
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
11. Gtk.Revealer:显示容器，控制子部件是否显示

12. 单例模式
```
private static GLib.Once<T> instance;
public static unowned T get_default () {
    return instance.once (() => { return new T (); });
}  
```
13. 默认程序设置 ：GLib.AppInfo  
- set_as_default_for_type(MINE-TYPE); 
- get_default_for_type(mine-type)

14. 获取Application
`unowned SwitchboardApp app = (SwitchboardApp) GLib.Application.get_default ();`

15. 延时处理
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
3. GLib.Filename 获取文件basename, display_name
4. 获取文件类型
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

## 进程相关
1. `Posix.getuid ()`
2. 


## command line args
GLib.OptionContext

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