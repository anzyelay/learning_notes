# bitbake

## reference

1. [openEuler Embedded 在线文档--bitbake 手册](https://embedded.pages.openeuler.org/master/bitbake/index.html#)--译文
2. [bitbake-user-manual](https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-intro.html)--官方手册，源码中相应目录`sources/bitbake/doc/bitbake-user-manual`亦可见
3. [DeepWiki openembedded/bitbake](https://deepwiki.com/openembedded/bitbake/1-bitbake-overview)

## 一、bitbake 简介

### 1.1 定位和解析配方

- 通过**层次结构机制**来定位与解析

    ```sh
        1. bitbake --read--> bblayers.conf --> to know which layers should be used
        2. bitbake enter to every layers --read--> meta-layerXXX/conf/layer.conf --specify--> BBPATH & BBFILES
            1. BBPATH --specify--> where to search `./conf` and `classes` files
            2. BBFILES --specify--> where to find recipes and append files (`.bb and .bbappend`)
    ```

- 配方中的`d`是什么？

  - 在 BitBake 中，`d` 代表 **数据存储对象 (DataStore)**

  - 这个对象保存了当前配方（.bb 文件）解析过程中所有的变量和值。

  - 当 BitBake 解析一个配方时，它会创建一个 DataStore 实例，并把配方文件路径、继承的类、变量定义等信息都放进去。

    BitBake 在加载配方时，会自动设置一个特殊变量**FILE**（当前配方文件的完整路径），因此使用`d.getVar('FILE', False)`就可获得此路径，False 是指不扩展解析（expand）路径中的其它变量

- **另一种机制--集合机制**

    在 BitBake 中，BBFILES 用来指定配方文件（.bb）的搜索路径。
    当你有多个来源（比如官方 upstream 和本地修改版 local），就需要用 BBFILE_COLLECTIONS 来区分不同的集合，并通过 模式匹配与优先级来控制解析顺序。

    ```bb
    BBFILES = "/stuff/openembedded/*/*.bb /stuff/openembedded.modified/*/*.bb"
    BBFILE_COLLECTIONS = "upstream local"
    BBFILE_PATTERN_upstream = "^/stuff/openembedded/"
    BBFILE_PATTERN_local = "^/stuff/openembedded.modified/"
    BBFILE_PRIORITY_upstream = "5"
    BBFILE_PRIORITY_local = "10"
    ```

  - 定义两个集合：upstream 和 local。

  - 每个集合对应一组配方文件来源。

  - 为每个集合定义匹配模式（正则表达式）。

  - 当 BitBake 扫描 BBFILES 时，会根据路径匹配来决定某个配方属于哪个集合。

  - 设置集合的优先级, 数字越大，优先级越高。

  - `local (10) > upstream (5)`: 意味着如果同一个配方在两个集合里都有，BitBake 会优先使用 local 版本

    **换句话说，这是一种 “本地覆盖上游” 的机制**

- **集合机制 vs 层次结构机制**

    在 BitBake 的早期版本中，常用 集合机制 (collections) 来组织和管理不同来源的配方文件。 随着 BitBake 和 Yocto 项目的发展，引入了 **层次结构机制 (layers)**

  - 每个层（layer）就是一个目录树，包含配方、类、配置等。
  - 层通过 bblayers.conf 文件统一管理。
  - 层次结构比集合更直观：你只需把需要的层加入构建环境，BitBake 就会自动解析其中的配方。

> 层次结构机制现在是收集代码的首选方法。虽然集合代码仍然存在，但其主要用途是设置层优先级和处理层之间的重叠（冲突）

### 1.2 Providers

每个配方的**PROVIDES**列表通过配方的 PN 变量**隐式创建**，也可以通过配方的 PROVIDES 变量**显式创建**，这是可选的。

1. 背景:

    - 当 BitBake 被要求构建一个目标（target）时，它会先解析所有配方文件（.bb）。
    - 接着，它需要决定 哪个配方能提供这个目标。
    - 这时就会用到 **PROVIDES** 列表。

1. **PROVIDES** 列表的作用:

    - **PROVIDES** 是配方可能被识别的名称集合
    - 每个配方至少有一个隐式的 PROVIDES 名称, 由 PN（包名）变量自动生成
        - 例如：`keyboard_1.0.bb` → PN = `keyboard` → 隐式 PROVIDES = `keyboard`
    - 配方还可以通过显式设置 PROVIDES 来增加额外的别名
        - 例如：`PROVIDES += "fullkeyboard"`

**总结:**

- PN → 隐式 PROVIDES 名称（每个配方都有）,可以用`PN = "${@bb.parse.vars_from_file(d.getVar('FILE', False),d)[0]}"`获取
- PROVIDES → 显式添加的别名（可选）
- 作用：让同一个配方能以多个名字被引用，方便在不同场景下使用或替代

### 1.3 Preferences

**PROVIDES**列表只是解决目标配方问题的一部分。因为目标可能有多个提供者，BitBake 需要通过确定**PREFERRED_PROVIDER**(提供者偏好)来对提供者进行优先级排序

- 默认的**PREFERRED_PROVIDER**是与目标同名的提供者
- 相同的提供名下，可以使用**DEFAULT_PREFERENCE**变量影响顺序， 默认情况下，文件的优先级为“0”。将**DEFAULT_PREFERENCE**设置为“-1”意味着配方不太可能被使用，除非明确引用
- BitBake 默认选择提供者的最高版本，可以使用**PREFERRED_VERSION**变量指定特定版本

```bitbake
PREFERRED_PROVIDER_virtual/kernel = "linux-yocto"
PREFERRED_VERSION_a = "1.1"
DEFAULT_PREFERENCE = "-1"
```

### 1.4 Dependencies

- 为了在多核系统上获得最佳性能，BitBake 将每个任务视为具有自己依赖项集的独立实体
- Bitbake 计算依赖项时使用**DEPENDS**和**RDEPENDS**变量

### 1.5 任务列表

- 基于上述生成的提供者列表和依赖信息，BitBake 现在可以准确计算出它需要运行哪些任务以及以何种顺序运行它们
- 构建现在从 BitBake 开始，它会分叉出线程，直到达到在 BB_NUMBER_THREADS 变量中设置的限制
- 每个任务完成后，会在 STAMP 变量指定的目录写入一个时间戳
- BitBake 的时间戳检查是**局部的**，而不是全局的依赖传播机制, 依赖有效性靠**签名机制**
  - 时间戳只在单个配方文件范围内生效
  - 如果配置比编译更新，编译会重新运行
  - 但这种重新运行不会影响依赖该目标的其他配方或提供者

**时间戳 vs 签名机制:**

|   机制         |   检查范围             |   触发条件                                                           |   对依赖的影响                            |   典型用途                          |
|  ------------  |  --------------------  |  ------------------------------------------------------------------  |  ---------------------------------------  |  ---------------------------------  |
|   时间戳机制   |   单个配方文件（局部）   |   如果某个任务的输入文件时间戳比输出文件新 → 任务重新运行            |   不影响依赖者，只在当前配方内生效         |   检查配置/编译结果是否过期         |
|   签名机制     |   跨配方依赖链（全局）   |   如果任务的输入内容或依赖的输出发生变化 → 签名不同 → 任务重新运行   |   会影响依赖者，依赖链上的任务会重新构建   |   保证依赖一致性，避免使用过期结果   |

**总结:**

1. 时间戳机制 → 局部有效，只保证单个配方的任务结果不过期。

1. 签名机制 → 全局有效，确保跨依赖链的构建一致性。

### 1.6 执行任务

**1. 任务可以是`shell`任务或`Python`任务**

1. shell 任务:

    1. BitBake 会将一个 shell 脚本写入`${T}/run.do_taskname.pid`，然后执行该脚本
    2. shell 脚本的输出进入文件`${T}/log.do_taskname.pid`

    生成的 shell 脚本包含所有导出的变量和所有变量展开后的 shell 函数

2. python 任务: BitBake 在内部执行任务并将信息记录到控制终端，未来版本将参考 shell 任务演进

**2. BitBake 运行任务的顺序由其任务调度器控制.**

- BB_SCHEDULER
- BB_SCHEDULERS

**3. 可以在任务的主函数之前和之后运行函数.**

这是通过使用任务的`[prefuncs]`和`[postfuncs]`标志来完成的，这些标志列出了要运行的函数

```bitbake
do_compile[prefuncs] += "prepare_env"
do_compile[postfuncs] += "cleanup_temp"

python prepare_env() {
    bb.note("Preparing environment before compile...")
}

python cleanup_temp() {
    bb.note("Cleaning up after compile...")
}
```

对比`addtask`

- `prefuncs/postfuncs` → 用来在已有任务的前后挂钩函数，轻量级扩展
- `addtask` → 用来定义新的任务，并通过依赖关系控制它的执行顺序，适合更复杂的构建逻辑。

## 二、bitbake 基本语法和操作符

### 2.1 变量赋值操作

#### 2.1.1 BitBake 变量赋值操作符流程图

```sh

变量赋值过程
│
├── 定义阶段
│   ├── "="        → 延迟展开（使用时解析）
│   ├── ":="       → 立即展开（解析时替换）
│   ├── "?="       → 默认赋值（立即，若未定义则赋值）
│   └── "??="      → 弱默认赋值（立即，仅在完全未赋值时生效）
│
├── 修改阶段
│   ├── 立即修改
│   │   ├── "+="   → 后置追加（自动加空格，立即）
│   │   ├── "=+"   → 前置追加（自动加空格，立即）
│   │   ├── ".="   → 后置追加（无空格，立即）
│   │   └── "=."   → 前置追加（无空格，立即）
│   │
│   └── 覆盖式修改（延迟）
│       ├── ":append"  → 展开时后置追加（不自动加空格）
│       ├── ":prepend" → 展开时前置追加（不自动加空格）
│       └── ":remove"  → 展开时移除指定值
│
├── 展开阶段
│   ├── "${VAR}"   → 延迟展开变量
│   └── "${@...}"  → 内联 Python（延迟或立即，取决于 "=" 或 ":="）
│
└── 其他操作
    ├── "\\"       → 行连接（立即解析）
    ├── "[flag]"   → 变量标志（立即生效）
    └── "unset"    → 移除变量（立即生效）

```

**总结:**

- 立即类：`:=、+=、=+、.=、=.、?=、??=、unset` → 解析时立刻确定。

- 延迟类：`=、${}、:append、:prepend、:remove、${@...}` → 使用/展开时才解析。

- 覆盖式操作：属于延迟类，保证最终结果修改不会被覆盖。

#### 2.1.2 BitBake 操作符对照表

|   类别             |   操作符        |   示例                                     |   生效时机     |   特点 / 用法说明                                                                                                         |
|  ----------------  |  -------------  |  ----------------------------------------  |  ------------  |  -----------------------------------------------------------------------------------------------------------------------  |
|   **基本赋值**     |   `=`           |   `VAR = "value"`                          |   延迟展开     |   最常见赋值方式，使用时才解析。                                                                                            |
|                    |   `:=`          |   `A := "test ${T}"`                       |   立即展开     |   在赋值时替换变量值，未定义变量保留原样。                                                                                  |
|   **默认赋值**     |   `?=`          |   `A ?= "aval"`                            |   立即展开     |   若未定义则赋值，已有值保持（哪怕是空字符串）。多个 `?=` 时，取第一个生效                                                     |
|                    |   `??=`         |   `W ??= "x"`                              |   立即展开     |   仅在完全未赋值时才生效，优先级最低。只要变量有过任何赋值（包括 `?=`），`??=` 就失效。多个 `??=` 时，后者会替换前者的弱默认值   |
|   **立即追加**     |   `+=` / `=+`   |   `B += "extra"` / `C =+ "test"`           |   立即展开     |   后置/前置追加，自动加空格。                                                                                               |
|                    |   `.=` / `=.`   |   `B .= "extra"` / `C =. "test"`           |   立即展开     |   后置/前置追加，不加空格。                                                                                                 |
|   **覆盖式操作**   |   `:append`     |   `FOO:append = " extra"`                  |   延迟展开     |   展开时后置追加，不自动加空格。                                                                                            |
|                    |   `:prepend`    |   `FOO:prepend = "extra "`                 |   延迟展开     |   展开时前置追加，不自动加空格。                                                                                            |
|                    |   `:remove`     |   `FOO:remove = "123"`                     |   延迟展开     |   展开时移除指定值，顺序为 `append → prepend → remove`。                                                                    |
|   **变量展开**     |   `${VAR}`      |   `B = "pre${A}post"`                      |   延迟展开     |   使用时才解析。变量没有值则保持原样                                                                                       |
|                    |   `${@...}`     |   `DATE = "${@time.strftime('%Y%m%d')}"`   |   延迟或立即   |   内联 Python，取决于 `=` 或 `:=`。                                                                                         |
|   **其他**         |   `\`           |   `FOO = "bar \ baz"`                      |   立即解析     |   行连接，去掉换行符。                                                                                                      |
|                    |   `[flag]`      |   `FOO[doc] = "说明"`                      |   立即生效     |   给变量附加属性或说明。                                                                                                   |
|                    |   `unset`       |   `unset VAR`                              |   立即生效     |   移除变量或其 flag。                                                                                                      |

#### 2.1.3 BitBake 操作符对比矩阵

|   对比维度         |   立即类                                                                              |   延迟类                                                    |   覆盖式操作                                                                        |
|  ----------------  |  -----------------------------------------------------------------------------------  |  ---------------------------------------------------------  |  ---------------------------------------------------------------------------------  |
|   典型语法         |   `:=`、`+=`、`=+`、`.=`、`=.`、`?=`、`??=`、`unset`                                         |   `=`、`${}`、`${@...}`                                       |   `:append`、`:prepend`、`:remove`                                                    |
|   生效时机         |   解析时立即确定值                                                                    |   使用/展开时才解析                                         |   在最终展开时才应用修改                                                            |
|   是否自动加空格   |   `+=` / `=+` 自动加空格<br>`.=` / `=.` 不加空格                                      |   取决于赋值内容                                            |   不自动加空格，需要手动控制                                                         |
|   是否可能被覆盖   |   ✅ 可能被后续赋值覆盖                                                                |   ✅ 可能被后续赋值覆盖                                      |   ❌ 不会被覆盖，保证最终结果修改生效                                                 |
|   常见用途         |   - 提供默认值（`?=`、`??=`）<br> - 立即拼接字符串（`+=`、`.=`）<br> - 强制立即展开（`:=`）   |   - 延迟依赖其他变量（`=`、`${}`）<br> - 动态计算（`${@...}`）   |   - 在 `.bbclass` 或 `.bbappend` 中修改已有变量 <br> - 保证最终结果包含追加或移除   |
|   优缺点           |   ✅ 快速确定值<br> ❌ 容易被覆盖                                                       |   ✅ 灵活依赖其他变量<br> ❌ 结果不确定直到展开               |   ✅ 最终结果保证修改生效<br> ❌ 不自动加空格，需手动控制                              |

**总结:**

- 立即类：适合在当前文件中直接修改，结果立刻确定，但可能被覆盖。

- 延迟类：适合依赖其他变量或动态计算，结果在使用时才解析。

- 覆盖式操作：属于延迟类，但有“保险机制”，保证最终结果修改不会被覆盖。

```bitbake
A = "1"
A:append = "2"
A:append = "3"
A += "4"
A .= "5"
```

`A += "4"` 和 `A .= "5"` 是 立即追加，在解析时就修改了值。

结果：`A = "1 45"`。

`A:append = "2"` 和 `A:append = "3"` 是 覆盖式追加，在最终展开时才应用。

最终结果：`A = "1 4523"`。

### 2.2 变量的条件语法(overrides)

**`OVERRIDES` 是 BitBake 条件语法的核心，用来控制变量在不同条件下的取值。**

#### 2.2.1 基本概念

- `OVERRIDES`是一个由冒号分隔的列表，用来控制变量的条件覆盖。

- 当某个override名称出现在`OVERRIDES`中时，BitBake会选择该条件版本的值做为变量值使用。

- override名称只能使用小写字母、数字和短横线（-），不能使用冒号。

#### 2.2.2 作用

- **Selecting a Variable**: 在`OVERRIDES`中指定一个条件选项，那么bitbake就选中这个条件版本的变量值使用

  ```bitbake
    OVERRIDES = "architecture:os:machine"
    TEST = "default"
    TEST:os = "osspecific"
    TEST:nooverride = "othercondvalue"
  ```

  如果 `os` 在 `OVERRIDES` 中，则 `TEST` 的值为 "osspecific"; 如果`OVERRIDES`的值是`nooverride`,则`TEST`的值为"othercondvalue"

- **Appending and Prepending**: bitbake也支持条件变量的append和prepend操作

  ```bitbake
  DEPENDS = "glibc ncurses"
  OVERRIDES = "machine:local"
  DEPENDS:append:machine = " libmad"
  ```

  如果 `machine` 在 `OVERRIDES` 中，最终 `DEPENDS = "glibc ncurses libmad"`; 不在则值为`DEPENDS = "glibc ncurses`

  另注下面的区别：

  | 写法              | 示例代码                          | OVERRIDES = "foo" 时的结果 | 解释说明 |
  |-------------------|-----------------------------------|----------------------------|----------|
  | `A:foo:append`    | A = "Z" <br> A:foo:append = "X"  | A = "X"                    | 在 `foo` override 生效时，`A:foo` 版本替换掉原始 `A`，然后在该版本上追加 `"X"`。原始值 `"Z"` 不保留。 |
  | `A:append:foo`    | A = "Z" <br> A:append:foo = "X"  | A = "ZX"`                  | 在 `foo` override 生效时，对原始 `A` 进行追加。原始值 `"Z"` 保留，并在其后拼接 `"X"`。 |

- **Setting a Variable for a Single Task**: bitbake支持设置变量的值仅在某一单个任务期间有效

  ```bitbake
  FOO:task-configure = "val 1"
  FOO:task-compile = "val 2"
  ```

  在执行`do_configure`时`FOO`为“val 1”, 在执行`do_compile`时`FOO`为“val 2”

  内部实现上，就是在`do_compile`任务的本地存储数据库中的`OVERRIDES`值被前缀加上"task-compile:"来实现的。

### 2.3 功能的共享

这一部分介绍了BitBake让你能够在配方之间共享功能机制。具体来说，这些机制包括`include、inherit、INHERIT`和`require`指令以及`addfragments`指令

| 指令/变量            | 关键词         | 作用/场景示例                          | 使用范围              | 注意事项/区别（含示例） |
|----------------------|----------------|----------------------------------------|-----------------------|-------------------------|
| **inherit**          | 局部立即       | 在 recipe 中继承类，如 `inherit autotools` | `.bb` / `.bbclass`    | 立即生效，只影响当前 recipe<br>示例：`inherit autotools` → 当前 recipe 使用 autotools 构建 |
| **inherit_defer**    | 局部延迟       | 条件继承，避免覆盖，如 `inherit_defer autotools` | `.bb` / `.bbappend`   | 延迟到解析结束；适合条件继承<br>示例：`inherit_defer ${@bb.utils.contains('DISTRO_FEATURES','x11','x11-base','',d)}` |
| **INHERIT**          | 全局继承       | 在 `local.conf` 强制所有 recipe 继承，如 `INHERIT += "rm_work"` | 配置文件 (`conf`)      | 全局作用，影响所有 recipe<br>示例：`INHERIT += "rm_work"` → 所有 recipe 构建完成后清理工作目录 |
| **require**          | 强制引入       | 必须包含共享文件，如 `require common.inc` | `.bb` / `.bbclass` / `.conf` | 文件必须存在，否则报错<br>示例：`require recipes-common/common.inc` |
| **include**          | 可选引入       | 可选包含文件，如 `include optional.inc` | `.bb` / `.bbclass` / `.conf` | 文件不存在不报错；只引入第一个匹配<br>示例：`include debug.inc`（若不存在则跳过） |
| **include_all**      | 全量引入       | 聚合所有层的同名文件，如维护者清单      | `.bb` / `.bbclass` / `.conf` | 包含所有匹配文件；适合跨层聚合<br>示例：`include_all maintainers.inc` → 所有层的 maintainers.inc 都被包含 |
| **EXPORT_FUNCTIONS** | 函数共享       | 导出类函数供 recipe 调用/覆盖，如 `EXPORT_FUNCTIONS do_compile` | `.bbclass`             | 函数需按 `classname_func` 命名；支持覆盖<br>示例：类中定义 `autotools_do_compile` 并 `EXPORT_FUNCTIONS do_compile`，recipe 中可调用或覆盖 `do_compile` |
| **addfragments**     | 片段管理器     | 管理配置片段，如内核特性 `OE_FRAGMENTS` | `.conf` (如 bitbake.conf) | 四参数：路径前缀、启用列表、元数据、内置映射<br>示例：<br>`OE_FRAGMENTS = "core/net/ipv6"`<br>`addfragments conf/fragments OE_FRAGMENTS OE_FRAGMENTS_METADATA_VARS OE_FRAGMENTS_BUILTIN` |
