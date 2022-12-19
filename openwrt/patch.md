
- [参考](#参考)
- [前言](#前言)
- [概要](#概要)
- [准备quilt配置](#准备quilt配置)
- [添加新补丁](#添加新补丁)
- [修改已存在的补丁](#修改已存在的补丁)
- [针对内核的补丁命名空间](#针对内核的补丁命名空间)

## 参考

1. [Working with patches](https://openwrt.org/docs/guide-developer/toolchain/use-patches-with-buildsystem)

## 前言  

openwrt的构建系统为方便补丁管理集成了quilt工具， 此文简要介绍下如何新增或修改已存在的补丁文件

## 概要

1. 准备源码和补丁： `make package/example/{clean,prepare} V=s QUILT=1`
2. 进入目录，应用已有补丁或新增补丁: `quilt push/add`
3. 修改文件:`quilt edit files`
4. 更新补丁:`quilt refresh`
5. 拷出临时补丁:`make package/example/update V=s`

-----

## 准备quilt配置

为了使用quilt生成适配的补丁格式文件，需要在本地家目录下创建一个配置文件“.quiltrc”, 文件中包含diff和patch的一些通用选项，如下：

```sh
cat << EOF > ~/.quiltrc
QUILT_DIFF_ARGS="--no-timestamps --no-index -p ab --color=auto"
QUILT_REFRESH_ARGS="--no-timestamps --no-index -p ab"
QUILT_SERIES_ARGS="--color=auto"
QUILT_PATCH_OPTS="--unified"
QUILT_DIFF_OPTS="-p"
EDITOR="nano"
EOF
```

--------

- EDITOR:指定交互编辑器
- 其它变量控制补丁格式的属性，像a/, b/ 目录名和无时间戳
- FreeBSD 不支持`--color=auto`选项并且`-pab`必须写成`-p ab`,-p ab在0.63版本之后支持，详见man page.

## 添加新补丁

1. 为了在一个存在的包中添加一个全新的补丁，首先准备好源码目录

    ```sh
    make package/example/{clean,prepare} V=s QUILT=1  
    ```

    上述命令会解压软件包，将存在的补丁准备好作为"quilt patch series"，调试输出将会显示源码解压的目录地址

2. 切换到准备好的源码目录，并用`quilt push`打上所有准备好的补丁

    ```sh
    cd build_dir/target-*/example-*
    quilt push -a
    ```

3. 添加新补丁，有两种方式：  

    - 引入上游源的补丁

        ```sh
        quilt import /path/to/010-main_code_fix.patch
        ```

    - 手动新建空白补丁

        ```sh
        quilt new 010-main_code_fix.patch
        # if modify kernel, common to all architectures patch
        quilt new generic/010-main_code_fix.patch
        # or specific target patch
        quilt new platform/010-main_code_fix.patch
        ```

4. 针对新建空白补丁，要打补丁的文件必须与补丁关联起来（使用`quilt add`），然后就可以像平常使用编辑器一样修改文件，也可以使用`quilt edit path/to/file`将两步合并,任何需要修改的文件都要如此处理
    ```sh
    quilt edit src/main.c 
    ## 1. add src/main.c to 010-main_code_fix.patch
    ## 2. open src/main.c with EDITOR
    ```

5. 更改完成后检查并更新修改

    ```sh
    quilt diff
    quilt refresh
    ```

6. 返回顶层目录，执行update(将临时目录下修改的补丁放到对应包的补丁目录下)，重构包
    
    ```sh
    cd ../../..
    make package/example/update V=s ## 将临时目录下修改的补丁放到对应包的补丁目录下
    make package/example/{clean,compile} package/index V=s ## 重构包
    ```
    如果出现问题，需要修改补丁解决，参考下面的 **修改已存在的补丁** 章节

-----------------------------------------------

## 修改已存在的补丁    

 1. 首先准备好源码目录

    ```sh
    make package/example/{clean,prepare} V=s QUILT=1
    cd build_dir/target-*/example-*
    ```

2. 进入该目录中, 列出可用的补丁

    ```sh
    quilt series
    ```

3. 推出需要编辑的补丁文件

    ```sh
    quilt push 010-main_code_fix.patch
    ```

    > - When passing a valid patch filename to push, quilt will only apply the series until it reaches the specified patch.
    > - If unsure, use quilt series to see existing patches and quilt top to see the current position.
    > - If the current position is beyound the desired patch, use quilt pop to remove patches in the reverse order.
    > - You can use the “force” push option (e.g. quilt push -f 010-main_code_fix.patch) to interactively apply a broken (i.e. has rejects) patch.

4. 使用`quilt edit`命令编辑需要修改的补丁, 每一个需要修改的补丁都如此

    ```sh
    quilt edit src/main.c
    ```  

    其它有用的命令：  
    - `quilt files`: 检查哪些文件在补丁中  
    - `quilt diff`: 审查修改  
　　

    **注： 如果不想后面直接clean源码目录，此处可以执行`quilt push -a`,再到顶层执行`make package/example/{compile,install}`，验证补丁修改是否成功，如果需要进一步修改，则返回第3步，继续修改。验证成功后直接到每6步update即可**


5. 如果修改OK了，用就`quilt refresh`更新当前修改

    ```sh
    quilt refresh
    ```

6. 返回构建顶层目录，将更新的补丁覆盖到构建目录，在对应包中执行更新命令

    ```sh
    cd ../../../
    make package/example/update V=s
    ```

7. 最后重构包进行修改的测试

    ```sh
    make package/example/{clean,compile} package/index V=s
    ```

--------------

## 针对内核的补丁命名空间
> The patches-* subdirectories contain the kernel patches applied for every OpenWrt target.
> All patches should be named 'NNN-lowercase_shortname.patch' and sorted into the following categories:
>
> 0xx - upstream backports
> 1xx - code awaiting upstream merge
> 2xx - kernel build / config / header patches
> 3xx - architecture specific patches
> 4xx - mtd related patches (subsystem and drivers)
> 5xx - filesystem related patches
> 6xx - generic network patches
> 7xx - network / phy driver patches
> 8xx - other drivers
> 9xx - uncategorized other patches
>
> ALL patches must be in a way that they are potentially upstreamable, meaning:
>
> - they must contain a proper subject
> - they must contain a proper commit message explaining what they change
> - they must contain a valid Signed-off-by line
> 

