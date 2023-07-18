## mermaid example
插入甘特图
-----------------
```mermaid
gantt
       
        title Adding GANTT diagram functionality to mermaid
        section 现有任务
        已完成               :done,    des1, 2014-01-06,2014-01-08
        进行中               :active,  des2, 2014-01-09, 3d
        计划中               :         des3, after des2, 5d
```

插入UML图
------------
```mermaid
sequenceDiagram
张三 ->> 李四: 你好！李四, 最近怎么样?
李四-->>王五: 你最近怎么样，王五？
李四--x 张三: 我很好，谢谢!
李四-x 王五: 我很好，谢谢!
Note right of 王五: 李四想了很长时间, 文字太长了<br/>不适合放在一行.
 
李四-->>张三: 打量着王五...
张三->>王五: 很好... 王五, 你怎么样?
```

插入Mermaid流程图
--------
```mermaid
graph LR
A[长方形] -- 链接 --> B((圆))
A --> C(圆角长方形)
B --> D{菱形}
C --> D
```


 

## asdfasd

1. 电源相关的设置：org.gnome.settings-daemon.plugins.power


2. 播放源码
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

```

3. 系统提示音开关
``` xml
  <schema id="io.patapua.settings.sound" path="/io/patapua/settings/sound/">
    <key name="login-play" type="b">
      <default>false</default>
      <summary>play login/logout sound</summary>
      <description>whether to play sound when login or logout</description>
    </key>
  </schema>

```
4. 注销相关的设置（screensaver屏保）
- setting path:/org/gnome/desktop/screensaver/
   - logout_enable
   - logout_command


5. ` sudo apt install gnome-session-canberra `

6. `paplay /usr/share/sounds/ubuntu/stereo/desktop-login.ogg`


7. sh
```sh
#!/bin/sh
ret=`gsettings get org.gnome.desktop.sound event-sounds`
echo $ret
if [ $ret = "true"  ] ;then
	paplay /usr/share/sounds/freedesktop/stereo/bell.oga 
fi
```



8. systemed
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
原文链接：https://blog.csdn.net/z1026544682/article/details/104538239



usr: gsd-sound.target

## 更改插件名称
1. 改依赖包名
2. 改PLUG文件
   1. 改类型名
   2. 改code_name
3. 改安装路径
4. 摸索修改
`grep switchboard . -rn --exclude-dir={build,.git,.github} --exclude=*.{po,pot} `

5. 修改翻译文件
 - 修改主工程的meson.build中的gettext_name 
```
gettext_name = 'settingboard-plug-' + meson.project_name() 
```
 - rename  xml中的translation type
 - 寻找是否有`GLib.Intl.bindtextdomain`函数，找出定义的宏参数修改
  

##  screensaver
`xdg-screensaver`
`mplayer -fs -nosound -noautosub -loop 0 -fixed-vo`
``
[add xnoise-screensaver-manager.vala ](https://github.com/jozadaquebatista/xnoise/commit/84ec07c29d34d10c629acda813202afe7e34c7fe)

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

### 终端页面无法回滚，不能使用上和pageup键回看,原因是pager默认配置为more,改为less即可
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

```
    public int monitor_number { get { return monitors.size; } }
    public int virtual_monitor_number { get { return virtual_monitors.size; } }
    public bool is_mirrored { get { return virtual_monitors.size == 1 && monitors.size > 1; } }
```
- xranr: 调节显示的命令行程序

```xranr --output xxx --brightness 0.8```

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


## unable to login  in 
```sh
apt install gnome-session
apt remove gnome-session
```
--------------------
## settingboard -- network
设备状态变更信号传导图
-------

```mermaid
graph LR
A(device.state_changed)-->B(infoBox.info_changed)-->C{page.update} -.override.->D & E & F & H & I -.-> J[update_switch:更改status_switch.active]
E[HotspotPage.update]
D[EthernetPage.update]
F[ModemPage.update]
H[VPNPage.update]
I[WifiPage.update]
```

**status_switch**开关设置的信号传导图
------------

```mermaid
graph LR
A(status_swtich.active)--notify-->B[control_switch_activated]
B--false-->C[device:disconnect]--failed-->Ab[active=true]-->A
B--yes-->D[device:connect]
D-->E(device.state_changed)
```

------------------
## find git repository
```sh
for file in `ls` ; do cd $file; if [ -d .git ] ;then  git remote -v | grep fetch | grep leixa | awk {'print $2'}; fi; cd ->/dev/null ;done

#install build dependency
for i in `ls ./settingboard* --directory`;do  cd $i; sudo apt build-dep .;cd ->/dev/null;done


```
## gtk theme

###  flatpak app无法使用系统的theme
1. [Applying GTK themes to Flatpak applications](https://itsfoss.com/flatpak-app-apply-theme/)
   - Step 1: Give Flatpak apps access to GTK themes location
     - all app:`sudo flatpak override --filesystem=$HOME/.themes`
     - single app:`sudo flatpak override org.gnome.Calculator --filesystem=$HOME/.themes`
   - Step 2:Tell Flatpak apps which theme to use
     - all app:`sudo flatpak override --env=GTK_THEME=my-theme `
     - single app:`sudo flatpak override org.gnome.Calculator --env=GTK_THEME=my-theme `
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
> * Setting @decorated to %FALSE hints the desktop environment
> * that the surface has its own, client-side decorations and
> * does not need to have window decorations added.
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
- https://docs.aws.amazon.com/cli/latest/reference/s3/


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

## 招人评分参考：

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



##  3588
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
io.elementary.screenshot* io.elementary.videos* org.gnome.fileroller*

org.gnome.desktop.background picture-uri


## 7.25
wallpapers
patapua-default-settings
metapackages

##  7.26
显示已安装软件及版本
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
链接：https://pan.baidu.com/s/15hXLJ5Wbi-5sJipF_k1Ihw?pwd=1234 提取码：1234
2、Google driver
https://drive.google.com/drive/folders/1GWlPj3D8SrceV_nRhTBNdxULQ8FtAtRK?usp=sharing

 
ITX-3588J，Linux sdk：
1、百度网盘：
链接：https://pan.baidu.com/s/1w1fd31vgxkyFmRBOs_Wu_g?pwd=1234 
提取码：1234 
2、Google driver
https://drive.google.com/drive/folders/1LovVy481bNUfbZpoCMSAeDEzIZmvZNKH?usp=sharing

## grub 
### 启动步骤：
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
1.  打开网络设置代理设置，勾选手动模式，在http proxy, https proxy和ftp proxy上都填写上代理如 : "username:passwd@proxy_ip", 在端口号上填写对应端口如3128
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

### window

1. powershell 

   ```powershell
   netsh winhttp set proxy http://proxy.example.com:port
   ```

## open remote github repository

1. install extension: remote repositories
2. `ctrl+shitf+p` to choose "Remote Repositories: Open ..." or click left corner >< icon
3. open github repository

## conntrack 

作用：
> command line interface for netfilter connection tracking

conntrack 输出含义

position |ip:port | description
-|-|-|
1 | protocol | <li>tcp:6 <li>udp:17
2 | unkown |
origin | src | origin src ip
origin | dst | origin dst ip
origin | sport | origin src port
origin | dport | origin dst port
expect | src | expect reply src ip， 一般与origin dst相同
expect | dst | expect reply dst ip， 一般与origin src相同,有NAT转换与转换ip相同 
expect | sport | expect reply origin src port， 一般与origin dport相同
expect | dport | expect reply origin dst port， 一般与origin sport相同，有NAT转换，与转换port相同


```sh
/ # conntrack -L
tcp      6 3508 ESTABLISHED src=192.168.10.170 dst=17.57.145.132 sport=49417 dport=5223 src=17.57.145.132 dst=10.228.118.101 sport=5223 dport=49417 [ASSURED] mark=0 use=1
tcp      6 20 LAST_ACK src=192.168.10.170 dst=17.248.221.12 sport=49474 dport=443 src=17.248.221.12 dst=10.228.118.101 sport=443 dport=49474 [ASSURED] mark=0 use=1

```

解释：
 
上述显示结果中，每行表示一个连接跟踪项。

每行相同的地址和端口号会出现两次，而且第二次出现的源地址/端口对和目标地址/端口对会与第一次正好相反！

这是因为每个连接跟踪项会先后两次被插入连接状态表。

第一个四元组（源地址、目标地址、源端口、目标端口）记录的是原始方向的连接信息，即发送者发送报文的方向。

第二个四元组则记录的是连接跟踪子系统**期望收到**的对端回复报文的连接信息。

这解决了两个问题：

- 如果报文匹配到一个NAT规则，例如IP地址伪装，相应的**映射信息**会记录在链接跟踪项的**回复方向部分**，并自动应用于**同一条流**的**所有**后续报文。（即NAT修改后的信息可从期望回复方向部分找出来）

- 即使一条流经过了地址或端口的转换，也可以成功在连接状态表中查找到回复报文的四元组信息。

**原始方向的（第一个显示的）四元组信息永远不会改变：它就是发送者发送的连接信息。**

NAT 操作只会修改回复方向（第二个）四元组，因为这是接受者看到的连接信息。

修改第一个四元组没有意义：netfilter 无法控制发起者的连接状态，它只能在收到/转发报文时对其施加影响。

**当一个报文未映射到现有连接表项时，连接跟踪可以为其新建一个表项。**

- 对于 UDP 报文，该操作会自动进行。

- 对于 TCP 报文，连接跟踪可以配置为只有 TCP 报文设置了 SYN 标志位 才新建表项。

 默认情况下，连接跟踪会允许从流的中间报文开始创建，这是为了避免对启用连接跟踪之前就存在的流处理出现问题。

## 检测是否在docker内

```sh
ps --no-headers --pid 1 | grep --silent docker-init && in_docker=1 || in_docker=0
[ $in_docker = 1 ] && {
        PS1='\[\033[01;36m\][docker] '$PS1
}
```
