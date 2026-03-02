# 智能灯泡状态机 (Smart Light State Machine)

本项目实现了一个基于 C 语言的、高度模块化的有限状态机（FSM）系统，用于模拟一个具有双重自动关闭逻辑的智能灯泡。该项目旨在展示如何在 C 语言中运用面向对象的设计思想，特别是通过 `vtable`（虚函数表）模式来实现状态模式。

## 功能描述

- 两种状态：灯泡可以在 OFF (关闭) 和 ON (开启) 两种状态间切换。
- 双重自动关闭：
  - 长时无操作：灯开启后，如果在 30 分钟内没有任何操作，它将自动关闭。
  - 短时延迟关闭：在 ON 状态下按一次开关，灯不会立即关闭，而是进入一个 5 秒的延迟关闭倒计时。在此期间，如果再次按下开关，倒计时会被取消，灯继续亮着；如果倒计时结束，灯将关闭。

- 交互界面：用户可以通过键盘输入 1 来模拟按下开关，输入 0 来退出程序。

## 设计与实现

### 1. 核心模式：vtable (虚函数表)

这是整个设计的灵魂。C 语言本身不支持类和继承，但我们可以通过结构体和函数指针来模拟面向对象的核心特性——**多态**。

- `StateObject` 结构体：代表一个抽象的状态。它包含一个指向 `StateVTable` 的指针 `vptr` 和一个状态名称 name。这个结构体是所有具体状态（如 OffState, OnState）的“基类”。
- `StateVTable` 结构体：即“虚函数表”。它包含了该状态需要实现的所有行为（方法）的函数指针，例如 `handle_event` 和 `update_timer`。不同的状态可以提供自己独特的函数实现。
- `LightContext` 结构体：即“上下文”。它持有一个指向 `StateObject` 的指针，代表当前状态。状态切换就是修改这个指针，使其指向不同的 `StateObject` 实例。

```c
// 事件处理的核心逻辑
ctx->current_state = ctx->current_state->vptr->handle_event(ctx, event);
```

当需要处理事件或更新定时器时，`Context` 通过 `vptr` 调用当前状态的 `vtable` 中对应的函数。由于每个状态的 `vtable` 都不同，这就实现了运行时的动态分派，即**多态**。

**优点：**

- 高内聚低耦合：每个状态的逻辑完全封装在自己的 .c 文件中，互不影响。
- 易于扩展：要添加新状态，只需创建新的 .c/.h 文件，定义其 StateVTable 和 StateObject，然后在 Context 中引用即可，无需修改现有状态的代码。
- 清晰的架构：代码结构清晰，职责分明。

### 2. 状态实现：静态数据与内存优化

在传统的面向对象语言中，一个状态对象通常有自己的成员变量来存储其私有数据。在 C 语言中，我们通过一个技巧来实现这一点。

- `OnState` 的数据：`OnState` 需要记录 `last_activity_time` 和 `delay_start_time` 等信息。这些数据被定义为 `OnStateData` 结构体。
- 内存布局：`OnStateData` 的第一个成员是 `StateObject base`。这意味着 `OnStateData` 实例的内存起始地址与其 `base` 成员的地址是相同的。
- 指针转换：我们创建一个全局的 `static OnStateData` 实例。在 `get_on_state()` 函数中，我们将其地址强制转换为 `const StateObject*` 返回。当 `Context` 调用 `handle_event` 时，`ctx` 参数会被传入，而 `OnState` 的实现函数内部可以将 `ctx->current_state` 指针安全地转换回 `OnStateData*`，从而访问和修改其私有数据。

```c
// 在 OnState 的实现中
OnStateData* self_data = (OnStateData*)ctx->current_state;
// 现在可以访问 self_data->last_activity_time 等成员
```

这种技术被称为“继承”或“嵌入式多态”，它使得状态能够拥有自己的数据，同时又能完美融入 `vtable` 模式。

此外，使用 `static const` 来定义状态实例，确保了状态对象在整个程序生命周期内只存在一份，极大地节省了内存。

### 3. 性能权衡：内联状态处理

理论上最快的实现是将所有状态逻辑合并到一个巨大的 switch 语句中

```c
void handle_event(Context* ctx, Event e) {
    switch(ctx->current_state_id) {
        case STATE_OFF:
            // ... OffState 的所有逻辑 ...
            break;
        case STATE_ON:
            // ... OnState 的所有逻辑 ...
            break;
    }
}
```

这种方式完全消除了函数指针调用的开销，性能最优。然而，它的代价是巨大的**可维护性**和**可扩展性**损失。每当需要添加或修改状态时，都必须修改这个庞大的 `switch` 块，这严重违反了“开闭原则”（对扩展开放，对修改封闭）。

因此，**`vtable`** 模式是在性能、可维护性和设计优雅性之间取得的最佳平衡。只有在性能要求极端苛刻且状态逻辑几乎不变的特殊领域，才会考虑这种牺牲架构的方案。

## 项目结构

```text
.
├── Makefile
├── README.md
└── src/
    ├── main.c              # 主函数入口，包含 Context 的实现
    ├── state_common.h      # 所有状态共享的类型定义
    ├── state_off.c         # OffState 的具体实现
    ├── state_off.h         # OffState 的头文件
    ├── state_on.c          # OnState 的具体实现
    └── state_on.h          # OnState 的头文件
```
