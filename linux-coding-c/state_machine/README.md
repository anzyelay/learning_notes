# 精简版状态机示例

这是一个使用C语言实现的、基于虚函数表（vtable）和中央状态注册表的泛型状态机示例。该项目旨在演示一种可扩展、高内聚、低耦合的状态机设计模式。

## 特性

- **高内聚**：每个状态逻辑被封装在其独立的 `.c` 文件中，职责单一。
- **低耦合**：状态之间通过中央注册表进行通信，无需互相引用头文件，降低了依赖性。
- **易于扩展**：添加新状态只需在 `light_state_id.h` 中定义ID，在`light_state_list.h`注册表中添加入口，并创建相应的 `.c/.h` 文件，即可无缝集成。
- **清晰的状态流转**：通过 `enum` 定义所有状态ID，使所有可能的状态一目了然，状态切换关系清晰可控。
- **模块化设计**：将状态机核心逻辑 (`state_machine.c`) 与应用层逻辑 (`main.c`) 分离，提高了代码的组织性和可维护性。
- **状态变化通知**：`SMContext` 支持设置回调函数，在状态发生改变时自动通知使用者，简化了 `main.c` 的逻辑。
- **高效的状态管理**：`SMContext` 内部仅持有当前 `StateObject` 的指针作为缓存。在事件处理和离开事件时，直接通过该指针调用方法，无需任何查找。状态切换时，通过比较当前对象的ID与新ID来决定是否进行切换，保证了核心循环的高性能。
- **多状态机管理**：通过`sm_context_create`可构建多个`SMContext`分别管理，针对一个状态下面又由多个子状态等可复用`state_machine`的内核实现来处理

## 示例场景

本项目模拟了一个具有双重定时器的智能开关：

- **规则1 (长定时器)**：灯开启后，如果在30分钟内没有任何操作，将自动关闭。
- **规则2 (短定时器)**：按下关闭键后，灯不会立即熄灭，而是会等待5秒后才关闭。在此期间，如果再次按下开关，则会取消本次关闭操作。

## 设计模式

- **状态模式 (State Pattern)**：核心模式，通过不同的对象来表示不同的状态，将状态转换和动作封装在状态对象内部。
- **虚函数表 (Virtual Function Table)**：C语言中模拟面向对象多态性的常用技巧。每个状态对象都有一个指向 `StateVTable` 的指针，其中包含了**处理事件**和**状态离开后**的的函数指针。
- **中央注册表 (Central Registry)**：一个全局数组 `g_state_registry`，将 `LightStateId_t` 映射到具体的 `GetStateFunction`。这使得状态切换逻辑与具体的状态实现解耦，所有状态的获取都通过统一的入口进行。
- **回调模式 (Callback Pattern)**：`LightContext` 允许外部设置一个 `StateChangeCallback`，当内部状态发生变化时自动调用，实现了松耦合的通知机制。

## 文件结构

```text
project_root/
├── README.md
└── src/
    ├── main.c                 # 主程序入口和UI逻辑
    ├── state_machine.c        # 核心实现：状态机核心逻辑和Context实现
    ├── state_machine.h        # 核心定义：StateObject, VTable, Context
    ├── light_state_id/list.h  # 事件, 状态ID枚举, 注册表声明
    ├── state_off.h            # OFF状态的头文件
    ├── state_off.c            # OFF状态的实现
    ├── state_on.h             # ON状态的头文件
    └── state_on.c             # ON状态的实现, 模拟了ON的子状态机`sub_state_registry`的实现
```

## 核心概念

- **`StateObject_t`**：状态的基类，包含一个指向 `StateVTable` 的指针、状态名称和状态ID。
- **`StateVTable`**：虚函数表，定义了每个状态必须实现的方法，如 `handle_event`。
- **`SMContext_t`**：上下文对象, 包含状态机名称，持有一个指向当前 `StateObject` 的指针，并负责驱动状态的流转。指针使得在处理事件时能够以最高的效率执行状态逻辑。状态切换时，通过比较当前对象的 `id` 字段与新状态的ID来判断是否需要更新指针，实现了性能和逻辑的平衡。
- **`LightStateId_t`**：一个枚举，定义了系统中所有可用的状态ID。这是整个状态系统的“地图”。
- **`g_state_registry`**：一个全局的函数指针数组，将 `LightStateId_t` 与 `GetStateFunction` 关联起来。这是状态切换的“路由器”。
- **`StateChangeCallback`**：一个回调函数类型，用于在 `Context` 状态改变时通知外部。
- **`state_machine.c`**：封装了 `SMContext` 的所有操作，包括状态创建、初始化、事件处理、状态切换和销毁，形成了一个完整的状态机服务。

## 编译与运行

您可以使用任何标准的C编译器（如 `gcc` 或 `clang`）来编译此项目。

```bash
cd src
gcc -o state_machine main.c state_machine.c state_off.c state_on.c
./state_machine
```

根据提示输入 `1` 切换状态，输入 `2` 退出程序。

## 如何添加新状态

假设要添加一个 `SLEEPING` 状态：

1. **修改 `state_common.h`**：
    - 在 `LightStateId_t` 枚举中添加 `LIGHT_STATE_ID_SLEEPING`。
2. **修改 `light_state_id/list.h`**：
    - 在 `g_state_registry` 数组中添加 `[LIGHT_STATE_ID_SLEEPING] = get_sleeping_state,` 的条目。
3. **创建新文件 `state_sleeping.h` 和 `state_sleeping.c`**：
    - 实现 `get_sleeping_state` 函数，返回一个配置好的 `StateObject`。
    - 实现 `sleeping_handle_event_impl` 函数，并将其地址填入 `StateVTable`。
    - 在需要切换到睡眠状态时，从 `handle_event` 的实现中返回 `LIGHT_STATE_ID_SLEEPING`。Context会自动通过注册表找到并切换到新状态。

## 创建状态机演示

- step 1: 构建状态驱动表 --> g_state_registry

```c
    GetStateFunction g_state_registry[] = {
        [LIGHT_STATE_ID_OFF] = get_off_state,
        [LIGHT_STATE_ID_ON] = get_on_state,
    };
```

- step 2: 创建状态机

```c
    SMContext_t *light_ctx = sm_context_create("LSM", g_state_registry,
                                SM_ARRAY_SIZE(g_state_registry), NULL);
```

- step 3: 初始化状态

```c
sm_context_intial_state(light_ctx, LIGHT_STATE_ID_OFF);
```

- step 4: 通过EVENT驱动状态机处理事件和状态转移

```c
    sm_context_handle_event(light_ctx, event);
```

- step 5: 销毁

```c
    sm_context_destroy(light_ctx);
```
