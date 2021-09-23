- [命令](#命令)
- [路径和名称](#路径和名称)
- [资源文件的引入](#资源文件的引入)
- [Config.vala 文件自动生成](#configvala-文件自动生成)
- [翻译](#翻译)
## 命令
1. 查看当前配置状态 (build为构建目录)
`meson configure build`

2. 译文安装
```sh 
i18n.merge_file(
    input: 'sound.appdata.xml.in',
    output: 'com.patapua.settingboard.sound.appdata.xml',
    po_dir: join_paths(meson.source_root(), 'po', 'extra'),
    type: 'xml',
    install: true,
    install_dir: join_paths(datadir, 'metainfo'),
)
```

## 路径和名称
1. 源代码root路径： `meson.source_root() `
1. 当前路径 :   `meson.current_source_dir()`
1. 安装路径： `install_dir: join_paths('dir', 'sub', 'low')` 合成为"/dir/sub/low/"
1. 工程名： `meson.project_name()`

## 资源文件的引入
- 文件所在位置
```sh
ann@B460M-d5c6e3b7:settingboard-plug-sound$ ls data/
icons.gresource.xml
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
# step 3 : 资源文件加到工程中
shared_module(
    meson.project_name(),
    'test.vala',
    plug_resources,
)


```

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

## 翻译
- step 1 : 在工程的./meson.build 中 import i18n
    ```sh
    # Include the translations module
    i18n = import('i18n')
    gettext_name = meson.project_name() # 其它也行，生成模板时使用此名
    # Set our translation domain
    add_global_arguments('-DGETTEXT_PACKAGE="@0@"'.format (gettext_name, language:'c')

    subdir('data')
    ```

- step 2 : 在./data/meson.build 中 merge_file desktop和appdata文件
    
    merge_file 翻译和安装，可替代install_data
    ```sh
    #Translate and install our .desktop file
    i18n.merge_file(
        input: 'data' / 'hello-again.desktop.in',
        output: meson.project_name() + '.desktop',
        po_dir: meson.source_root() / 'po',
        type: 'desktop',
        install: true,
        install_dir: get_option('datadir') / 'applications'
    )

    #Translate and install our .appdata file
    i18n.merge_file(
        input: 'data' / 'hello-again.appdata.xml.in',
        output: meson.project_name() + '.appdata.xml',
        po_dir: meson.source_root() / 'po',
        install: true,
        install_dir: get_option('datadir') / 'metainfo'
    )

    ```
    **.appdata.xml 文件中要指定使用的翻译文件名需与gettext_name一致** 

- step 3 :  在工程root目录下创建po,并引入po下的meson.build

    ```sh
    subdir('po')
    ```
    ./po/meson.build
    ```sh
    i18n.gettext(gettext_name,
        args: '--directory=' + meson.source_root(),
        preset: 'glib'
    )
    ```
- step 4 : 完善目录po下的文件
   -  创建 POTFILES ： 包含要翻译的所有文件
   -  创建 LINGUAS  ： 它应该包含您要为其提供翻译的所有语言的两个字母的语言代码
   -  返回构建目录并运行命令行生成翻译模板：
        ```
        ninja translate-file-pot
        ``` 
        运行后会在po下产生一个包含所有待翻译字符串的文档

   - 在构建目录中运行命令生成LINGUAS所列的所有语言的翻译文档
        ```sh
        ninja translate-file-update-po
        ```

- step 5 : 每次更新改变待翻译字串后都需要在构建目录下运行`ninja *-pot` 和 `ninja *-update-po`两条指令
