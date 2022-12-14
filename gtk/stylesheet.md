- [Style sheets](#style-sheets)
- [选择器](#选择器)
- [GTK CSS STYLE 概念介绍](#gtk-css-style-概念介绍)
  - [StyleContext](#stylecontext)
  - [Style Classes](#style-classes)
  - [StyleProvider](#styleprovider)
- [使用](#使用)
  - [使用时的一些注意事项](#使用时的一些注意事项)
    - [直接使用markup标签表达式](#直接使用markup标签表达式)
    - [关于css stylesheet中的类名和ID问题](#关于css-stylesheet中的类名和id问题)
  - [旋转](#旋转)
  - [color](#color)
- [GTK.CSS](#gtkcss)
  - [文件功能](#文件功能)
  - [scss](#scss)
  - [分类](#分类)
    - [状态](#状态)
    - [属性](#属性)
- [参考](#参考)
## Style sheets
> GTK解析的样式表的基本结构是一系列语句，它们要么是规则集，要么是“@-rules”，由空格分隔。
> 规则集由选择器和声明块组成，声明块是用花括号括起来的一系列声明。每条声明用分号分隔。  多个选择器可以共享同一个声明块，方法是将所有选择器放在块的前面，用逗号分隔。如下是两个选择器的规则集
```css
button, entry {
  color: #ff00ea;
  font: 12px "Comic Sans";
}
```
> 为了加载并解析另一个样式表，Gtk支持Css的`import`规则来引入
> 语法如下：
> 
```css
#〈import rule〉 = @import [ 〈url〉 | 〈string〉 ]
#〈url〉 = url( 〈string〉 )

@import url("path/to/common.css");
```

## 选择器
> 所有小部件都有一个或多个带有元素名称和样式类的 CSS节点。  
> 1. 当在选择器中使用样式类(style class)时，它们必须以句点为前缀(.classname) 。
> 2. 小部件的名称（widget name）也可以在选择器中以类似IDs的形式使用。在选择器中使用时，小部件名称必须以'#'字符为前缀(#widgetname)。
> 3. 在更复杂的情况下，可以以多种方式组合选择器    
>  3.1 要求一个节点满足多个条件，请将多个选择器通过连接它们组合成一个   
>  3.2 仅在某个节点出现在某个其他节点内时才匹配该节点，请将两个选择器一个接一个地写入，用空格分隔
>  3.3 要将匹配限制为父节点的直接子节点，请在两个选择器之间插入 > 字符。

具体的参考[inspector](./inspector.md#css-选择器httpswwww3schoolcomcncssrefcssselectorsasp)

## GTK CSS STYLE 概念介绍
### StyleContext
一个用来存储组件样式信息的对象(*组件在绘制时由此信息修饰*)，由所有attached到其上的sytleProviders所提供的样式信息按优先级排序组合而成最终的样式信息（*即一个styleContext可添加多个rovider组合起来*）。

- 获取一个widget的styleContext的方法：`get_style_context ()`
- 添加样式信息的方法：
    - Gtk.StyleContext.add_provider_for_screen (Screen screen, StyleProvider provider, uint priority):针对screen下的所有widget, `screen=Gdk.Screen.get_default ()`
    - Gtk.StyleContext.add_provider (StyleProvider provider, uint priority):针对单一widget

### Style Classes
widget的styleContext中的样式信息可以通过样式类名来区分不同的样式，GTK定义了很多样式类的宏可以直接使用(eg:"GTK.STYLE_CLASS_XX")。

- 如下css信息,类名为class_name
    ```css
    .class_name { 
        color:red;
    }
    .flat {
        border:none;
    }
    ...
    .xx {
        xx:xx;
    }

    ```
- widget使用styleContext信息中的某个样式类的方法：`add_class (string class_name) `

### StyleProvider
GtkStyleProvider是一个用于向StyleContext提供样式信息的接口,具体的实现子类可见CssProvider,用于加载解析Css样式表内容

注：**GTK的默认加载**，详见[GtkCssProvider](https://valadoc.org/gtk+-3.0/Gtk.CssProvider.html)
   > GTK+初始化时会默认读取加载入特定的几个css文件:
   > 1. $XDG_CONFIG_HOME/gtk-3.0/gtk.css (如果存在)
   > 2. 从下面依次搜索并读取出第一个存在的gtk.css文件. 
   >    - XDG_DATA_HOME/themes/THEME/gtk-VERSION/gtk.css
   >    - $HOME/.themes/THEME/gtk-VERSION/gtk.css
   >    - $XDG_DATA_DIRS/themes/THEME/gtk-VERSION/gtk.css
   >    - DATADIR/share/themes/THEME/gtk-VERSION/gtk.css
   > 3. 读取载入gtk-keys.css(同上)
   >
   > 其中: 
   > - THEME: the name of the current theme (Gtk.Settings.gtk_theme_name())
   > - VERSION: the GTK+ version number
   >

- CssProvider提供的加载方法
  ```vala
    public bool load_from_buffer (uint8[] data) throws Error
    public bool load_from_data (string data, ssize_t length = -1) throws Error
    public bool load_from_file (File file) throws Error
    public bool load_from_path (string path) throws Error
    public void load_from_resource (string resource_path) 
  ```
## 使用
- 步骤
1. 使用provider加载css sheetstyle data
2. widget添加provider,加载的样式信息到其styleContext中
3. widget使用加载入context中的某一样式类 (GTK+宏定义样式类的可以直接加，GTK+初始化时已经载入)
- example
  ```vala
    var provider = new Gtk.CssProvider ();
    var provider_gres = new Gtk.CssProvider ();
    var css_data = "levelbar.horizontal block{ min-width:10px; }";
    try {
        provider.load_from_data (css_data, css_data.length);
        level_bar.get_style_context ().add_provider (provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch (Error e) {
        message (e.message);
    }
    try {
        provider_gres.load_from_resource("/org/path/to/resource/css.xml");
        level_bar.get_style_context ().add_provider (provider_gres, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch (Error e) {
        message (e.message);
    }
    level_bar.get_style_context ().add_class(GTK.STYLE_CLASS_FLAT);
  ```

### 使用时的一些注意事项
#### 直接使用markup标签表达式   
`logo_text.set_markup ("\<span weight='bold' color='white' font_desc='16'>Jide OS\</span>")`    
具体使用参见Pangoy库的[Text Attributes and Markup](https://docs.gtk.org/Pango/pango_markup.html)

#### 关于css stylesheet中的类名和ID问题
以下是一个css.xml
  ```css
  .class_name { 
      color:red;
  }
  button#settings_restore {
    color:red; 
  }
  ```
1. 如何使用该css.xml 
   - way1. label设置支持markup才能使用css标签如：   
      ```vala
      var logo_text = new Gtk.Label ("<b>Jide Os</b>") {
          use_markup = true,
          name = "settings_restore" //logo_text.set_name("settings_restore") 无效，对控件id操作命名与set_name无关
      };
      ```
   - way2. 在xml中使用 `#id` 直接指定具体的widget,然后加入该样式表到该widget的context中: `logo_text.get_style_context ().add_provider(xx,xx)`;    
  
   - way3. 使用style class标记，然后载入该样式并指定具体应用的类名：`logo_text.get_style_context ().add_class ("class_name")`;
  
2. `#id`: CSS中id为控件的name，而`set_name`，`get_name`是对控件id的操作命名,与name无关。见下面的button.xml
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
### 旋转
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
### color
- 颜色值的指定方式：
  ```text
  〈color〉 = currentColor | transparent | 〈color name〉 | 〈rgb color〉 | 〈rgba color〉 | 〈hex color〉 | 〈gtk color〉

  〈rgb color〉 = rgb( 〈number〉, 〈number〉, 〈number〉 ) | rgb( 〈percentage〉, 〈percentage〉, 〈percentage〉 )
  〈rgba color〉 = rgba( 〈number〉, 〈number〉, 〈number〉, 〈alpha value〉 ) | rgba( 〈percentage〉, 〈percentage〉, 〈percentage〉, 〈alpha value〉 )
  〈hex color〉 = #〈hex digits〉

  〈alpha value〉 = 〈number〉
  ```
  example:
  ```css
  color: transparent;
  background-color: red;
  border-top-color: rgb(128,57,0);
  border-left-color: rgba(10%,20%,30%,0.5);
  border-right-color: #ff00cc;
  border-bottom-color: #ffff0000cccc;
  ```
- 颜色变量的定义和引用
   ```CSS
    # 变量名定义
    @define-color COLOR_NAME color_value
    .class_name{ 
        color: @COLOR_NAME;
    }

  ```
  code中引用如下：
   ```vala
    var provider = new Gtk.CssProvider ();
    var colored_css = "@define-color COLOR_NAME red";
    provider.load_from_data (colored_css, colored_css.length);
    context.add_provider (provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
    context.add_class ("class_name");
   ```

## GTK.CSS
### 文件功能
- 所在位置：/usr/share/theme/theme-name/gtk-3.0/gtk[-dark].css
- 影响系统gtk控件的所有类别class在定义在此文件中
### scss

### 分类
#### 状态 
- 定义方法：类名（或&）+“:”+状态
- 状态列表

|状态名|解释|
|:-|-|
backdrop|在其它窗口后，非当前窗口
active| 被点击时
disabled| 被禁用，sensitive为false时
hover|鼠标放置于上时
prelight|小部件上有一个鼠标指针
checked|选中


#### 属性
|属性名|解释|
|-|-|
-gtk-icon-theme|图标，在button的background-image用此指定图标时图标当作前景图。color可以直接改图标颜色
-gtk-icon-shadow|图标阴影颜色
background-clip|规定背景的绘制区域,有content-box（内容框）, padding-box（内边距框）, border-box（边框盒）三种
borde-radius | 1个占位点：代表四角<br>2个占位点：上左下右\   上右下左/<br>4个占位点:按顺时针方向排序，即 上左 上右 下右 下左 eg: border-radius:0 10px 10px 20px
margin | 1个占位点：代表四角 <br>2个点位点: 上下，左右<br>4个占位点：上右下左，顺时针

- box:
  ![box](./../picture/gtk/box.png)

- scale:
  - slider:小球
  - trough:滑槽
  - highlight:已滑过的槽体
  - fill:剩下未滑过的槽体
  - indicator:

## 参考
1. [gtk3 css-overview](https://docs.gtk.org/gtk3/css-overview.html)
1. [gtk3 Supported CSS Properties](https://docs.gtk.org/gtk3/css-properties.html)
