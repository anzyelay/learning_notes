# 内存异常调试方法

## 常见的几个内存问题分析工具

- Valgrind (Linux)：检测内存泄漏、越界访问。
- ASan (AddressSanitizer)：编译时开启，快速定位越界、UAF（Use After Free）。
- GDB：结合 watchpoint 监控内存变量变化。
- [日志 + 内存填充](../linux-coding-c/debug_alloc.h)

快速定位建议：

- 如果问题是 崩溃或随机异常 → 首选 ASan。
- 如果问题是 内存泄漏 → 首选 Valgrind 或 heap profiler。
- 如果是 嵌入式资源紧张 → 用 日志 + 内存填充。

## [参考debug.md](../vala/debug.md)

- 编译程序时启用调试信息， `-g`
- 开启core抓取功能, 开启脚本见下面, 主要`ulimt -c`和输出保存地址写到文件`/proc/sys/kernel/core_pattern`中

    ```sh
    #!/bin/bash

    core_down()
    {
        sed -i '/ulimit -c 2048/culimit -c 0' /root/.bashrc
        echo "Set off coredump"
        echo "logout and login again to make it work"
    }

    core_up()
    {
        grep 'ulimit -c' /root/.bashrc >> /dev/null
        if [ $? -eq 0 ];then
            sed -i '/ulimit -c 0/culimit -c 2048' /root/.bashrc
        else
            echo "ulimit -c 2048" >> /root/.bashrc
        fi
        echo "/home/logs/core-%e-%p-%t" > /proc/sys/kernel/core_pattern
        echo "Set on coredump, the coredump log will place in /home/logs"
        echo "logout and login again to make it work"
    }

    case "$1" in
        "up")
                core_up
                ;;
        "down")
                core_down
                ;;
        *)
                echo $0 up/down
                ;;
    esac
    ```

- `gdb your-app coredump-file`
- 启动后输入`bt`

## 使用库ASAN库协助

### 准备工作

此库一般为GCC自带，如果没有需要自行编译，此库包含在gcc源码中

编译参考[自己动手编译asan库](https://zhuanlan.zhihu.com/p/71564723#%E5%87%86%E5%A4%87%E5%B7%A5%E4%BD%9C)

- 下载相应的gcc版本： 建议用[国内镜像](https://mirrors.aliyun.com/gnu/gcc/)
- 准备编译环境
- 编译:
  - 修改libstdc++-v3和libsanitizer下的configure，做法如下：

    ```sh
        if test "$srcdir" = "."; then
            if test "$with_target_subdir" != "."; then
                multi_basedir="$srcdir/$with_multisrctop../.."
            else
                multi_basedir="$srcdir/$with_multisrctop.."
            fi
            # 增加下面一行，就是不要上面的那个
            multi_basedir="$srcdir/.."
        else
            multi_basedir="$srcdir/.."
        fi
    ```

  - 配置编译

    ```sh
    ## stdc++可不编
    #cd libstdc++-v3
    #./configure --host=arm-linux-gnueabi --prefix=/path/to/install
    #make
    cd ..
    cd libsanitizer
    ./configure --host=arm-linux-gnueabi --prefix=/path/to/install
    make
    cp /path/to/install/lib/lib* /path/to/your/gcclib
    ```

### 编译目标程序

增加编译条件`-fsanitize=address`如下：

```sh
    gcc -Wall -g3 -fsanitize=address -o new new.c
```

静态链接加上`-static-libasan`

### 到目标板上运行查看错误提示

如果库没有安装到目标板上，可以如下临时加载运行, 静态链接可不用LD_PRELOAD

```sh
LD_PRELOAD=libasan.so.3.0.0 && ./new
```

### 其它编译选项和错误含义

1. -fsanitize=address： 它可以检测内存访问错误，如堆栈缓冲区溢出、堆区越界
    - 释放后使用（野指针）:heap-use-after-free
    - 堆缓冲区溢出 :heap-buffer-overflow, 申请内存比实际使用小
    - 栈缓冲区溢出 :stack-buffer-overflow
    - 全局缓冲区溢出 :global-buffer-overflow
    - 返回局部堆区地址后使用
    - 作用域外使用栈内存 :stack-use-after-scope
    - 初始化顺序错误
    - 内存泄漏: detected memory leaks
1. fsanitize=thread：可以检测多线程程序中的数据竞争和其他线程相关的错误，**注意：不能和-fsanitize=address -fsanitize=leak一起使用**

1. fsanitize=undefined: 可以检测代码中的未定义行为，如除以零、空指针解引用等

### Yocto 配方添加

```bbfile
DEPENDS += "gcc-sanitizers"
```

### CMAKE

```CMakeFile

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -static-libasan")
endif()

```

构建

```sh
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## valgrind

```sh
valgrind --leak-check=full ./your_app
```

## [日志 + 内存填充](../linux-coding-c/debug_alloc.h)

1. 全局替换 malloc/free/calloc/realloc
1. 自动泄漏报告（atexit）
1. 调用栈打印 + 文件名行号
1. 内存填充（分配填充 0xAA，释放填充 0xDD）
1. 分配表记录（检测泄漏）

### 功能说明

- 宏替换系统 malloc/free/calloc/realloc
  - 自动记录文件名和行号
  - 在标准 free() 不需要 size，但我们的封装需要 size 来填充内存, 解决方案是在分配表中记录 size，debug_free_impl() 根据 ptr 查找 size
- 调用栈打印：
  - backtrace() 显示分配/释放时的调用路径。
- 内存填充(帮助检测未初始化和 UAF)：
  - 分配后填充 0xAA → 检测未初始化使用。
  - 释放后填充 0xDD → 检测 UAF（Use-After-Free）。
- 日志输出：显示地址、大小、调用栈。
- 分配表记录（检测泄漏）:
  - 每次分配都会记录调用栈，释放时删除
  - 程序结束时调用 report_leaks() 检测泄漏

### 使用示例

```c
#include "debug_alloc.h"

int main() {
    char *buf = malloc(64);
    strcpy(buf, "Hello Debug Alloc!");
    buf = realloc(buf, 128);
    free(buf);

    char *arr = calloc(10, sizeof(char));
    // 忘记 free(arr)，测试泄漏报告

    return 0; // 程序退出时自动调用 report_leaks()
}
```
