# cmake

## 指南（Guides）

总指南: [CMake Reference Documentation]

- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html): 该部分针对想创建一个CMake工程的开发者
- [User Interaction Guide](https://cmake.org/cmake/help/latest/guide/user-interaction/index.html): 该指南针对想构建编译CMake工程的用户
- [Using Dependencies Guide](https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html): 该指南针对使用第三方库的开发者
- [Importing and Exporting Guide](https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html): 该指南讲如何导入其它工程文件和导出现有工程给其它工程导入用
- [IDE Integration Guide](https://cmake.org/cmake/help/latest/guide/ide-integration/index.html): IDE集成指南

## CMake变量及CMake支持命令

- 变量
    [官网查找](https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html)
    *注：关于特定语言的查找要搜`CMAKE_<LANG>_COMPILER`,中间用\<**LANG**\>代替，如找CMAKE_C_COMPILER时不要直接写C，换成\<LANG>才能找到*

- CMake提供的寻找命令有：
  - `find_program`
  - `find_library`
  - `find_file`
  - `find_path`
  - `find_package`： 实质是执行`Find<*>.cmake`模块，里面调用上述4个find命令

## cmake命令行工具

### overview

功能|command
-|-
生成构建系统 | <li>`cmake [<options>] -B <path-to-build> [-S <path-to-source>]`</li><li>`cmake [<options>] <path-to-source \| path-to-existing-build>`</li>
编译工程| `cmake --build <dir> [<options>] [-- <build-tool-options>]`
安装| `cmake --install <dir> [<options>]`
打开| `cmake --open <dir>`
运行脚本| `cmake [-D <var>=<value>]... -P <cmake-script-file>`
运行指令| `cmake -E <command> [<options>]`
寻找库、包<br>(替代`pkg-config`)| `cmake --find-package [<options>]`
Run a Workflow Preset| `cmake --workflow [<options>]`
View Help| `cmake --help[-<topic>]`

### 命令使用

#### 1. 生成工程构建系统: `cmake <path-to-source>`

概念| 说明
-|-
Source Tree|工程源码的顶层目录，包含一个名为**CMakeLists.txt**的文件，描述构建目标和依赖关系，使用`-S`指定
Build Tree|构建输出的顶层目录，包含一个名为**CMakeCache.txt**的文件，永久保存构建配置项的信息，使用`-B`指定
Generator|选择生成的构建系统类型，使用`-G`指定
Entry | 可配置项条目，CMakeList.txt中使用`set(Entry value)`标识，命令行中使用`-D`添加、修改，使用`-L[AH]`查看，使用`-U`删除

在当前或指定目录下生成构建系统(generate build system-Makefile)
生成后所有配置项目(entry)条目在文件`CMakeCache.txt`中保存，可以用option为**D、L、U**对这些条目进行增改、查、删操作。

```sh
cmake [options] <path-to-source>
cmake [options] <path-to-existing-build>
cmake [options] -S <path-to-source> -B <path-to-build>
```

options:

- `-S <path-to-source>`          :  明确指定cmake工程源码根目录.
- `-B <path-to-build>`           :  明确指定构建根目录.
- `-D <var>[:<type>]=<value>`    :  创建或更新一个cmake缓存中的条目(entry).内部支持的条目可参考CMake变量
- `-U <globbing_expr>`           :  从cmake缓存中删除匹配的条目.支持"*"和"?"
- `-L[A][H]`                     : 列出缓存中的非内部和高级标志的条目(变量)，可以快速显示由`-D`改变的设置项，加了`A`则显示高级标志项，加了`H`显示条目的帮助信息
- `-N`                           : 观看模式，仅加载cache，不运行配置和生成步骤,必须与-L分开独立写（即不能直接写`-LN`,要写成`-L -N`）
- `--toolchain <file>`           :  指定交叉编译平台信息的[toolchain file]与**cmake -DCMAKE_TOOLCHAIN_FILE=file**等效.
- `--install-prefix <directory>` :  指定安装目录,与**cmake -DCMAKE_INSTALL_PREFIX=dir**等效
- `-G <generator-name>`          :  指定一个构建系统的生成器eg: "Makefile Generators", "Ninja Generators","CodeBlocks","Visual Studio Generators".
- `-T <toolset-name>`            :  如果生成器支持则可指定toolset name
- `-A <platform-name>`           :  如果生成器支持则可指定platform name

#### 2. 构建工程: `cmake --build`

运行`cmake --build`不要参数查看帮助

#### 3. 安装工程: `cmake --install`

## CMake中的相关命令

在CMakeLists.txt中可使用的命令

1. 加入子工程， `add_subdirectory`

1. 工程目标有三种，可执行文件，库和定制命令,`add_executable, add_library, add_custom_target`

    ```CMakeLists

    ## 增加一个从源代码列表构建成名为name库的目标
    add_library(<name> [<type>] [EXCLUDE_FROM_ALL] <sources>...)
    ```

    - type: **STATIC**(default), **SHARE**, **MODULE**
    - sources: 后面使用`target_sources()`添加后，此处可忽略

1. target命令

    ```cmake
    target_compile_options()
    target_include_directories()
    target_sources()
    target_link_libraries()
    target_link_directories()
    target_link_options()
    ```

1. **option**: 提供一个用户可以选择的布尔类型选项, value不加默认OFF
   `option(<variable> "<help_text>" [value])`

1. `add_custom_command`,用于生成文件和创建编译过程的构建事件

    ```cmake
    # 运行someTool去生成out-$<CONFIG>.c文件,然后此文件做为生成库的源文件
    add_custom_command(
    OUTPUT "out-$<CONFIG>.c"
    COMMAND someTool -i ${CMAKE_CURRENT_SOURCE_DIR}/in.txt
                    -o "out-$<CONFIG>.c"
                    -c "$<CONFIG>"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/in.txt
    COMMENT "Display the given message before the commands are executed at build time" 
    VERBATIM)
    add_library(myLib "out-$<CONFIG>.c")
    add_library(myPlugin MODULE myPlugin.c)

    # 在目标myPlugin生成后执行someHasher命令
    add_custom_command(
    TARGET myPlugin POST_BUILD
    COMMAND someHasher -i "$<TARGET_FILE:myPlugin>"
                        --as-code "myPlugin-hash-$<CONFIG>.c"
    BYPRODUCTS "myPlugin-hash-$<CONFIG>.c"
    VERBATIM)
    add_executable(myExe myExe.c "myPlugin-hash-$<CONFIG>.c")
    ```

1. `get_filename_component`, 获取文件信息

   ```cmake
   # hw_proto = /absolute/path/to/helloworld.proto
   get_filename_component(hw_proto "../../protos/helloworld.proto" ABSOLUTE)
   # hw_proto_path = /absolute/path/to/
   get_filename_component(hw_proto_path "${hw_proto}" PATH)
   ```
  
1. 使用`pkg-config`配置第三方库和头文件

   假设有一个名为/usr/local/Cellar/glew/2.1.0_1/lib/pkgconfig/glew.pc的文件。则在cmakelist中可如下写法：
  
   ```cmake
   set(ENV{PKG_CONFIG_PATH} "/usr/local/Cellar/glew/2.1.0_1/lib/pkgconfig")
   find_package(PkgConfig)
   # GLEW是变量名，后面XXX_INCLUDE_DIRS, XXX_LIBRARIES等都是以其为前缀
   pkg_search_module(GLEW REQUIRED glew)    
   MESSAGE(STATUS "glew dirs:" ${GLEW_INCLUDE_DIRS})
   MESSAGE(STATUS "glew lib:" ${GLEW_LIBRARIES})
   include_directories(${GLEW_INCLUDE_DIRS})
   link_directories(${GLEW_LIBRARY_DIRS})
   ... ...
   target_link_libraries(main ${GLUT_LIBRARY} ${OPENGL_LIBRARY} ${GLEW_LIBRARIES})
   ```
  
  pkg-config方式查找的名称引用如上为
  
  ```cmake
  GLEW_INCLUDE_DIRS 
  GLEW_LIBRARY_DIRS
  GLEW_LIBRARIES
  ```

  如果是使用`find_package()`方式，则为

  ```cmake
  GLEW_INCLUDE_DIR
  GLEW_LIBRARY_DIR
  GLEW_LIBRARY
  ```

1. 安装指令**INSTALL**, 指令用于定义安装规则，安装的内容可以包括目标二进制、动态库、静态库以及文件、目录、脚本等
  
   1. 目标文件的安装, 目标类型有三种,**ARCHIVE**特指静态库，**LIBRARY**特指动态库，**RUNTIME**特指可执行目标二进制

      ```cmake
      INSTALL(TARGETS myrun mylib mystaticlib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION libstatic
      )
      ```

      **特别注意的是不需要关心 TARGETS 具体生成的路径，只需要写上 TARGETS 名称就可以了。**
      **DESTINATION指定安装目录，如果开头不是“/”则是相对于CMAKE_INSTALL_PREFIX的目录, 否则就是无视CMAKE_INSTALL_PREFIX的绝对路径**

      上面的例子会将：
        - 可执行二进制 **myrun** 安装到${CMAKE_INSTALL_PREFIX}/bin 目录
        - 动态库 **libmylib** 安装到${CMAKE_INSTALL_PREFIX}/lib 目录
        - 静态库 **libmystaticlib** 安装到${CMAKE_INSTALL_PREFIX}/libstatic 目录

   1. 普通文件安装

   ```cmake

   ```

## Cross Compiling With CMake

- 参考[toolchain file]和[cmake-toolchains](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)

- 概念
  
    概念| 说明
    -|-
    build host | 构建主机，当前用于构建工程所运行的系统
    target system/platform | 目标系统/平台，构建的软件所要运行的目标系统/平台
    toolchain file | 用于让CMake知晓工程的**target platform**信息的文件, 告知构建平台和目标运行平台的差异信息, 用命令`cmake -DCMAKE_TOOLCHAIN_FILE=`指定

- 用法：`cmake -DCMAKE_TOOLCHAIN_FILE=toolchain path/to/source`

- toolchain中可用的变量：

    变量名| 说明
    -|-
    CMAKE_SYSTEM_NAME | 必须(mandatory),设置目标平台的名称<br><li>Linux</li><li>Windows</li><li>Generic:无OS的嵌入式系统</li>预设此值而不是用默认值可使CMake认为是cross构建而将**CMAKE_CROSSCOMPILING**设为**TRUE**,更多请参考[CMAKE_SYSTEM_NAME]
    CMAKE_SYSTEM_VERSION|目标平台系统版本, 可选
    CMAKE_SYSTEM_PROCESSOR|可选，目标平台的处理器或硬件名，可用`uname -m`,参考[CMAKE_HOST_SYSTEM_PROCESSOR]
    CMAKE_C_COMPILER|c编译器可执行程序,如果是完整(绝对)路径，则其它编译工具(c++,binutils,ld等)也会从该路径中寻找，如果有前缀则CMake自动寻找对应的其它编译工具。<br>编译器也可通过环境变量**CC**设置
    CMAKE_CXX_COMPILER|c++编译器，其它同上面的C编译器，环境变量为**CXX**
    CMAKE_SYSROOT | 可选的，如果有 sysroot 可用，则可以指定 
    CMAKE_STAGING_PREFIX | 可选的。它可用于指定主机上的安装路径。CMAKE_INSTALL_PREFIX 始终是运行时安装位置，即使在交叉编译时也是如此
    CMAKE_FIND_ROOT_PATH | 默认为空，设置find命令对lib,header找寻的路径前缀，可设置多个路径, 还有三个选项用来精确控制：**NO_CMAKE_FIND_ROOT_PATH**,**ONLY_CMAKE_FIND_ROOT_PATH**,**CMAKE_FIND_ROOT_PATH_BOTH**
    CMAKE_FIND_ROOT_PATH_MODE_PROGRAM|配合上面**CMAKE_FIND_ROOT_PATH**使用，设置`find_proram`找寻时是否加上**CMAKE_FIND_ROOT_PATH**做前缀的形为，有三个选项：<br><li>NERVER:除非明确指定，否则不使用</li><li>ONLY:只搜索CMAKE_FIND_ROOT_PATH做前缀下的目录</li><li>BOTH:即搜索CMAKE_FIND_ROOT_PATH下路径，也搜索其它系统默认路径，该项为**默认选项**</li>
    CMAKE_FIND_ROOT_PATH_MODE_LIBRARY|同上，只是针对`find_library`命令
    CMAKE_FIND_ROOT_PATH_MODE_INCLUDE|同上，只是针对`find_path`、`find_file`命令
    CMAKE_C_FLAGS_INIT | 用来在构建系统首次生成时初始化**CMAKE_<LANG>_FLAGS**缓存条目
    CMAKE_CXX_FLAGS_INIT | 同上

- 示例

  ```cmake
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR armv7l)
    set(devel_root /tmp/raspberrypi_root)
    set(CMAKE_STAGING_PREFIX ${devel_root}/stage)
    set(tool_root /working_dir/ql-ol-crosstool/sysroots)
    # where is the target environment located
    set(CMAKE_SYSROOT ${tool_root}/armv7ahf-neon-oe-linux-gnueabi)
    # which compilers to use for C and C++
    set(CMAKE_C_COMPILER ${tool_root}/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-gcc)
    set(CMAKE_CXX_COMPILER ${tool_root}/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-g++)
    set(CMAKE_CXX_FLAGS_INIT "-march=armv7-a -marm -mfpu=neon -mfloat-abi=hard")
    set(CMAKE_C_FLAGS_INIT "-march=armv7-a -marm -mfpu=neon -mfloat-abi=hard")

    # search programs in the host environment
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    # search headers and libraries in the target environment
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  ```

## 参考

[CMake Reference Documentation]:https://cmake.org/cmake/help/latest/index.html
[toolchain file]:https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html#target-platform-and-toolchain-issues
[CMAKE_SYSTEM_NAME]:https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
[CMAKE_HOST_SYSTEM_PROCESSOR]:https://cmake.org/cmake/help/latest/variable/CMAKE_HOST_SYSTEM_PROCESSOR.html#variable:CMAKE_HOST_SYSTEM_PROCESSOR
