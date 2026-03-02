// state_common.h
#ifndef STATE_COMMON_H
#define STATE_COMMON_H

#include <time.h>

// 提前声明结构体，以便在类型定义中相互引用
struct LightContext;
struct StateObject;

typedef enum {
    LIGHT_EVENT_TOGGLE,
} LightEvent_t;

// typedef 用于定义函数指针类型
typedef const struct StateObject* (*EventHandler)(struct LightContext* ctx, LightEvent_t event);
typedef const struct StateObject* (*TimerUpdater)(struct LightContext* ctx);

// --- 虚函数表 (vtable) ---
typedef struct {
    EventHandler handle_event;
    TimerUpdater update_timer;
} StateVTable;

// --- 状态基类 (StateObject) ---
typedef struct StateObject {
    const StateVTable* vptr;
    const char* name;
} StateObject;

// --- 上下文 (Context) ---
typedef struct LightContext {
    const StateObject* current_state;
} LightContext;

// 时间常量
#define AUTO_OFF_LONG_DELAY_SECONDS (30 * 60) // 30 分钟
#define AUTO_OFF_SHORT_DELAY_SECONDS 5        // 5 秒

// Context 的公共接口
void light_context_init(LightContext* ctx);
void light_context_handle_event(LightContext* ctx, LightEvent_t event);
void light_context_update_timer(LightContext* ctx);
const char* get_current_state_name(const LightContext* ctx);

#endif // STATE_COMMON_H
