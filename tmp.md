# tmp

## asdfasd

1. 电源相关的设置：org.gnome.settings-daemon.plugins.power

1. 播放源码

```vala
Canberra.Context context;
Canberra.Context.create (out context);
context.play (0, Canberra.PROP_EVENT_ID, "desktop-login"
                ,Canberra.PROP_CANBERRA_CACHE_CONTROL, "never", null
                );
```

- desktop-login : 登录音
- desktop-logout  : 注销ID
- desktop-screen-lock ：锁屏ID

1. 系统提示音开关

``` xml
  <schema id="io.patapua.settings.sound" path="/io/patapua/settings/sound/">
    <key name="login-play" type="b">
      <default>false</default>
      <summary>play login/logout sound</summary>
      <description>whether to play sound when login or logout</description>
    </key>
  </schema>

```

1. 注销相关的设置（screensaver屏保）

   - setting path:/org/gnome/desktop/screensaver/

      - logout_enable
      - logout_command

1. ` sudo apt install gnome-session-canberra `

1. `paplay /usr/share/sounds/ubuntu/stereo/desktop-login.ogg`

1. sh

    ```sh
    #!/bin/sh
    ret=`gsettings get org.gnome.desktop.sound event-sounds`
    echo $ret
    if [ $ret = "true"  ] ;then
      paplay /usr/share/sounds/freedesktop/stereo/bell.oga 
    fi
    ```

1. systemed

    ```sh
    [Unit]
    Description=Cleanup of Temporary Directories
    Documentation=man:tmpfiles.d(5) man:systemd-tmpfiles(8)
    DefaultDependencies=no
    Conflicts=shutdown.target
    After=local-fs.target time-set.target
    Before=shutdown.target

    [Service]
    Type=oneshot
    ExecStart=systemd-tmpfiles --clean
    SuccessExitStatus=DATAERR
    IOSchedulingClass=idle

    ```

    > conflicts with all system services：指那些定义了Conflicts=shutdown.target 和 Before=shutdown.target 依赖关系（除非明确设置了 DefaultDependencies=no ，否则 service 单元将会自动添加这些依赖）的服务，这些服务在shutdown.target运行之前会停止。
    ————————————————
    原文链接：<https://blog.csdn.net/z1026544682/article/details/104538239>

    usr: gsd-sound.target

## 更改插件名称

1. 改依赖包名

1. 改PLUG文件
   1. 改类型名
   2. 改code_name

1. 改安装路径

1. 搜索修改`grep switchboard . -rn --exclude-dir={build,.git,.github} --exclude=*.{po,pot}`

1. 修改翻译文件

    - 修改主工程的meson.build中的gettext_name

      ```meson
      gettext_name = 'settingboard-plug-' + meson.project_name() 
      ```

    - rename  xml中的translation type

    - 寻找是否有`GLib.Intl.bindtextdomain`函数，找出定义的宏参数修改

## screensaver

`xdg-screensaver`
`mplayer -fs -nosound -noautosub -loop 0 -fixed-vo`

[add xnoise-screensaver-manager.vala](https://github.com/jozadaquebatista/xnoise/commit/84ec07c29d34d10c629acda813202afe7e34c7fe)

Actor设置图形

```vala
  Gdk.Pixbuf pixbuf = Gala.Utils.get_icon_for_window (window, icon_size, scale);
  try {
      var image = new Clutter.Image ();
      Cogl.PixelFormat pixel_format = (pixbuf.get_has_alpha () ? Cogl.PixelFormat.RGBA_8888 : Cogl.PixelFormat.RGB_888);
      image.set_data (pixbuf.get_pixels (), pixel_format, pixbuf.width, pixbuf.height, pixbuf.rowstride);
      Clutter.Actor actor = new Clutter.Actor()
      actor.set_content (image);
      actor.set_size(pixbuf.width, pixbuf.height);
  } catch (Error e) {
```

Actor动画

```vala
    opacity = 0 // 初始化opacity值

    //设置此状态后，后面的动态变化的属性值即使赋值也不会立即更改,会按设置的easing模式更改
    save_easing_state ();
    set_easing_mode (AnimationMode.EASE_OUT_QUAD);
    set_easing_duration (200);

    //设置在duration时间后要达到的动画属性, 只要是animatable的属性都可以设置
    opacity = 255;
    warning("opacity = %d",opacity);// opacity = 0  ,设置后当前值仍为0,在随后的200ms慢慢为255
    ...

    //恢复设置值立即生效模式
    //这句只是恢复easing_state,不是恢复opacity值到0的哦！！！！，因此下次还要初始化它为0
    restore_easing_state ();
    

```

### linux下使用feh一个基于命令行的高速图片查看器的方法

```sh
 feh -x -F -r -Y -Z -z -A slideshow -D 2 "$DIR" &
```

1. [simplescreensaver](https://github.com/jamcnaughton/useful-linux-scripts/tree/master/simplescreensaver) - shell

## 通知

1. 开启消息服务进程 io.elementary.notifications

2. 发送消息

    - 使用命令行：`notify-send messge`
    - 使用测试程序：io.elementary.notifications.demo

## 终端页面无法回滚，不能使用上和pageup键回看,原因是pager默认配置为more,改为less即可

```sh
ann@dell:settingboard-plug-applications-1.0.0$ sudo update-alternatives --config pager 
有 2 个候选项可用于替换 pager (提供 /usr/bin/pager)。

  选择       路径          优先级  状态
------------------------------------------------------------
  0            /usr/bin/less    77        自动模式
* 1            /bin/more        50        手动模式
  2            /usr/bin/less    77        手动模式

要维持当前值[*]请按<回车键>，或者键入选择的编号：0
update-alternatives: 使用 /usr/bin/less 来在自动模式中提供 /usr/bin/pager (pager)

```

## display

- data stream logic
framebuffer-->CRTCs（从framebuffer拿到数据送往encoder，）-->encoder（将数据流编码成对应数据线识别的数据流）-->connector(物理数据线，HDMI，VGA)
  - **serial**:是代表当前状态的唯一标识符 屏幕的。它必须传递回 ApplyConfiguration() 并且每次配置更改都会增加（以便 mutter 可以检测到新的配置是基于旧的 状态）
  - **CRTCs**:a logic monitor,组合成的坐标空间中的一部分, 分屏、整合转换坐标,clone模式一对多个monitors,它的数量决定了最大显示的屏数
  - **output**: a physical screen, connected to the computer
  - **modes**:a set of parameters that are applied to each output
  -

- display dbus

```vala
[DBus (name = "org.gnome.Mutter.DisplayConfig")]
public interface MutterDisplayConfigInterface : Object {
    public abstract void get_resources (out uint serial, out MutterReadDisplayCrtc[] crtcs, out MutterReadDisplayOutput[] outputs, out MutterReadDisplayMode[] modes, out int max_screen_width, out int max_screen_height) throws Error;
    public abstract void apply_configuration (uint serial, bool persistent, MutterWriteDisplayCrtc[] crtcs, MutterWriteDisplayOutput[] outputs) throws Error;
    public abstract int change_backlight (uint serial, uint output, int value) throws Error;
    public abstract void get_crtc_gamma (uint serial, uint crtc, out uint[] red, out uint[] green, out uint[] blue) throws Error;
    public abstract void set_crtc_gamma (uint serial, uint crtc, uint[] red, uint[] green, uint[] blue) throws Error;
    public abstract int power_save_mode { get; set; }
    public signal void monitors_changed ();
    /***
      @serial: configuration serial
      @monitors: available monitors, represent connected physical monitors
      @logical_monitors: current logical monitor configuration,
      @properties: display configuration properties
    ***/
    public abstract void get_current_state (out uint serial, out MutterReadMonitor[] monitors, out MutterReadLogicalMonitor[] logical_monitors, out GLib.HashTable<string, GLib.Variant> properties) throws Error;
    /***
      @serial: configuration serial
      @method: configuration method
      @logical_monitors: monitors configuration, consists of a list of logical monitor configurations
      @properties: properties
    ***/
    public abstract void apply_monitors_config (uint serial, MutterApplyMethod method, MutterWriteLogicalMonitor[] logical_monitors, GLib.HashTable<string, GLib.Variant> properties) throws Error;
}

```

- struct demenstration

```vala
public struct MutterWriteLogicalMonitor {
    public int x;
    public int y;
    public double scale;
    public DisplayTransform transform;
    public bool primary;// true if this is the primary logical monitor
    public struct MutterWriteMonitor {
        public string connector;
        public string monitor_mode;//monitor mode ID
        public GLib.HashTable<string, GLib.Variant> properties;
    } [] monitors;
}

public struct MutterReadMonitor {
    public struct MutterReadMonitorInfo {
        public string connector;
        public string vendor;
        public string product;
        public string serial;
        public uint hash { get { return (connector + vendor + product + serial).hash (); } }
    } monitor;//
    public struct MutterReadMonitorMode {
        public string id;
        public int width;
        public int height;
        public double frequency;
        public double preferred_scale;
        public double[] supported_scales;
        public GLib.HashTable<string, GLib.Variant> properties;//is-current,is-preferred,is-interlaced
    }[] modes;//当下monitor的可用模式
    public GLib.HashTable<string, GLib.Variant> properties;//width-mm,height-mm,is-underscanning,max-screen-size,is-builtin,display-name
}

public struct MutterReadLogicalMonitor {
    public int x;
    public int y;
    public double scale;
    public DisplayTransform transform;
    public bool primary;// true if this is the primary logical monitor
    public struct MutterReadMonitorInfo {
        public string connector;
        public string vendor;
        public string product;
        public string serial;
        public uint hash { get { return (connector + vendor + product + serial).hash (); } }
    }[] monitors;// monitors displaying this logical monitor
    public GLib.HashTable<string, GLib.Variant> properties; // layout-mode(1:logical,2:physical)
}
```

- (MutterWriteLogicalMonitor)logical_monitor <---> (Display.VirtualMonitor)virtual_monitor
- (MutterWriteMonitor)mutter_monitor         <---> (Display.Monitor)virtual_monitor.monitors
- MutterReadMonitor[] mutter_monitors     <---> (Display.Monitor linkedlist)monitors + displayname,is_builtin,modes{is_preferred,is_current}
- MutterReadLogicalMonitor[] mutter_logical_monitors <---> (Display.VirtualMonitor linkedlist)virtual_monitors

```vala
    public int monitor_number { get { return monitors.size; } }
    public int virtual_monitor_number { get { return virtual_monitors.size; } }
    public bool is_mirrored { get { return virtual_monitors.size == 1 && monitors.size > 1; } }
```

- xranr: 调节显示的命令行程序

`xranr --output xxx --brightness 0.8`

```c
dpy = XOpenDisplay(display_name);
screen = DefaultScreen(dpy);
root = RootWindow(dpy, screen);

get_screen(True);
   XRRGetScreenSizeRange(dpy, root, &minWidth, &minHeight, &maxWidth, &maxHeight);
    res = XRRGetScreenResourcesCurrent(dpy, root);
get_crtcs();
  for (c = 0; c < res->ncrtc; c++)
    XRRGetCrtcInfo(dpy, res, res->crtcs[c])
    XRRGetPanning(dpy, res, res->crtcs[c]);
    XRRGetCrtcTransform(dpy, res->crtcs[c], &attr)

get_outputs();
  XRRGetOutputInfo(dpy, res, res->outputs[o]);
  output = find_output(&output_name); 
  output?:output = add_output();


set_positions();
set_screen_size();

pick_crtcs();
set_crtcs();
mark_changing_crtcs();

fb_width_mm = DisplayWidthMM(dpy, screen);
fb_height_mm = DisplayHeightMM(dpy, screen);

set_panning();//skip
set_gamma();
  size = XRRGetCrtcGammaSize(dpy, crtc->crtc.xid);
  crtc_gamma = XRRAllocGamma(size);
  for (i = 0; i < size; i++)
     if (gammaRed == 1.0 && output->brightness == 1.0)//include gammaGreen,gammaBlue
    crtc_gamma->red[i] = (double)i / (double)(size - 1) * 65535.0;
   else
    crtc_gamma->red[i] = dmin(pow((double)i / (double)(size - 1), gammaRed) * output->brightness, 1.0) * 65535.0;
  XRRSetCrtcGamma(dpy, crtc->crtc.xid, crtc_gamma);

apply();
  XGrabServer(dpy);
  screen_apply();
   XRRSetScreenSize(dpy, root, fb_width, fb_height, fb_width_mm, fb_height_mm);
  for (c = 0; c < res->ncrtc; c++)
  s = crtc_apply(crtc);//XRRSetCrtcConfig 亮度设置下去了
  set_primary();
  XUngrabServer(dpy);

```

亮度调节要点：

- 获取output的gamma和亮度值在函数`set_gamma_info()`中

```c
static void
set_gamma_info(output_t *output)
{
 XRRCrtcGamma *crtc_gamma;
 double i1, v1, i2, v2;
 int size, middle, last_best, last_red, last_green, last_blue;
 CARD16 *best_array;

 if (!output->crtc_info)
  return;

 size = XRRGetCrtcGammaSize(dpy, output->crtc_info->crtc.xid);
 if (!size)
 {
  warning("Failed to get size of gamma for output %s\n", output->output.string);
  return;
 }

 crtc_gamma = XRRGetCrtcGamma(dpy, output->crtc_info->crtc.xid);
 if (!crtc_gamma)
 {
  warning("Failed to get gamma for output %s\n", output->output.string);
  return;
 }

 /*
     * Here is a bit tricky because gamma is a whole curve for each
     * color.  So, typically, we need to represent 3 * 256 values as 3 + 1
     * values.  Therefore, we approximate the gamma curve (v) by supposing
     * it always follows the way we set it: a power function (i^g)
     * multiplied by a brightness (b).
     * v = i^g * b
     * so g = (ln(v) - ln(b))/ln(i)
     * and b can be found using two points (v1,i1) and (v2, i2):
     * b = e^((ln(v2)*ln(i1) - ln(v1)*ln(i2))/ln(i1/i2))
     * For the best resolution, we select i2 at the highest place not
     * clamped and i1 at i2/2. Note that if i2 = 1 (as in most normal
     * cases), then b = v2.
     */
 last_red = find_last_non_clamped(crtc_gamma->red, size);
 last_green = find_last_non_clamped(crtc_gamma->green, size);
 last_blue = find_last_non_clamped(crtc_gamma->blue, size);
 best_array = crtc_gamma->red;
 last_best = last_red;
 if (last_green > last_best)
 {
  last_best = last_green;
  best_array = crtc_gamma->green;
 }
 if (last_blue > last_best)
 {
  last_best = last_blue;
  best_array = crtc_gamma->blue;
 }
 if (last_best == 0)
  last_best = 1;

 middle = last_best / 2;
 i1 = (double)(middle + 1) / size;
 v1 = (double)(best_array[middle]) / 65535;
 i2 = (double)(last_best + 1) / size;
 v2 = (double)(best_array[last_best]) / 65535;
 if (v2 < 0.0001)
 { /* The screen is black */
  output->brightness = 0;
  output->gamma.red = 1;
  output->gamma.green = 1;
  output->gamma.blue = 1;
 }
 else
 {
  if ((last_best + 1) == size)
   output->brightness = v2;
  else
   output->brightness = exp((log(v2) * log(i1) - log(v1) * log(i2)) / log(i1 / i2));
  output->gamma.red = log((double)(crtc_gamma->red[last_red / 2]) / output->brightness / 65535) / log((double)((last_red / 2) + 1) / size);
  output->gamma.green = log((double)(crtc_gamma->green[last_green / 2]) / output->brightness / 65535) / log((double)((last_green / 2) + 1) / size);
  output->gamma.blue = log((double)(crtc_gamma->blue[last_blue / 2]) / output->brightness / 65535) / log((double)((last_blue / 2) + 1) / size);
 }

 XRRFreeGamma(crtc_gamma);
}

```

- 设置亮度值在函数`set_gamma`中

  ```c
   if (output->gamma.red == 0.0)
   output->gamma.red = 1.0;
  if (output->gamma.green == 0.0)
   output->gamma.green = 1.0;
  if (output->gamma.blue == 0.0)
   output->gamma.blue = 1.0;

  gammaRed = 1.0 / output->gamma.red;
  gammaGreen = 1.0 / output->gamma.green;
  gammaBlue = 1.0 / output->gamma.blue;

  for (i = 0; i < size; i++)
  {
   if (gammaRed == 1.0 && output->brightness == 1.0)
    crtc_gamma->red[i] = (double)i / (double)(size - 1) * 65535.0;
   else
    crtc_gamma->red[i] = dmin(pow((double)i / (double)(size - 1), gammaRed) * output->brightness, 1.0) * 65535.0;

   if (gammaGreen == 1.0 && output->brightness == 1.0)
    crtc_gamma->green[i] = (double)i / (double)(size - 1) * 65535.0;
   else
    crtc_gamma->green[i] = dmin(pow((double)i / (double)(size - 1), gammaGreen) * output->brightness, 1.0) * 65535.0;

   if (gammaBlue == 1.0 && output->brightness == 1.0)
    crtc_gamma->blue[i] = (double)i / (double)(size - 1) * 65535.0;
   else
    crtc_gamma->blue[i] = dmin(pow((double)i / (double)(size - 1), gammaBlue) * output->brightness, 1.0) * 65535.0;
  }

  ```

## unable to login in

```sh
apt install gnome-session
apt remove gnome-session
```

--------------------

## settingboard -- network

### 设备状态变更信号传导图

```mermaid
graph LR
A(device.state_changed)-->B(infoBox.info_changed)-->C{page.update} -.override.->D & E & F & H & I -.-> J[update_switch:更改status_switch.active]
E[HotspotPage.update]
D[EthernetPage.update]
F[ModemPage.update]
H[VPNPage.update]
I[WifiPage.update]
```

### **status_switch**开关设置的信号传导图

```mermaid
graph LR
A(status_swtich.active)--notify-->B[control_switch_activated]
B--false-->C[device:disconnect]--failed-->Ab[active=true]-->A
B--yes-->D[device:connect]
D-->E(device.state_changed)
```

## find git repository

```sh
for file in `ls` ; do cd $file; if [ -d .git ] ;then  git remote -v | grep fetch | grep leixa | awk {'print $2'}; fi; cd ->/dev/null ;done

#install build dependency
for i in `ls ./settingboard* --directory`;do  cd $i; sudo apt build-dep .;cd ->/dev/null;done

```

## gtk theme

### flatpak app无法使用系统的theme

1. [Applying GTK themes to Flatpak applications](https://itsfoss.com/flatpak-app-apply-theme/)
   - Step 1: Give Flatpak apps access to GTK themes location
     - all app:`sudo flatpak override --filesystem=$HOME/.themes`
     - single app:`sudo flatpak override org.gnome.Calculator --filesystem=$HOME/.themes`
   - Step 2:Tell Flatpak apps which theme to use
     - all app:`sudo flatpak override --env=GTK_THEME=my-theme`
     - single app:`sudo flatpak override org.gnome.Calculator --env=GTK_THEME=my-theme`
   - Step 3: Test the theme change by running a Flatpak app
   - Revert the changes
     - all :`sudo flatpak override --reset`
     - single :`sudo flatpak override --reset org.example.app`
2. [How To Get Flatpak Apps To Use The Correct Gtk Theme](https://www.linuxuprising.com/2018/05/how-to-get-flatpak-apps-to-use-correct.html)
   下载对就的flatpak版本的主题
3. [How do I disable client side decoration globally in GNOME?](https://askubuntu.com/questions/961161/how-do-i-disable-client-side-decoration-globally-in-gnome)

  > I was able to run the evince PDF viewer with decorations provided by my window manager (Openbox) on Ubuntu 18.04 as follows:

  > `$ sudo apt-get install gtk3-nocsd`

  > `$ gtk3-nocsd evince`

4. GTK_CSD=1, 在启动程序将禁止窗口管理器自动加标题栏

5. chrome设置默认的系统标题栏
    路径：

    ```mermaid
    graph LR
    A("～/.config/goolge-chrome/Default/Preferences")
    B(custom_chrome_frame)
    A-->B
    ```

6. 针对默认加载的的标题栏的css的样式，试验发现只在登录环境时加载一次（可能libgtk3库首次加载时会读取记录一次默认的样式到自己的上下文中），如果改变了样式文件需要测试默认标题栏是否更改，则必须注销重新登录才能测试出来，否则更改无效。
标题栏的button在目前的gtk.css中如下（大致在1306行）：

    ```css
    window.ssd .titlebar button {
      border-radius: 3px;
    }
    ```

### gtk default title bar source

1. file: gtk/gtkwindow.c

```c
static void
gtk_window_realize (GtkWidget *widget)
{
  GtkWindow *window = GTK_WINDOW (widget);
  GtkWindowPrivate *priv = gtk_window_get_instance_private (window);
  GdkSurface *surface;
  GdkFrameClock *frame_clock;

  /* Create default title bar */
  if (!priv->client_decorated && gtk_window_should_use_csd (window))
    {
      priv->use_client_shadow = gtk_window_supports_client_shadow (window);
      if (priv->use_client_shadow)
        {
          gtk_window_enable_csd (window);

            if (priv->title_box == NULL)
              {
                priv->title_box = gtk_header_bar_new ();
                gtk_widget_add_css_class (priv->title_box, "titlebar");
                gtk_widget_add_css_class (priv->title_box, "default-decoration");

                gtk_widget_insert_before (priv->title_box, widget, NULL);
              }

            update_window_actions (window);
        }
    }

  ...

  GTK_WIDGET_CLASS (gtk_window_parent_class)->realize (widget);

  ...

  if (priv->title)
    gdk_toplevel_set_title (GDK_TOPLEVEL (surface), priv->title);

  gdk_toplevel_set_decorated (GDK_TOPLEVEL (surface), priv->decorated && !priv->client_decorated);

  ...
}

void
gtk_window_set_titlebar (GtkWindow *window,
                         GtkWidget *titlebar)
{
  ...

  unset_titlebar (window);

  if (titlebar == NULL)
    {
      /* these are updated in realize() */
      priv->client_decorated = FALSE;
      gtk_widget_remove_css_class (widget, "csd");
      gtk_widget_remove_css_class (widget, "solid-csd");
    }
  else
    {
      priv->use_client_shadow = gtk_window_supports_client_shadow (window);

      gtk_window_enable_csd (window);
      priv->titlebar = titlebar;
      priv->title_box = titlebar;
      gtk_widget_insert_before (priv->title_box, widget, NULL);

      gtk_widget_add_css_class (titlebar, "titlebar");
    }
  ...
}

static void
gtk_window_enable_csd (GtkWindow *window)
{
  GtkWindowPrivate *priv = gtk_window_get_instance_private (window);
  GtkWidget *widget = GTK_WIDGET (window);

  /* We need a visual with alpha for client shadows */
  if (priv->use_client_shadow)
    gtk_widget_add_css_class (widget, "csd");
  else
    gtk_widget_add_css_class (widget, "solid-csd");

  priv->client_decorated = TRUE;
}

static gboolean
gtk_window_should_use_csd (GtkWindow *window)
{
  GtkWindowPrivate *priv = gtk_window_get_instance_private (window);
  const char *csd_env;

  if (priv->csd_requested)
    return TRUE;

  if (!priv->decorated)
    return FALSE;

  csd_env = g_getenv ("GTK_CSD");

  ...

  return (g_strcmp0 (csd_env, "1") == 0);
}
```

- 代码中的gdk_toplevel_set_decorated (GDK_TOPLEVEL (surface), priv->decorated && !priv->client_decorated);

> /*
>
> - Setting @decorated to %FALSE hints the desktop environment
> - that the surface has its own, client-side decorations and
> - does not need to have window decorations added.
> */
在自己代码中可以直接调用`gtk_window_set_decorated`设置

```Mermaid
flowchart LR
F0[gtk_window_init]
F1(gtk_window_set_titlebar)
F2(gtk_window_realize)
F3(gtk_widget_show)

C1{tiltebar!=null ?}
C2{"client_decorated==false<br> && decorated==true<br> && (GTK_CSD=1 or csd_requested=true)"}

AACSD((add csd/solid-csd,titlebar<br>client_decorated=true))
ARCSD((remove csd/solid-csd<br>client_decorated=false))
AADEF((add default csd))

SINIT[[decorated:true<br>client_decorated:false<br>GTK_CSD=0]]
SCSD1[[csd/solid-csd<br>titlebar]]
SNCSD[[default: ssd]]
SCSD2[[csd/solid-csd<br>titlebar<br>default-decoration]]


F0---SINIT--> F1
F0---SINIT--> |1|F3
F1-.-C1-.->|yes| AACSD --> |2| F3
C1-.->|no| ARCSD --> |3| F3


F3-->F2-.-C2-.no 2.-> SCSD1 
C2-.no 1,3-.-> SNCSD
C2-.yes 1,3-.-> AADEF--> SCSD2

```

## gnome-panel

### source code review

```mermaid
flowchart TB
panel_object_loader_idle_handler 
--> panel_applet_frame_load
--> panel_applets_manager_get_applet_info
--> gp_module_manager_init??
---> load_modules
--> A{find so file}
--yes-->gp_module_new_from_path
--> newojbect["g_object_new (GP_TYPE_MODULE, NULL)"]
--> xdasdas["gp_module[_class]_init"]
--> PRI0["module-->gp_module_load"]
--> get_applet_info
--> PRI1[module->get_applet_info_func<br>.func=sn_applet_get_type]
--> A

A--no--> gp_applet_manager_get_applet_info
--> panel_applet_frame_load_helper
--> panel_applets_manager_load_applet
--+1--> gp_applet_manager_get_applet_info
--2----> gp_applet_manager_load_applet
--> gp_module_applet_new
--> PRI2["call get_applet_info"]
--> PRI3["info->get_applet_type_func"]

```

### gnome-panel notification

必须项:

- indicator-
/usr/lib/x86_64-linux-gnu/indicator-application/indicator-application-service--->creat watcher
- application---> StatusNotifierItem
- gnome-panel---> display notification area

## AWS  cml

### install

```sh
curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip awscliv2.zip
sudo ./aws/install
aws configure //
```

### configure快速配置

`aws configure`

### s3 cli 文档

- <https://docs.aws.amazon.com/cli/latest/reference/s3/>

## http

### question

1. 怎么获取后台仓库的所有可用包信息？
2. 获取的包库的存储格式？

● ID： 必需
● Type：必需，支持,
  ○ generic
  ○ desktop-application
  ○ console-application
  ○ addon
  ○ codec
  ○ inputmethod
  ○ firmware
● Name：必需
● Package：必需， 类似pkgname,安装的包名
● Summary：必需

应用详情页-tag | xml tag | 类型
-|-|-
应用名称 | name
slogan | summary
分类标签,类别 | categories
应用简介 | description
新功能 |  release ->description
大小 | release --> artifact
版本号 |  release
更新时间 | release
开发者 | developer_name 或 project_group
应用类型 |  | android, linux
图标 | icon
截图 | screenshots

## 招人评分参考

|评价内容 | 评分 |
|-|-|
|linux系统熟悉程度 | 一般
|（面向对象）编程经验 | 欠缺
|界面开发经验 |  欠缺
|沟通表达能力 |  一般, 未正面回应也未能自圆其说
|解决问题能力 |  未展现
| 用人建议: 不建议

## seeds --> metapackages

## What is [Germinate](https://wiki.debian.org/Germinate#:~:text=What%20is%20Germinate%3F%20Germinate%20is%20a%20package%20available,for%20each%20of%20these%20lists.%20Files%20You%20Need)?  

Germinate is a package available in Debian and Ubuntu which starts with lists of packages (called seeds) and grows them into a full list of packages including dependencies and (in additional lists) suggests, recommends, and sources for each of these lists.
可以结合elementary seeds工程看

## 3588

Rockusb>pl
Partition Info(gpt):
NO  LBA        Size       Name
01  0x00004000 0x00002000 uboot
02  0x00006000 0x00002000 misc
03  0x00008000 0x00100000 boot
04  0x00108000 0x00100000 recovery
05  0x00208000 0x00010000 backup
06  0x00218000 0x00c00000 rootfs
07  0x00e18000 0x06663fc0 userdata
Rockusb>

Rockusb>pl
Partition Info(gpt):
NO  LBA        Size       Name
01  0x00004000 0x00002000 uboot
02  0x00006000 0x00002000 misc
03  0x00008000 0x00040000 boot
04  0x00048000 0x00040000 recovery
05  0x00088000 0x00010000 backup
06  0x00098000 0x00c00000 rootfs
07  0x00c98000 0x067e3fc0 userdata

seeds to removed:
io.elementary.screenshot*io.elementary.videos* org.gnome.fileroller*

org.gnome.desktop.background picture-uri

## 7.25

wallpapers
patapua-default-settings
metapackages

## 7.26 显示已安装软件及版本

dpkg -l | awk '{ if(NR%5==4){printf "%s (<= %s)\n", $2, $3} else { printf "%s (<= %s)," , $2, $3 } }'

## GPU

1. 测试性能工具：glmark2, glmark2-es2
2. 设置环境变量输出调试信息：LIBGL_DEBUG=verbose glmark2
3. 设置运行环境： COGL_DRIVER=gles2, (gstream:GST_GL_API=gles2)

## shell异常退出处理

```sh
finish() {
  echo "here do something before exit"
  exit -1
}
set -e # If not interactive, exit immediately if any untested command fails.
trap  finish ERR
```

## 如何在x86机器上跑arm应用

1. 安装QEMU emulator ：``
2.

## 3588 linux sdk

1. ITX-3588J android12.0 SDK 源码

1、百度网盘链接
链接：<https://pan.baidu.com/s/15hXLJ5Wbi-5sJipF_k1Ihw?pwd=1234> 提取码：1234
2、Google driver
<https://drive.google.com/drive/folders/1GWlPj3D8SrceV_nRhTBNdxULQ8FtAtRK?usp=sharing>

ITX-3588J，Linux sdk：
1、百度网盘：
链接：<https://pan.baidu.com/s/1w1fd31vgxkyFmRBOs_Wu_g?pwd=1234>
提取码：1234
2、Google driver
<https://drive.google.com/drive/folders/1LovVy481bNUfbZpoCMSAeDEzIZmvZNKH?usp=sharing>

## grub

### 启动步骤

1. 加载内核：

   ```sh
   grub> cat (lvm/data-root)/etc/fstab
   PARTUUID=238232-234.....         /boot/efi vfat umask=0077 0 0
   UUID=33b3063f-5dfa-4cec-bb4c-b0ae86b47d86 / ext4 noatime,errors=remount-ro 0 0
   
   grub> linux (lvm/data-root)/boot/vmlinuz ro root=UUID=33b3063f-5dfa-4cec-bb4c-b0ae86b47d86 quiet
   #其它版本linux命令改为kernel
   grub> kernel 
   ```

   - (lvm/data-root)/boot/vmlinuz：指定了内核文件的位置，这里的 (lvm/data-root)/boot/ 是指 boot 分区。
   - ro：启动时以只读方式挂载根文件系统，这是为了不让启动过程影响磁盘内的文件系统。
   - root=UUID=33b3063f-5dfa-4cec-bb4c-b0ae86b47d86：指定根文件系统的所在位置。这里和以前的Linux版本不太一样了，不再通过分区的设备文件名或卷标号来指定，而是通过分区的 UUID 来指定的。

2. 加载initramfs虚拟文件系统镜像

   ```sh
   grub> initrd (lvm/data-root)/boot/initrd.img
   ```

3. 启动

   ```sh
   grub> boot
   ```

## proxy

### linux

1. 打开网络设置代理设置，勾选手动模式，在http proxy, https proxy和ftp proxy上都填写上代理如 : "username:passwd@proxy_ip", 在端口号上填写对应端口如3128
    设置好后，打开终端，查看环境变量就会看到 `env | grep -i proxy`， 成功后即可访问网页，wget也可以正常使用

2. 设置apt代理，touch /etc/apt/apt.conf, 内容如下：

    ``` txt  
    Acquire::http::Proxy "http://username:passwd@proxy_ip:port";
    ```

3. docker 代理设置

   [参考链接](https://docs.docker.com/config/daemon/systemd/)  

    ```sh
    sudo mkdir -p /etc/systemd/system/docker.service.d
    sudo touch /etc/systemd/system/docker.service.d/http-proxy.conf
    cat << eof >> /etc/systemd/system/docker.service.d/http-proxy.conf
    > [Service]
    > Environment="HTTP_PROXY=http://proxy.example.com:80"
    > Environment="HTTPS_PROXY=https://proxy.example.com:443"
    > Environment="NO_PROXY=localhost,127.0.0.1,docker-registry.example.com,.corp"
    > eof
    sudo systemctl daemon-reload
    sudo systemctl restart docker
    sudo systemctl show --property=Environment docker
    ```

4. snap proxy

  ```sh
  sudo snap set system proxy.http="http://username:passwd@ip:port"
  sudo snap set system proxy.https="http://username:passwd@ip:port"
  ```

### window

1. powershell

   ```powershell
   netsh winhttp set proxy http://proxy.example.com:port
   ```

## open remote github repository

1. install extension: remote repositories
2. `ctrl+shitf+p` to choose "Remote Repositories: Open ..." or click left corner >< icon
3. open github repository

## 检测是否在docker内

```sh
ps --no-headers --pid 1 | grep --silent docker-init && in_docker=1 || in_docker=0
[ $in_docker = 1 ] && {
        PS1='\[\033[01;36m\][docker] '$PS1
}
```

## curl 7.58 under proxy issue ssl wrong version

https_proxy代理设置错了，设置成与http_proxy一样即可

## rmnet_data0及桥接上网

1. "error: +brctl: bridge bridge0: Operation not supported"

必须先关网桥才能设置

```sh
ifconfig bridge0 down
```

## CSI

![Alt text](picture/blog/MIPI-multimedia-specifications.png "MIPI Multimedia Spec")

- MIPI(Mobile Industry Processor Interface)
  - protocol layers:
    - CSI: camera serial interface
    - DSI: display serial interface
    - UFS: universal flash storage
  - physical layers:
    - C-PHY: C指的是Channel-limited，而不是Camera，通道被限制到3，C-PHY总共有**3条lanes**，**每条lane使用3线差分**的传输模式，无时钟源，嵌入式时钟（时钟隐藏在通信的时序之中）
    - D-PHY: D-PHY是**每条lane是一对差分线**, 有同步时钟源
    - M-PHY: UFS专用

![Alt text](picture/blog/MIPI-CPHY-VS-DPHY.png "CPHY VS DPHY")

### SUMMARY

C-PHY、D-PHY、M-PHY属于物理接口层，同一协议可以使用不同的物理接口，比如CSI协议可以使用C-PHY、D-PHY、M-PHY输出，DSI协议可以使用C-PHY、D-PHY、M-PHY输出，UFS协议只能使用M-PHY输出。

## linux kernel media

### 说明文档

> kernel/Documentation/devicetree/bindings/media
> kernel/Documentation/userspace-api/media
> kernel/Documentation/admin-guide/media
> kernel/Documentation/driver-api/media

## AG35-emmc自动挂载

- 使用的是mdev

- 在/etc/mdev.conf中匹配

```sh

mmcblk[0-9]*     0:6     660
mmcblk[0-9]*p[0-9]* 0:6     660 */etc/mdev/automountsdcard.sh ${MDEV}
[hs]d[a-z][0-9]* 0:0 660 */etc/mdev/automountsdcard.sh ${MDEV}

```

- automountsdcard.sh

```sh
#! /bin/sh

destdir=/mnt/sdcard/

umount_partition()
{
        if grep -qs "^/dev/$1 " /proc/mounts ; then
                umount -lf "${destdir}";
        fi
}

mount_partition()
{
        fsck.ext4 -y /dev/$1
        if [ ! -d "${destdir}" ]; then
            mkdir "${destdir}"
        fi
        if ! mount -t ext4 "/dev/$1" "${destdir}" -o nodev,noexec,nosuid; then
                # failed to mount
                exit 1
        fi
}

check_if_boot_dev()
{
                ret_val=`cat /proc/cmdline | grep "androidboot.bootdevice" |wc -l`
                if  [ $ret_val -ne 0 ]; then
                    boot_dev_list=`cat /proc/cmdline | awk '{ for ( n=1; n<=NF; n++ ) if($n ~ "androidboot.bootdevice") print $n }' | awk '{split($0,a, "=");print a[2]}'`
                    boot_dev=`echo $boot_dev_list | awk '{print $NF}'`
                    real_sysfs_path=`realpath /sys/class/block/$1`
                    ret_val=`echo $real_sysfs_path | grep /sys/devices/ | grep $boot_dev | wc -l`
                fi
                echo $ret_val
}


create_symlink()
{
                real_sysfs_path=`realpath /sys/class/block/$1`
                partition_name=`cat $real_sysfs_path/uevent | awk '{ for ( n=1; n<=NF; n++ ) if($n ~ "PARTNAME") print $n }' | awk '{split($0,a, "=");print a[2]}'`
                mkdir -p /dev/block/bootdevice/by-name/
                partition_name=/dev/block/bootdevice/by-name/$partition_name
                target_dev=/dev/$1
                ln -s $target_dev $partition_name
}

case "${ACTION}" in
add|"")
        result=`check_if_boot_dev $1`
        if [ $result -eq 1 ]; then
                 create_symlink $1 &
        else
                 umount_partition ${1}
                 mount_partition ${1}
        fi
        ;;
remove)
        umount_partition ${1}
        ;;
esac

```

## TI

### [SDK origin 资料](https://www.ti.com.cn/tool/cn/PROCESSOR-SDK-AM62X?keyMatch=am62x&tisearch=universal_search#related-design-resources)

dir|where|url
-|-|-
AM62xAM62xSIP SK Quick Start Guide | AM62xAM62xSIP SK Quick Start Guide | 开发板Guide
AM62 Academy | online |academy^[https://dev.ti.com/tirex/explore/node?node=A__AEIJm0rwIeU.2P1OBWwlaA__AM62-ACADEMY__uiYMDcq__LATEST], Learn how to build embedded systems using AM62 processors
Yocto SDK user manual | Yocto-SDK-user-manual--processor-sdk-linux-docs | sdk手册, 可以下离线版
Yocto Build Instructions | Yocto-SDK-user-manual--processor-sdk-linux-docs |与上一个是同一份指导
Debian SDK user manual | online | Debian Developer’s Guide^[https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/10_00_07_Debian/exports/docs/devices/AM62X/debian/Getting_Started_Guide.html]
Buildroot SDK user manual | online | Buildroot Developer’s Guide^[https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/10_00_07_Buildroot/exports/docs/devices/AM62X/buildroot/index.html]

1. 如果在本机开发，则linux Host 需要装的包

    ```sh
    host# sudo apt-get install build-essential autoconf automake bison flex libssl-dev bc u-boot-tools swig python3 python3-pip
    host# pip3 install jsonschema pyelftools PyYAML Mako
    ```

1. 解决docker中的administrator权限问题

    ```sh
    sudo adduser f1339899 sudo && usermod -aG sudo f1339899 && echo "f1339899 ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
    ```

1. make install时要指定位置才可安装, 且安装目录必须存在，不会自动创建

    ```sh
    DESTDIR=`pwd`/output make install
    ```

1. piglit的lib占用2G ，主要是`targetNFS/usr/lib/piglit/generated_tests`
1. MLO: 保存在emmc里，指示first boot up选项为emmc，清除这部分才能改boot up 行为

### 文件目录说明

**PSDK_PATH**假设此目录为sdk安装目录

dir | description
-|-
<PSDK_PATH>/filesystem/ | 镜像文件存放位置，<li>*.wic.xz: SD卡镜像烧录用的文件（烧录工具：**balenaEtcher**,将其解压后用**bmap-tools**烧录）</li><li></li>
<PSDK_PATH>/bin  | 工具目录，如`create-sdcard.sh`为烧写脚本
<PSDK_PATH>/setup.sh | 在环境中安装配置一些开发工具：tftp, nfs, u-boot, telnet等
<PSDK_PATH>/Makefile | 可用于构建**SDK**中的某些子组件。它使用`Rules.make`文件，并给出了如何构建各种组件以及要使用的参数的示例. 使用它时不能source过linux-devkit下的`environment-setup`脚本, 因为在source后有些make目标会不能被make
<PSDK_PATH>/Rules.make | 共享变量和设置配置文件，它不仅被顶层Makefile使用，还被许多子组件 Makefile 使用，以访问常见的共享变量和设置
<PSDK_PATH>/linux-devkit |  toolchain的安装目录,  针对ARMv8 -- Cortex-A53 SOC
<PSDK_PATH>/k3r5-devkit |  toolchain的安装目录，针对ARMv7 -- Cortex-M4F MCU

来源：[GP to HS-FS Migration Guide](https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/08_06_00_42/exports/docs/linux/Foundational_Components_Migration_Guide.html#k3-image-gen)
Device types | 释义 | desc
-|-|-
GP|General Purpose|这是 SoC/主板状态，其中没有设备保护并且未启用用于启动设备的身份验证。
HS-FS|High Security - Field Securable|这是客户在设备中烧毁密钥之前的 SoC/电路板状态。即 HS 设备离开 TI 工厂时的状态。在此状态下，设备保护 ROM 代码、TI 密钥和某些安全外围设备。在此状态下，设备不会强制进行启动身份验证，但 DMSC 已锁定。
HS-SE|High Security - Security Enforced|这是客户成功烧毁密钥并设置“客户密钥启用”后的SoC/电路板状态。在 HS-SE 设备中，所有安全功能均已启用。设备内的所有机密均受到全面保护，所有安全目标均已全面实施。该设备还强制执行安全启动。

  注：更改设备类型只需要修改分区目录下的tiboot3.bin为对应设备类型的image即可

image | description
-|-
tiboot3-am62x-xxx-evm.bin | xxx可为：**gp,hs,hs-fs**, 其中hs:HS-SE, hs-fs: HS-FS, Initial boot image
tispl.bin | **HS-FS**类型需要，其它两种不需要
u-boot.img | **GP,HS-FS,HS-SE**三者比需要
tiboot3.bin | 依据**GP,HS-FS,HS-SE**设备类型，由上方的tiboot3-am62x-xxx-evm.bin重命名而来

Target Types | description
-|-
target | 编译目标的release版本， target有哪些参见下一个表格，用法`make <target>`
target_stage | 拷贝目标文件到`<TISDK_path>/board-support/built-images`目录
target_install | 安装目标文件到**DESTDIR**指定目录, `sudo DESTDIR=/media/$USER/rootfs make linux_install`
target_clean | 清除目标编译文件

make target | description
-|-
linux | 使用默认配置`tisdk_<PLATFORM>_defconfig`编译内核
linux-dtbs | 编译设备树，生成blob文件
u-boot | 编译u-boot和u-boot SPL(MLO)
arm-benchmarks |构建 ARM 基准
cryptodev | 构建加密硬件加速器模块
ti-img-rogue-driver | 构建GPU内核模块
jailhouse | 构建支持hypervisor平台所需模块

以下这些环境变量都在**Rules.make**文件中有指定
env variants | desc
-|-
PLATFORM |AM62x installer支持`am62xx-evm`和`am62xx-lp-evm`, 前者是默认平台
ARCH | 
UBOOT_MACHINE |
TI_SDK_PATH |
DESTDIR |
LINUX_DEVKIT_PATH |
CROSS_COMPILE |
ENV_SETUP |
LINUXKERNEL_INSTALL_DIR | 

1. 用`bmap-tools`烧录SD卡

```sh
# install bmap-tools
sudo apt-get install bmap-tools

cd <PSDK_PATH>/filesystem
# decompress *.wic.xz to *.wic
unxz tisdk-default-image-<machine>.wic.xz

# generate a bmap file from the decompressed WIC image
bmaptool create -o tisdk-default-image.bmap tisdk-default-image-<machine>.wic

# umount sd card
$ lsblk
NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT
:
:
sdc      8:32   1    15G  0 disk
├─sdc1   8:33   1 131.8M  0 part /media/localUser/boot
└─sdc2   8:34   1 765.9M  0 part /media/localUser/root

$ sudo umount /media/localUser/boot
$ sudo umount /media/localUser/root
$ lsblk
NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT
:
:
sdc      8:32   1    15G  0 disk
├─sdc1   8:33   1 131.8M  0 part
└─sdc2   8:34   1 765.9M  0 part

# write the WIC image to the SD card
sudo bmaptool copy --bmap tisdk-default-image.bmap tisdk-default-image-<machine>.wic /dev/sdc

```

1. GCC ToolChain setup

[参考](https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/09_02_01_10/exports/docs/linux/Overview/GCC_ToolChain.html#yocto-built-sdk-toolchains)

```sh
host# CROSS_COMPILE_64="${SDK_INSTALL_DIR}/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-"
host# SYSROOT_64="${SDK_INSTALL_DIR}/linux-devkit/sysroots/aarch64-oe-linux"
host# CC_64="${CROSS_COMPILE_64}gcc --sysroot=${SYSROOT_64}"
host# CROSS_COMPILE_32="${SDK_INSTALL_DIR}/k3r5-devkit/sysroots/x86_64-arago-linux/usr/bin/arm-oe-eabi/arm-oe-eabi-"
```

Variable| Location| Description
-|-|-
CROSS_COMPILE_64| linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-| Cross compiler toolchain for the ARMv8 architecture
SYSROOT_64| linux-devkit/sysroots/aarch64-oe-linux/ | Sysroot with the cross compiled libraries and headers for the ARMv8 architecture with Linux OS
ENV_SETUP_64 | linux-devkit/environment-setup-aarch64-oe-linux | Shell script that sets environment variables to compile binaries for the ARMv8 linux target

### 编译第三方库

#### dconf

1. 使用meson + ninja编译方式

TI的meson有问题，其并非真实meson,而是封装了一层，设置的meson.cross.template文件有问题。即使cross file设置正确也会报**meson ERROR: Malformed value in machine file variable \'c\'**,在variable前后要加单引号 **''**。

自己设置交叉编译配置文件`aarch64-oe-linux.txt`,内容如下

```txt
[binaries]
c = '/home/anzye/ti/ti-sdk/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-gcc'
cpp = '/home/anzye/ti/ti-sdk/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-g++'
ar = '/home/anzye/ti/ti-sdk/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-ar'
nm = '/home/anzye/ti/ti-sdk/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-nm'
strip = '/home/anzye/ti/ti-sdk/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-strip'
pkg-config = 'pkg-config'

[built-in options]
c_args = '-mbranch-protection=standard --sysroot=/home/anzye/ti/ti-sdk/linux-devkit/sysroots/aarch64-oe-linux'
c_link_args = '--sysroot=/home/anzye/ti/ti-sdk/linux-devkit/sysroots/aarch64-oe-linux'
cpp_args = '-mbranch-protection=standard --sysroot=/home/anzye/ti/ti-sdk/linux-devkit/sysroots/aarch64-oe-linux'
cpp_link_args = '--sysroot=/home/anzye/ti/ti-sdk/linux-devkit/sysroots/aarch64-oe-linux'

[properties]
needs_exe_wrapper = true
sys_root = '/home/anzye/ti/ti-sdk/linux-devkit/sysroots/aarch64-oe-linux'

[host_machine]
system = 'linux'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
```

1. 配置编译, man编译需要其它库，禁用

```sh
meson.real setup --cross-file aarch64-oe-linux.txt builddir -D man=false
ninja -C builddir
```

1. 安装

```sh
DESTDIR=`pwd`/installed ninja install
```

### debian

参考[debian build](https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/10_00_07_Debian/exports/docs/debian/Building_Debian_Image.html)

1. 安装环境中的两个pip3命令要用root用户执行, 不然报toml命令找不到的错

   ```sh
   pip3 install yamllint
   pip3 install toml-cli
   ```

   > Since the build script is run as root user, toml-cli and yamllint should also be installed with sudo for root user to be able to access it.

1. 执行build时会先安装arm和aarch64 toolchain, 网络不行，建议自行安装解压到tools目录下。

   ```sh
   # download 
   host_arch=x86_64 && wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-${host_arch}-aarch64-none-linux-gnu.tar.xz
   host_arch=x86_64 && wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-${host_arch}-aarch64-none-linux-gnu.tar.xz 
   # decompress
   cd tools && tar -Jxf toolchain
   # rm
   rm *.tar.xz
   ```

1. build.sh里的构建rootfs时，apt无法下载，需要修改设置proxy才行

  ```patch
  @@ -81,6 +80,8 @@ mmdebstrap:
        # Setup Apt repository preferences
      - 'mkdir -p $1/etc/apt/preferences.d/'
      - 'printf "Package: *\nPin: origin TexasInstruments.github.io\nPin-Priority: 1001" >> $1/etc/apt/preferences.d/ti-debpkgs'
  +      # Setup proxy
  +    - 'printf "Acquire::http::proxy "http://804537:foxconn168!!@10.191.131.105:3128";" >> $1/etc/apt/apt.conf.d/80proxy'
        # Setup Kernel post-install scripts
      - 'mkdir -p $1/etc/kernel/postinst.d/'
      - 'echo "PWD = $PWD"'

  ```

### yocto

|file | description|
|-|-|
*.bbclass | classes目录中存放的bblcass文件，append to BBPATH
layer.conf | We have a conf and classes directory, append to BBPATH, 配置了BBFILES的属性，包涵的文件， 依赖关系等
recipes-* | 目录中存放*.bb或bbappend文件
*.bb | recipes files
*.bbappend | recipes files

- 编译出的image在目录**deploy-ti**下

- bb中写注释一定要顶行的**#**字符才起作用

yocto 

bbclass类型文件 c++类型，使用`inherit bbclassname`引入，bbclassname即为bbclass文件不带后缀的名字
bb文件类似一个bbclass实例，里面可以继承重载bbclass方法`do_configure, do_compile, do_install`等，可以用`xxx.inc`引用xxx文件变量，
每一个方法都可以在其后加 **":suffix"** 后缀修饰动作如 `do_confiure:append ()`, 这些后缀有如下动作

- prepend: 在当前任务中前插动作
- append: 在当前任务中后缀动作

插入任务`do_run_xxx`

```bb
addtask do_run_xxx before do_run_test after do_compile
```


##### error

1. 报“Files/directories were installed but not shipped in any package”

```sh
ERROR: grpc-1.27.0-1.0-r0 do_package: QA Issue: grpc-1.27.0: Files/directories were installed but not shipped in any package:
  /usr/share
  /usr/share/grpc
  /usr/share/grpc/roots.pem
Please set FILES such that these items are packaged. Alternatively if they are unneeded, avoid installing them or delete them within do_install.
grpc-1.27.0: 3 installed and not shipped files. [installed-vs-shipped]
ERROR: grpc-1.27.0-1.0-r0 do_package: Fatal QA errors were found, failing task.
ERROR: Logfile of failure stored in: /working_dir/build/arago-tmp-default-glibc/work/aarch64-oe-linux/grpc-1.27.0/1.0/temp/log.do_package.39231
ERROR: Task (/working_dir/sources/meta-foxconn/recipes-grpc/grpc/grpc-1.27.0.bb:do_package) failed with exit code '1'
```

  错误说明下方的文件没有被打包，可以在`FILES`变量中加入这些文件，比如 

  `FILES:{PN} += "${datadir}"`

  为什么share目录是datadir?可以参考“sources/oe-core/meta/conf/bitbake.conf”中的变量查找

## minicom 无法输入

> 发现无法回车进入到命令行模式输入命令了，通过查找资料发现关键点就在串口的配置中有个Serial port setup-->Hardware Flow Contorl选项被改成了Yes，这样就造成了键盘没有用了，接受不了任何输入。
>
> (1)sudo minicom -s
(2)进入Serial port setup界面
(3)时F - Hardware Flow Contorl项为Yes，按下F键就修改为No了，即关闭硬件流控，回车回到上级菜单
(4)选择| Save setup as dfl |，然后选择 Exit from Minicom  重启minicom,这样就可以输入命令了。

-------

# kernel debug

1. 反向编译设备树文件： `dtc -I dtb -O dts k3-am625-<boardname>.dtb -o <boardname>-reversed.dts`
   
2. dmesg中内核打印信息

  ```sh
  [time] am65-cpsw-nuss 8000000.ethernet: Use random MAC address
  ```

  am65-cpsw-nuss说的是哪个模块，8000000.ethernet是哪个设备，模块在文件夹`/drivers/net/ethernet/ti/`下，设备在`/arch/arm64/boot/dts/`下
  模块对应的文件是`am65-cpsw-nuss.c`

1. TI MAC TO MAC [参考文档](https://www.ticom/cn/lit/an/zhcaae4/zhcaae4.pdf?ts=1623414228626)

1. 在TI AM62X中实现mac to mac的转发，需要配置`am65-cpsw-nuss`模块的`mac_to_mac`属性为`1`，在`am65-cpsw-nuss.dtsi`文件中添加如下配置：

  将mdio接口导出到sysfs中，这样就可以通过sysfs来配置mdio接口的寄存器了。

  ```c
  &mdio {
          compatible = "ti,mdio-gmii";
          ti,mdio-mux-phy-mode = "mdio-gmii";
          ti,mdio-mux-slave-id = <0>; // 0: MDIO slave 0, 1: MDIO slave 1
          ti,mdio-mux-slave-addr = <0>; // MDIO slave address
          ti,mdio-mux-slave-addr-mask = <0>; // MDIO slave address mask
          ti,mdio-mux-master-addr = <0>; // MDIO master address

          // Export MDIO interface to sysfs
          mdio-export = <1>;
  };
  ```

  在`am65-cpsw-nuss.c`文件中添加如下配置：

  ```c
  static int am65_cpsw_nuss_probe(struct platform_device *pdev)
  {
          struct am65_cpsw_nuss *priv;
          struct resource *res;
          int ret;

          priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
          if (!priv)
                  return -ENOMEM;

          platform_set_drvdata(pdev, priv);


          res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
          priv->base = devm_ioremap_resource(&pdev->dev, res);
          if (IS_ERR(priv->base))
                  return PTR_ERR(priv->base);

          // Export MDIO interface to sysfs
          priv->mdio_export = devm_of_property_read_bool(&pdev->dev, "mdio-export");
          if (priv->mdio_export) {
                  priv->mdio_bus = devm_mdio_bus_alloc_data(&pdev->dev, priv);
                  if (IS_ERR(priv->mdio_bus))
                          return PTR_ERR(priv->mdio_bus);

                  priv->mdio_bus->name = "am65-cpsw-nuss-mdio";
                  priv->mdio_bus->priv = priv;
                  priv->mdio_bus->read = am65_cpsw_nuss_mdio_read;
                  priv->mdio_bus->write = am65_cpsw_nuss_mdio_write;

                  ret = devm_mdio_bus_register(&pdev->dev, priv->mdio_bus);
                  if (ret)
                          return ret;
          }

          return 0;
  }

  static int am65_cpsw_nuss_remove(struct platform_device *pdev)
  {
          struct am65_cpsw_nuss_priv *priv = platform_get_drvdata(pdev);

          if (priv->mdio_export)
                  devm_mdio_bus_unregister(&pdev->dev, priv->mdio_bus);

          return 0;
  }

  static const struct of_device_id am65_cpsw_nuss_of_match[] = {

          { .compatible = "ti,am65-cpsw-nuss", },
          { },
  };
  MODULE_DEVICE_TABLE(of, am65_cpsw_nuss_of_match);

  static struct platform_driver am65_cpsw_nuss_driver = {
          .driver = {
                  .name = "am65-cpsw-nuss",
                  .of_match_table = am65_cpsw_nuss_of_match,
          },
          .probe = am65_cpsw_nuss_probe,
          .remove = am65_cpsw_nuss_remove,
  };

  module_platform_driver(am65_cpsw_nuss_driver);

  MODULE_AUTHOR("Texas Instruments");
  MODULE_DESCRIPTION("AM65 CPSW Nuss MDIO driver");
  MODULE_LICENSE("GPL v2");

  
  }

  

