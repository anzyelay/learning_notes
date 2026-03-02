#ifndef STATE_COMMON_H
#define STATE_COMMON_H

#include <time.h>

// 提前声明结构体，以便在类型定义中相互引用
struct LightContext;
struct StateObject;

typedef enum {
    LIGHT_EVENT_TOGGLE,
} LightEvent_t;

// --- 状态 ID 枚举：定义所有可用状态 ---
// 这是状态注册表的核心，提供了状态的全局视图
typedef enum {
    LIGHT_STATE_ID_OFF,
    LIGHT_STATE_ID_ON,
    // 当添加新状态时，在这里追加，例如：
    // LIGHT_STATE_ID_SLEEPING,
    // LIGHT_STATE_ID_ERROR,
} LightStateId_t;

// --- 状态注册表：将状态ID映射到获取函数 ---
// 这样可以在Context中通过ID来切换状态，而不是直接调用各状态的获取函数
typedef const struct StateObject* (*GetStateFunction)(void);
extern GetStateFunction g_state_registry[];

// typedef 用于定义函数指针类型
// 现在，handler 返回的是状态ID，而不是状态对象
typedef LightStateId_t (*EventHandler)(struct LightContext* ctx, LightEvent_t event);
typedef LightStateId_t (*TimerUpdater)(struct LightContext* ctx);

// --- 虚函数表 (vtable) ---
typedef struct {
    EventHandler handle_event;
    TimerUpdater update_timer;
} StateVTable;

// --- 状态基类 (StateObject) ---
typedef struct StateObject {
    const StateVTable* vptr;
    const char* name;
    LightStateId_t id; // 每个状态对象都包含自己的ID，方便查询
} StateObject;

// --- 状态变化回调函数类型 ---
typedef void (*StateChangeCallback)(const char* old_state_name, const char* new_state_name);

// --- 上下文 (Context) ---
typedef struct LightContext {
    const StateObject* current_state;
    StateChangeCallback on_state_change_callback;
} LightContext;

// 时间常量
#define AUTO_OFF_LONG_DELAY_SECONDS (30 * 60) // 30 分钟
#define AUTO_OFF_SHORT_DELAY_SECONDS 5        // 5 秒

// Context 的公共接口 (由 state_machine.c 实现)
void light_context_init(LightContext* ctx);
void light_context_handle_event(LightContext* ctx, LightEvent_t event);
void light_context_update_timer(LightContext* ctx);
void light_context_switch_to_state(LightContext* ctx, LightStateId_t state_id);
void light_context_set_on_state_change_callback(LightContext* ctx, StateChangeCallback callback);
const char* get_current_state_name(const LightContext* ctx);
LightStateId_t get_current_state_id(const LightContext* ctx);

#endif // STATE_COMMON_H
