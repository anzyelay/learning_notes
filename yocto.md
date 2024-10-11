# YOCTO

1. 对已知的**TASKS**和**VARIABLES**的说明文件在

   - sources/oe-core/meta/conf/documentation.conf

1. 标准目标文件系统的路径变量和构建过程的变量定义的文件所在(Standard target filesystem paths)

   - sources/oe-core/meta/conf/bitbake.conf

## yocto

bb/bbclass中的变量说明,未说到的可以参数前一章节第1点的说明：

- S: 源码包在构建目录下的解压出来的目录（S[doc] = "The location in the Build Directory where unpacked package source code resides."）
- PN: package name, PN指的是OpenEmbedded构建系统使用的文件上下文中的配方名(recipe name)，作为创建包的输入。它指的是OpenEmbedded构建系统创建或生成的文件上下文中的包名
- DEPENDS: 后面接一个recipe name，或者一个recipe name list
  - Build time dependency, foo构建时的依赖, 只有依赖包先编译成功后才能编译foo

  比如foo.bb内容如下

  ```bitbake
  # foo的编译依赖bar，所以先编译bar 
  DEPENDS = "bar"
  # 带native表示需要依赖构建主机上的程序
  DEPENDS = "bar-native"
  # package version? 在编译时会在构建recipe-name目录下创建version-dir目录
  PV = "version-dir"
  ```

- RDEPENDS: 后面接一个recipe name，或者一个recipe name list, 
  - Run time dependency: foo运行时依赖, 表示该依赖包被正常安装后foo才能正常运行

  - PROVIDES：主要是为了起别名

- 加调试信息：
  - bbnote
  - bbwarn
  - bbfatal

### 编译的情况

一个recipe真正构建的目录所在大致在： build/xxx/work/xxx-platform-info/recipe-name/version/temp

- recipe-name构建目录下紧跟一个与version对应相关的目录
- 在这个version目录下存放着`build,temp,image,pkgdata,package,package-split,pseudo,*.patch`等一堆目录或文件，version由变量**PV**指定
- build下为构建输出的目录
- temp为bitbake运行的task和对应任务运行日志的目录，命名以`run.do_task`,`log.do_task`及后缀pid号的文件

### bitbake

假设在 **cwd** 或 **BBPATH** 中有一个可用的`conf/bblayers.conf`配置文件，它将提供layer、BBFILES和其他配置信息。

- 指令用法说明

    ```sh
    General options:
    recipename/target     为指定的recipe目标(.bb files)执行特定任务(默认为 build 任务).
    -s, --show-versions   显示所有recipes的 **当前** 和 **首选** 版本
    -e, --environment     显示全局或每个recipe的环境变量，此环境变量包含变量在哪被设置或修改的完整信息
    -g, --graphviz        以dot语法格式保存指定目标的依赖树信息
    -u UI, --ui UI        The user interface to use (knotty, ncurses, taskexp_ncurses or teamcity - default knotty).

    Task control options:
    -f, --force           强制执行指定目标/任务(使任何已存的stamp(戳记)文件失效).
    -c CMD, --cmd CMD     指定运行某个task. 可用的确切选项取决于元数据. 比如task可以是'compile'或'populate_sysroot'或'listtasks'.
    -C INVALIDATE_STAMP, --clear-stamp INVALIDATE_STAMP
                            Invalidate the stamp for the specified task such as 'compile' and then run the default task for the
                            specified target(s).
    --runall RUNALL       Run the specified task for any recipe in the taskgraph of the specified target (even if it wouldn't
                            otherwise have run).
    --runonly RUNONLY     Run only the specified task within the taskgraph of the specified targets (and any task dependencies
                            those tasks may have).
    --no-setscene         Do not run any setscene tasks. sstate will be ignored and everything needed, built.
    --skip-setscene       Skip setscene tasks if they would be executed. Tasks previously restored from sstate will be kept,
                            unlike --no-setscene.
    --setscene-only       只运行setscene任务，不运行任何实际任务

    Execution control options:
    -n, --dry-run         不执行，只是走过场
    -p, --parse-only      解析完BB配方后退出.
    -k, --continue        在出现错误后尽可能多的继续编译下去. 当目标构建失败，依赖于它的其它目标也不可能被构建，在错误目标停止构建前尽可能多的编译其它可构建的目标
    -P, --profile         配置命令并保存报告
    -S SIGNATURE_HANDLER, --dump-signatures SIGNATURE_HANDLER
                            Dump out the signature construction information, with no task execution. The SIGNATURE_HANDLER
                            parameter is passed to the handler. Two common values are none and printdiff but the handler may
                            define more/less. none means only dump the signature, printdiff means recursively compare the dumped
                            signature with the most recent one in a local build or sstate cache (can be used to find out why tasks
                            re-run when that is not expected)
    --revisions-changed   Set the exit code depending on whether upstream floating revisions have changed or not.
    -b BUILDFILE, --buildfile BUILDFILE
                            Execute tasks from a specific .bb recipe directly. WARNING: Does not handle any dependencies from
                        log.do_fetch.47910    other recipes.

    Logging/output control options:
    -D, --debug           Increase the debug level. You can specify this more than once. -D sets the debug level to 1, where
                            only bb.debug(1, ...) messages are printed to stdout; -DD sets the debug level to 2, where both
                            bb.debug(1, ...) and bb.debug(2, ...) messages are printed; etc. Without -D, no debug messages are
                            printed. Note that -D only affects output to stdout. All debug messages are written to
                            ${T}/log.do_taskname, regardless of the debug level.
    -l DEBUG_DOMAINS, --log-domains DEBUG_DOMAINS
                            Show debug logging for the specified logging domains.
    -v, --verbose         Enable tracing of shell tasks (with 'set -x'). Also print bb.note(...) messages to stdout (in addition
                            to writing them to ${T}/log.do_<task>).
    -q, --quiet           Output less log message data to the terminal. You can specify this more than once.
    -w WRITEEVENTLOG, --write-log WRITEEVENTLOG
                            Writes the event log of the build to a bitbake event json file. Use '' (empty string) to assign the
                            name automatically.

    Server options:
    -B BIND, --bind BIND  The name/address for the bitbake xmlrpc server to bind to.
    -T SERVER_TIMEOUT, --idle-timeout SERVER_TIMEOUT
                            Set timeout to unload bitbake server due to inactivity, set to -1 means no unload, default:
                            Environment variable BB_SERVER_TIMEOUT.
    --remote-server REMOTE_SERVER
                            Connect to the specified server.
    -m, --kill-server     Terminate any running bitbake server.
    --token XMLRPCTOKEN   Specify the connection token to be used when connecting to a remote server.
    --observe-only        Connect to a server as an observing-only client.
    --status-only         Check the status of the remote bitbake server.
    --server-only         Run bitbake without a UI, only starting a server (cooker) process.

    Configuration options:
    -r PREFILE, --read PREFILE
                            Read the specified file before bitbake.conf.
    -R POSTFILE, --postread POSTFILE
                            Read the specified file after bitbake.conf.
    -I EXTRA_ASSUME_PROVIDED, --ignore-deps EXTRA_ASSUME_PROVIDED
                            Assume these dependencies don't exist and are already provided (equivalent to ASSUME_PROVIDED). Useful
                            to make dependency graphs more appealing.
    ```

- 常用指令：

    1. 只运行recipe中的某个特定任务, 会把对应recipe依赖的其它recipe也运行

        ```sh
        bitbake -c task recipe-name
        ```

        常见task:
            下载（fetch）、解包（unpack）、打补丁（patch）、配置（configure）、编译（compile）、安装（install）、打包（package）、staging、做安装包（package_write_ipk）、构建文件系统等,可以在对应的构建目录下的temp中看到一堆的run.do_xxx, xxx就是task。
        不指定任务则是默认build

        eg:
        - `bitbake -fc cleansstate recipe-name`: 清包
        - `bitbake -c clean recipe-name`: 会把下载包也清了

    1. 可针对某个recipe寻找变量信息

       `bitbake -e recipe-name | grep -e '\bKERNEL_VERSION\b'`
