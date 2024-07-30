# lib的ld错误问题

## ld: cannot find -lQt5DBus: 没有那个文件或目录

一般就是表面意思，ld时寻找不到库文件，如果文件名对了，大部分是路径不对，可在前面加上`-L/path/to/lib/dir/`
QT中可将寻找路径加到`QT_LIBDIR += /path/to/lib/dir/`

## ld: 时报“undefined reference to xxx”错

可能原因如下：

- 未加入引用库
- 引用的申明类型不对，此与编译器有关

    错误信息还多了.text字段，如下:
    > (.text.startup+0x14): undefined reference to 

    此类情况大致是因为函数声明的前置修饰不对，如有以下头文件，发现在linux环境下编译如果用了WIN32的修饰则报此错，即用linux编译器时不能申明为**dllimport**，同样，在win编译下编译时如果使用的是**visibility**也报一样的错。

    ```c
    #ifdef _WIN32
    #  define Q_DECL_EXPORT     __declspec(dllexport)
    #  define Q_DECL_IMPORT     __declspec(dllimport)
    #else
    #  define Q_DECL_EXPORT     __attribute__((visibility("default")))
    #  define Q_DECL_IMPORT     __attribute__((visibility("default")))
    #  define Q_DECL_HIDDEN     __attribute__((visibility("hidden")))
    #endif

    #if defined(SIMULATOR_LIBRARY)
    #  define SIMULATOR_EXPORT Q_DECL_EXPORT
    #else
    #  define SIMULATOR_EXPORT Q_DECL_IMPORT
    #endif

    #define SIMULATOR_CALL __cdecl

    SIMULATOR_EXPORT int SIMULATOR_CALL sdk_init();

    ```

- **引用库的顺序**有问题，导致前后依赖错误，要注意被引用的库要放到引用者之后。
