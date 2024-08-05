# 内存异常调试方法

## [参考debug.md](../vala/debug.md)

- 编译程序时启用调试信息， `-g`
- 开启core抓取功能
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
