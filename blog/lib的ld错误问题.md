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

## 用的mingw编译程序, ld没问题，但运行时闪退，无任何输出，main函数也进不了，入口处无法下断

libxxx.dll.a和libxxx.dll在Windows系统编程中扮演着不同的角色，它们之间的主要区别在于它们的用途和适用的编译器环境。

- libxxx.dll：

.dll文件是动态链接库（Dynamic Link Library）的缩写，它是一种可执行文件，允许程序共享执行特殊任务所必需的代码和其他资源。libxxx.dll是一个具体的DLL文件，它可能包含了xxx的Windows版本中的函数和代码。
当一个应用程序或另一个库需要xxx的功能时，它会在运行时动态地加载libxxx.dll文件，并调用其中的函数。这样做的好处是减少了程序的大小，因为只有在需要时才加载DLL中的代码，同时也便于代码的共享和更新。
libxxx.dll文件通常被放置在系统的某个特定目录下，如C:\Windows\System32（对于32位DLL在64位Windows上）或C:\Windows\SysWOW64（对于32位DLL在32位Windows上）或应用程序的安装目录下。

- libxxx.dll.a：

.dll.a文件通常与MinGW（Minimalist GNU for Windows）编译器一起使用，它并不是DLL文件本身，而是一个导入库（Import Library）。导入库包含了DLL中函数的位置信息（即函数的索引或地址），但并不包含实际的函数代码。
在使用MinGW编译器链接到DLL时，编译器需要.dll.a文件来知道DLL中函数的位置，以便在生成的可执行文件中创建必要的导入表。但是，当可执行文件运行时，它实际上会加载并调用.dll文件中的函数代码。
简而言之，.dll.a文件是编译器在链接阶段使用的，而.dll文件是程序在运行时使用的。

**综上所述，libxxx.dll.a和libxxx.dll的主要区别在于它们的用途和适用的编译器环境。libxxx.dll是包含实际函数代码的动态链接库文件，而libxxx.dll.a是MinGW编译器用于链接到该DLL的导入库文件。所以在只有.dll.a的情况下随然链接通过，但运行程序时会闪退。**
