#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include <stdio.h>

/* 前向声明结构体，以便在类型定义中相互引用 */
// --- 状态基类 (StateObject) ---
struct StateObject;
// --- 上下文 (Context) ---
struct SMContext;
typedef struct SMContext SMContext_t;

/* 宏定义 */
#define SM_ARRAY_SIZE(x)         (sizeof(x) / sizeof((x)[0]))

// #define sm_print_dbg(x, ...) printf("SM <D>: "x, ##__VA_ARGS__)
#define sm_print_dbg(x, ...)
#define sm_print_info(x, ...) printf("SM <I>: "x, ##__VA_ARGS__)
#define sm_print_warn(x, ...) printf("SM <W>: "x, ##__VA_ARGS__)
#define sm_print_err(x, ...) printf("SM <E>: "x, ##__VA_ARGS__)

// 状态获取函数别名, 用来定义状态注册表
typedef struct StateObject* (*GetStateFunction)(void *);
// 在同一个上下文中, 每个状态有一个独立且唯一的状态ID，
typedef int SMStateID_t;
typedef int SMEvent_t;

typedef enum {
    LEAVE_DO_NOTHING,
    LEAVE_FREE_SELF
} LeaveResult_t;

// --- 虚函数表 (vtable) ---
typedef struct {
    // 当前状态的处理接口，返回的标识下一个状态ID, 此ID用于上下文内部自动切换下一个运行状态， 必须实现
    SMStateID_t (*handle_event)(const struct StateObject* ctx, SMEvent_t event);
    // 返回值标识当前状态对象在切换走后是否需释放？可选实现
    LeaveResult_t (*handle_leave)(struct StateObject* obj);
} StateVTable;

// --- 状态基类 ---
typedef struct StateObject {
    const StateVTable* vptr;
    const char* name;
    SMStateID_t id; // 每个状态对象都包含自己的ID，方便查询
} StateObject_t;

// --- 状态变化回调函数类型 ---
typedef void (*StateChangeCallback)(const char* old_state_name, const char* new_state_name);

// 状态创建相关接口, 在同一个上下文中, 每个状态有一个独立且唯一的状态ID，
StateObject_t *sm_state_object_create(const char *name, SMStateID_t id, const StateVTable *vptr);
void sm_state_object_destroy(StateObject_t *obj);
#define SM_STATE_OBJECT_DEFINE(_name_, _id_, _vptr_) \
    (StateObject_t) {   \
        .vptr = (_vptr_),   \
        .name = (_name_),   \
        .id = (_id_)        \
    }

// Context 的公共接口 (由 state_machine.c 实现), state_fun_array就是所有状态对象的数组
SMContext_t *sm_context_create(const char *name, GetStateFunction *state_fun_array, int array_size, void *gsf_arg);
void sm_context_intial_state(SMContext_t *ctx, SMStateID_t state_id);
void sm_context_destroy(SMContext_t *ctx);

void sm_context_handle_event(SMContext_t* ctx, SMEvent_t event);
void sm_context_set_on_state_change_callback(SMContext_t* ctx, StateChangeCallback callback);
const char* sm_get_current_state_name(const SMContext_t* ctx);
SMStateID_t sm_get_current_state_id(const SMContext_t* ctx);

#endif // STATE_COMMON_H
