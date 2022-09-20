- [- 手动添加编译选项](#--手动添加编译选项)
- [命令](#命令)
- [路径和名称](#路径和名称)
- [资源文件的引入](#资源文件的引入)
- [添加schemas](#添加schemas)
- [Config.vala 文件自动生成](#configvala-文件自动生成)
- [翻译](#翻译)
  - [构成目录](#构成目录)
  - [命令](#命令-1)
  - [i18n模块介绍](#i18n模块介绍)
  - [工程翻译步骤](#工程翻译步骤)
  - [问题集](#问题集)
- [翻译与Config相结合](#翻译与config相结合)
- [手动在vala工程中添加vapi （以添加sdl2_image为例)](#手动在vala工程中添加vapi-以添加sdl2_image为例)
  - [1. 在工程中添加vapi目录](#1-在工程中添加vapi目录)
  - [2. 添加vapi文件](#2-添加vapi文件)
  - [3. 在meson.build中添加 **'--vapidir'**](#3-在mesonbuild中添加---vapidir)
  - [4. 在meson.build中添加对应库, 参数本章命令节第二条](#4-在mesonbuild中添加对应库-参数本章命令节第二条)
- [手动添加编译选项](#手动添加编译选项)
-------------
## 命令
1. 查看当前配置状态 (build为构建目录)
`meson configure build`
2. 添加lib `dependency('libcanberra')`
   - `pkg-config --list-all` 列出所有可用库
   - `pkg-config --libs libname`, 去掉-l就是要添加到dependency中间的名字，大小写敏感
   - 都找不到就用meson查找命令：
        ```meson
        m_dep = meson.get_compiler('c').find_library('m', required : false)
        posix_dep = meson.get_compiler('vala').find_library('posix')
        # 其中posix是由valadoc中的某个函数的 Package:posix 字段得到。比如查找Posix.sleep就能看到
        ```
3. 获取参数值：`get_option('prefix')`
4. foreach
   ```meson.build
    schemas = [ '...', '...' ]
    generated_schemas = []                                                                                     
    foreach schema: schemas
        generated_schemas += configure_file(
            output: schema,
            input: schema + '.in',
            configuration: conf,
        )
    endforeach

    install_data(generated_schemas, install_dir: schemasdir)

   ```
5. custom target
   ```meson.build
    custom_proc = find_program('xsltproc', required: false)
    assert(custom_proc.found(), 'xsltproc is required to build documentation')
    custom_proc_cmd = [
        custom_proc,
        '--output', '@OUTPUT@',
        '...',
        '@INPUT@'
    ]
    output = meson.project_name() + '.1'
    custom_target(
        target_name, #工程名
        input : 输入,
        output : 输出,
        command : custom_proc_cmd, # 执行的命令
        install : true,
        install_dir : join_paths(control_center_mandir, 'man1')
    )

   ```
6. 工程, 有executable, shared_module, shared_lib
   ```
   executable(
        meson.project_name(),
        sources,
        include_directories : include_directories('.'),
        dependencies : shell_deps,
        c_args : cflags,
        link_with : panels_libs,
        install : true
    )
   ```
7. run_command
    ```
    ls = find_program('ls')
    result = run_command(ls, join_paths(meson.source_root (), directory, size))
    if (result.returncode() == 0)
        message ('success')
    else
        message ('failed')
    endif

    ```
--------------
## 路径和名称
1. 源代码root路径： `meson.source_root() `
1. 工程名： `meson.project_name()`
1. 当前路径 :   `meson.current_source_dir()`
1. 包含include目录: `include_directories('.')`
1. 安装路径： `install_dir: join_paths('dir', 'sub', 'low')` 合成为"/dir/sub/low/"

    ```vala
    //icon install
    install_data(
        'wingpanel.svg',
        rename: meson.project_name() + '.svg',
        install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps')
    )
    //autostart .desktop install
    install_data(
        'autostart.desktop',
        rename: meson.project_name() + '.desktop',
        install_dir: join_paths(get_option('sysconfdir'), 'xdg', 'autostart')
    )
    // .desktop install
    i18n.merge_file(
        input: 'wingpanel.desktop.in',
        output: meson.project_name() + '.desktop',
        po_dir: join_paths(meson.source_root(), 'po', 'extra'),
        type: 'desktop',
        install: true,
        install_dir: join_paths(get_option('datadir'), 'applications')
    )
    // appdata.xml install
    i18n.merge_file(
        input: 'wingpanel.appdata.xml.in',
        output: meson.project_name() + '.appdata.xml',
        po_dir: join_paths(meson.source_root(), 'po', 'extra'),
        type: 'xml',
        install: true,
        install_dir: join_paths(get_option('datadir'), 'metainfo'),
    )
    // gschema.xml install
    install_data(
        'io.elementary.switchboard.locale.gschema.xml',
        install_dir: join_paths(datadir, 'glib-2.0', 'schemas')
    )
    # Install Dbus Serivce file
    configure_file(
        input: 'my.service',
        output: meson.project_name()+'.service',
        install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services')
    )
    ```  
-------------

## 资源文件的引入
- 文件所在位置
    ```sh
    ann@B460M-d5c6e3b7:settingboard-plug-sound$ ls data/
    icons.gresource.xml
    css.gresource.xml
    ```
- meson.build
    ```sh
    # step 1 : 引入gnome模块
    gnome = import('gnome')
    # step 2 ： 添加资源到文件
    plug_resources = gnome.compile_resources(
        'plug_resources',
        'data/icons.gresource.xml',
        source_dir: 'data'
    )
    css_resources = gnome.compile_resources(
        'plug_resources',
        'data/css.gresource.xml',
        source_dir: 'data'
    )
    # step 3 : 资源文件加到工程中
    shared_module(
        meson.project_name(),
        'test.vala',
        plug_resources,
        css_resources,
    )
    ```
- 在源码中的引用
  - 在css文件中引用
    ```css
    .category.graphics label {
        border-image:
            -gtk-scaled(
                /* stylelint-disable-next-line function-comma-newline-after (bug in stylelint?) */
                url("resource:///io/elementary/appcenter/backgrounds/graphics.svg"),
                url("resource:///io/elementary/appcenter/backgrounds/graphics@2x.svg")
            ) 10 10 10 10 / 10px 10px 10px 10px repeat;
        padding: 12px;
    }

    ```
  - 在vala中的引用    
    ```vala
    weak Gtk.IconTheme default_theme = Gtk.IconTheme.get_default ();
    default_theme.add_resource_path ("/io/elementary/appcenter/icons");
    var icon = new GLib.ThemedIcon ("category");

    var arrow_provider = new Gtk.CssProvider ();
    arrow_provider.load_from_resource ("io/elementary/appcenter/arrow.css");

    // set_resource_base_path (resource_path)
    ```
-------------
## 添加schemas
 - 在工程schemas或data目录中添加*.gschema.xml文件
 - 在gschema.xml同层的meson.build中加入如下内容,安装到系统指定schemas目录中
    ```meson
    install_data(
        'io.elementary.switchboard.locale.gschema.xml',
        install_dir: join_paths(datadir, 'glib-2.0', 'schemas')
    )
    ```
 - 新建meson目录，创建编译脚本post_install.py
    ```python
    #!/usr/bin/env python3

    import os
    import subprocess

    schemadir = os.path.join(os.environ['MESON_INSTALL_PREFIX'], 'share', 'glib-2.0', 'schemas')

    if not os.environ.get('DESTDIR'):
        print('Compiling gsettings schemas...')
        subprocess.call(['glib-compile-schemas', schemadir])

    ```
- 在主meson.build中添加`meson.add_install_script('meson/post_install.py')`,执行编译脚本,使之生效

-------------
## Config.vala 文件自动生成

- 在目录“src”下创建“Config.vala.in”文件 , 内容如下：   
    ```vala
    public const string GETTEXT_PACKAGE = @GETTEXT_PACKAGE@;
    public const string LOCALEDIR = @LOCALEDIR@;
    ```
- meson.build
    ```sh
    # step 1: 设置模式值到config_data中
    config_data = configuration_data()
    config_data.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
    config_data.set_quoted('GETTEXT_PACKAGE', meson.project_name() + '-plug')
    # config_data.set('GETTEXT_PACKAGE', meson.project_name() + '-plug') 此方法在vala中需要加上“”号，如 ”@LOCALEDIR@“

    # step 2: 引入模式文件
    config_file = configure_file(
        input: 'src/Config.vala.in',
        output: '@BASENAME@',
        configuration: config_data
    )

    # step 3: 将模式文件加入工程
    shared_module(
        meson.project_name(),
        'test.vala',
        config_file,
    )
    ```
-  编译后会在src/下自动生成Config.vala文件, 内容如下：
    ```vala
    public const string GETTEXT_PACKAGE = "sound-plug";
    public const string LOCALEDIR = "/usr/share/locale";
    ```
 - 代码中的使用
    ```vala
    GLib.Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    GLib.Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    ```

--------------
## 翻译
### 构成目录   
```sh
project
|--meson.build
|--data
    |--meson.build
    |--各种待翻译的资源文件(eg:*.appdata.xml.in,*.desktop.in)
|--po
    |--meson.build
    |--POTFILES
    |--LINGUAS
    |--extra
        |--meson.build
        |--POTFILES
        |--LINGUAS
```
   - POTFILES: 内容为要翻译的所有源文件路径名，换行符分隔
   - LINGUAS:  它应该包含您要为其提供翻译的所有语言的两个字母的语言代码，换行符分隔
   - extra下一般为不需安装的翻译,故meson.build中i18n.gettext的install字段为false
  
### 命令
 - ninja \*-pot : 从POTFILES获取所有待翻译的文档，生成译文模板文档 "\*.pot"
 - ninja \*-update-po  : 生成LINGUAS所列的所有语言的翻译文档 "\*.po"
 - ninja \*-gmo : 编译"\*.po"成"\*.mo"

    **注意，要在构建目录中运行**, 上面的“*”为传递给gettext()方法的第一个字符参数。
###  [i18n](https://mesonbuild.com/i18n-module.html)模块介绍
该模块提供国际化和本地化功能, 在meson中引入该模块的方法： `i18n = import('i18n')` , 前面一个i18n为对象名，可任意命名，
提供的方法有：
- i18n.gettext(): 设置gettext本地化，以便在工程**安装期间**构建翻译并安装翻译到合适的位置
    ```sh
    i18n.gettext(
        gettext_name, #生成的pot名称的参数, 也是传递给 *-pot/update-po/-gmo 命令的参数
        args: '--directory=' + meson.source_root(), # 当前POTFILE中文件的相对路径(猜测,未证实)
        preset: 'glib', # 当前只有此项
        install: true, # 此为默认值
        install_dir: get_option('localedir') # 此为默认值
        languages : 'zh', # 可选项，不填则读取 LINGUAS的值
    )

    ```
- i18n.merge_file() : 将翻译合并到一个文本文件中并输出output
    ```bash
    i18n.merge_file(
        input: # 待翻译的文件
        output: # 加入翻译后的输出文件名
        po_dir: meson.source_root() / 'po', #包含po翻译文件的目录,  将input文件同时添加到当前po_dir下的POTFILES中
        type: 'desktop', # 当前可选项只有desktop, xml(默认此值)
        install: true, 
        install_dir: get_option('datadir') / 'applications'
    )
    ```

### 工程翻译步骤

- step 1 : 在工程的./meson.build 中 引入 i18n 模块
    ```sh
    # Include the translations module
    i18n = import('i18n')

    # 设置翻译文件名称，此处取工程名，也可设置其它值，用于传递给i18n.gettext()的参数， 生成模板时使用此名(eg: ninja gettext_name-pot)
    gettext_name = meson.project_name() 
    # Set our translation domain, 设置使用的翻译文件名称 gettext_name.mo
    add_global_arguments('-DGETTEXT_PACKAGE="@0@"'.format (gettext_name, language:'c')
    # 或者 add_project_arguments( '-DGETTEXT_PACKAGE="@0@"'.format(gettext_name), language:'c')

    # 具体翻译子工程在data,po中
    subdir('data') 
    subdir('po')
    ```

    也可在vala代码中设置使用翻译文件, 宏的引入参考[Config.vala 文件自动生成](#configvala-文件自动生成)
    ```vala
    GLib.Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    GLib.Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8")
    ```


- step 2 : 在./data/meson.build 中 merge_file .desktop和appdata.xml文件
    
    merge_file方法翻译和安装，可替代install_data
    ```sh
    #Translate and install our .desktop file
    i18n.merge_file(
        input: 'data' / 'hello-again.desktop.in',
        output: meson.project_name() + '.desktop',
        po_dir: meson.source_root() / 'po', # hello-again.desktop.in添加到此目录下的POTFILES
        type: 'desktop',
        install: true,
        install_dir: get_option('datadir') / 'applications'
    )
    #Translate and install our .appdata file
    i18n.merge_file(
        input: 'data' / 'hello-again.appdata.xml.in',
        output: meson.project_name() + '.appdata.xml', 
        po_dir: meson.source_root() / 'po', # hello-again.appdata.xml.in添加到此目录下的POTFILES
        install: true,
        install_dir: get_option('datadir') / 'metainfo'
    )

    ```
    **.appdata.xml 文件中要指定使用的翻译文件名需与gettext_name一致** 
    ```xml
    <translation type="gettext">gettext_name</translation>
    ```

- step 3 :  在工程root目录下创建po,po下的meson.build如下
    ```sh
    i18n.gettext(gettext_name, #生成的pot名称
        args: '--directory=' + meson.source_root(), 
        preset: 'glib'
    )
    ```

- step 4 : 完善目录po下的文件
   -  创建 POTFILES ： 包含要翻译的所有文件
   -  创建 LINGUAS  ： 它应该包含您要为其提供翻译的所有语言的两个字母的语言代码
   -  返回构建目录并运行命令行生成翻译模板, 运行后会在po下产生一个包含所有待翻译字符串的文档
        ```
        ninja gettext_name-pot
        ``` 

   - 在构建目录中运行命令生成LINGUAS所列的所有语言的翻译文档
        ```sh
        ninja gettext_name-update-po
        ```
        gettext_name替换为具体翻译文件名称

- step 5 : 每次源代码中更新改变待翻译字串后都需要在构建目录下运行`ninja *-pot` 和 `ninja *-update-po`两条指令
### 问题集
1. 将po翻译文件转成二进制文件时报错: 无效的多字节序列
    
    resolved: 需要将对应的.po文件中charset值从ASCII修改为UTF-8，再执行上述命令即可！
## 翻译与Config相结合
```meson
# AppData file
appdata_in = configure_file(
    input: 'my.appdata.xml.in.in',
    output: meson.project_name() + '.appdata.xml.in',
    configuration: conf_data
)
i18n.merge_file(
    input: appdata_in,
    output: meson.project_name() + '.appdata.xml',
    po_dir: join_paths(meson.source_root (), 'po', 'extra'),
    type: 'xml',
    install_dir: join_paths(get_option('datadir'), 'metainfo'),
    install: true
)
```

---------------
## 手动在vala工程中添加vapi （以添加sdl2_image为例)
### 1. 在工程中添加vapi目录
```sh
ann@dell:appstore$ mkdir vapi
ann@dell:appstore$ ls
build  builddir  data  debian  meson  meson.build  po  README.md  src  vapi
```
### 2. 添加vapi文件
查看SDL2_image.deps，得知SDL2_image只依赖sdl2, 则添加如下两个vapi文件
```sh
ann@dell:appstore$ ls vapi/
SDL2_image.vapi  sdl2.vapi
```
### 3. 在meson.build中添加 **'--vapidir'**
```sh
vapi_dir = join_paths(meson.current_source_dir(), 'vapi')
add_project_arguments(['--vapidir', vapi_dir], language: 'vala')
```
### 4. 在meson.build中添加对应库, 参数本章命令节第二条
```sh
dependencies:[
    dependency ('sdl2'),
    dependency ('SDL2_image')
    ]
    
```
## 手动添加编译选项
1. 在工程目录下创建meson_options.txt文件，如下：
    ```txt
    option('example', type: 'boolean', value: false, description: 'Build an example that shows how it works')
    ```
2. 在工程中获取配置项：
   ```meson
   if get_option('example')
    subdir('sample')
   endif
   ```
3. 更改配置项，编译工程
    ```sh
    # 配置为true
    meson configure build -D example=true
    # 查看配置是否正确
    meson configure build
    ```
