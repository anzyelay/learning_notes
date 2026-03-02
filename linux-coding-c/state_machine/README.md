# 精简版状态机示例

这是一个使用C语言实现的、基于虚函数表（vtable）和中央状态注册表的精简版状态机示例。该项目旨在演示一种可扩展、高内聚、低耦合的状态机设计模式。

## 特性

- **高内聚**：每个状态逻辑被封装在其独立的 `.c` 文件中，职责单一。
- **低耦合**：状态之间通过中央注册表进行通信，无需互相引用头文件，降低了依赖性。
- **易于扩展**：添加新状态只需在 `state_common.h` 中定义ID，在注册表中添加入口，并创建相应的 `.c/.h` 文件，即可无缝集成。
- **清晰的状态流转**：通过 `enum` 定义所有状态ID，使所有可能的状态一目了然，状态切换关系清晰可控。

## 示例场景

本项目模拟了一个具有双重定时器的智能开关：

- **规则1 (长定时器)**：灯开启后，如果在30分钟内没有任何操作，将自动关闭。
- **规则2 (短定时器)**：按下关闭键后，灯不会立即熄灭，而是会等待5秒后才关闭。在此期间，如果再次按下开关，则会取消本次关闭操作。

## 设计模式

- **状态模式 (State Pattern)**：核心模式，通过不同的对象来表示不同的状态，将状态转换和动作封装在状态对象内部。
- **虚函数表 (Virtual Function Table)**：C语言中模拟面向对象多态性的常用技巧。每个状态对象都有一个指向 `StateVTable` 的指针，其中包含了处理事件和更新定时器的函数指针。
- **中央注册表 (Central Registry)**：一个全局数组 `g_state_registry`，将 `LightStateId_t` 映射到具体的 `GetStateFunction`。这使得状态切换逻辑与具体的状态实现解耦，所有状态的获取都通过统一的入口进行。

## 文件结构

```text
project_root/
├── README.md
└── src/
    ├── main.c                 # 主程序入口和Context实现
    ├── state_common.h         # 核心定义：StateObject, VTable, Context, 事件, 状态ID枚举, 注册表声明
    ├── state_off.h            # OFF状态的头文件
    ├── state_off.c            # OFF状态的实现
    ├── state_on.h             # ON状态的头文件
    └── state_on.c             # ON状态的实现
```

## 核心概念

- **`StateObject`**：状态的基类，包含一个指向 `StateVTable` 的指针、状态名称和状态ID。
- **`StateVTable`**：虚函数表，定义了每个状态必须实现的方法，如 `handle_event` 和 `update_timer`。
- **`LightContext`**：上下文对象，持有一个指向当前 `StateObject` 的指针，并负责驱动状态的流转。
- **`LightStateId_t`**：一个枚举，定义了系统中所有可用的状态ID。这是整个状态系统的“地图”。
- **`g_state_registry`**：一个全局的函数指针数组，将 `LightStateId_t` 与 `GetStateFunction` 关联起来。这是状态切换的“路由器”。

## 编译与运行

您可以使用任何标准的C编译器（如 `gcc` 或 `clang`）来编译此项目。

```bash
cd src
gcc -o state_machine main.c state_off.c state_on.c
./state_machine
```

根据提示输入 `1` 切换状态，输入 `0` 退出程序。

## 如何添加新状态

假设要添加一个 `SLEEPING` 状态：

1. **修改 `state_common.h`**：
    - 在 `LightStateId_t` 枚举中添加 `LIGHT_STATE_ID_SLEEPING`。
2. **修改 `main.c`**：
    - 在 `g_state_registry` 数组中添加 `[LIGHT_STATE_ID_SLEEPING] = get_sleeping_state,` 的条目。
3. **创建新文件 `state_sleeping.h` 和 `state_sleeping.c`**：
    - 实现 `get_sleeping_state` 函数，返回一个配置好的 `StateObject`。
    - 实现 `sleeping_handle_event_impl` 和 `sleeping_update_timer_impl` 函数，并将其地址填入 `StateVTable`。
    - 在需要切换到睡眠状态时，从 `handle_event` 或 `update_timer` 的实现中返回 `LIGHT_STATE_ID_SLEEPING`。Context会自动通过注册表找到并切换到新状态。
