#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include <stdio.h>

// 提前声明结构体，以便在类型定义中相互引用
struct SMContext;
struct StateObject;

#define SM_ARRAY_SIZE(x)         (sizeof(x) / sizeof((x)[0]))

// #define sm_print_dbg(x, ...) printf("SM <D>: "x, ##__VA_ARGS__)
#define sm_print_dbg(x, ...)
#define sm_print_info(x, ...) printf("SM <I>: "x, ##__VA_ARGS__)
#define sm_print_warn(x, ...) printf("SM <W>: "x, ##__VA_ARGS__)
#define sm_print_err(x, ...) printf("SM <E>: "x, ##__VA_ARGS__)

// --- 状态注册表：将状态ID映射到获取函数 ---
// 这样可以在Context中通过ID来切换状态，而不是直接调用各状态的获取函数
typedef struct StateObject* (*GetStateFunction)(void *);
typedef int SMStateID_t;
typedef int SMEvent_t;

// typedef 用于定义函数指针类型
// 现在，handler 返回的是状态ID，而不是状态对象
typedef SMStateID_t (*EventHandler)(const struct StateObject* ctx, SMEvent_t event);

typedef enum {
    LEAVE_DO_NOTHING,
    LEAVE_FREE_SELF
} LeaveResult_t;

// --- 虚函数表 (vtable) ---
typedef struct {
    EventHandler handle_event;
    LeaveResult_t (*handle_leave)(struct StateObject* obj); // 当前状态对象在切换走后是否需释放？
} StateVTable;

// --- 状态基类 (StateObject) ---
typedef struct StateObject {
    const StateVTable* vptr;
    const char* name;
    SMStateID_t id; // 每个状态对象都包含自己的ID，方便查询
} StateObject_t;

// --- 状态变化回调函数类型 ---
typedef void (*StateChangeCallback)(const char* old_state_name, const char* new_state_name);

// --- 上下文 (Context) ---
typedef struct SMContext {
    StateObject_t* current_state;
    StateChangeCallback on_state_change_callback;
    GetStateFunction *state_registry;
    void *gsf_args; // args for GetStageFunction
    int  registry_num;
    const char *name;
} SMContext_t;


StateObject_t *sm_state_object_create(const char *name, SMStateID_t id, const StateVTable *vptr);
void sm_state_object_destroy(StateObject_t *obj);

// Context 的公共接口 (由 state_machine.c 实现)
SMContext_t *sm_context_create(const char *name, GetStateFunction *state_fun_array, int array_size, void *gsf_arg);
void sm_context_intial_state(SMContext_t *ctx, SMStateID_t state_id);
void sm_context_destroy(SMContext_t *ctx);

void sm_context_handle_event(SMContext_t* ctx, SMEvent_t event);
void sm_context_set_on_state_change_callback(SMContext_t* ctx, StateChangeCallback callback);
const char* sm_get_current_state_name(const SMContext_t* ctx);
SMStateID_t sm_get_current_state_id(const SMContext_t* ctx);

#endif // STATE_COMMON_H
