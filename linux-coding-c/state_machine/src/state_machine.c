// 将状态机的核心逻辑封装到此文件中
#include "state_machine.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// static char *my_strdup(const char *s)
// {
//     if (!s) return NULL;

//     size_t len = strlen(s) + 1;
//     char *p = malloc(len);
//     if (p) {
//         memcpy(p, s, len);
//     }
//     return p;
// }

void sm_state_object_destroy(StateObject_t *obj)
{
    free(obj);
}

StateObject_t *sm_state_object_create(const char *name, SMStateID_t id, const StateVTable *vptr)
{
    StateObject_t *obj = malloc(sizeof(StateObject_t));
    obj->id = id;
    obj->name = name;
    obj->vptr = vptr;
    return obj;
}
// 通过 current_state->id 获取旧ID，与新ID比较来判断是否切换状态的通用接口
static void sm_context_switch_to_state(SMContext_t* ctx, SMStateID_t state_id) {
    if (state_id >= ctx->registry_num) {
        sm_print_err("%s: the state id %d is out of range 0~%d\n",
            ctx->name,
            state_id, ctx->registry_num-1);
        return;
    }
    // 1. 从当前对象中获取旧ID
    SMStateID_t old_state_id = sm_get_current_state_id(ctx);
    if (old_state_id == -1) {
        // 2. 无初始状态， 直接切换状态指针, 处理新状态进入事件
        ctx->current_state = ctx->state_registry[state_id](ctx->gsf_args);
        sm_print_info("%s: initial state to %s!\n", ctx->name ,sm_get_current_state_name(ctx));
        // 3. 触发切换回调
        if (ctx->on_state_change_callback) {
            ctx->on_state_change_callback("unintial", sm_get_current_state_name(ctx));
        }
        return;
    }

    // 2. 比较ID，这是最核心的判断
    if (old_state_id != state_id) {

        // 3. 保存旧状态
        StateObject_t *old_state = ctx->current_state;

        // 4. 处理新状态进入事件, 切换状态指针
        ctx->current_state = ctx->state_registry[state_id](ctx->gsf_args);

        // 5. 触发切换回调
        if (ctx->on_state_change_callback) {
            const char* old_state_name = old_state->name;
            const char* new_state_name = sm_get_current_state_name(ctx);
            ctx->on_state_change_callback(old_state_name, new_state_name);
        }

        // 6. 处理old state离开事件
        if (old_state->vptr->handle_leave
            && old_state->vptr->handle_leave(old_state) == LEAVE_FREE_SELF) {
            // 7. 旧状态对象是否需要释放
            sm_state_object_destroy(old_state);
        }

    }
    else {
        sm_print_dbg("%s: same state id, keep in state: %s\n", ctx->name, sm_get_current_state_name(ctx));
    }
    return;
}

void sm_context_set_on_state_change_callback(SMContext_t* ctx, StateChangeCallback callback) {
    ctx->on_state_change_callback = callback;
}

void sm_context_handle_event(SMContext_t* ctx, SMEvent_t event) {
    if (!ctx || !ctx->current_state) {
        sm_print_warn("%s: no current state\n", ctx->name);
        return;
    }

    if (ctx->current_state->vptr && ctx->current_state->vptr->handle_event) {
        // 1. 状态实现返回下一个状态的ID
        SMStateID_t next_state_id = ctx->current_state->vptr->handle_event(ctx->current_state, event);
        // 2. 复用统一的状态切换接口
        sm_context_switch_to_state(ctx, next_state_id);
    }
    else {
        sm_print_warn("%s: %s has no vptr or handle_event\n", ctx->name, ctx->current_state->name);
    }
}

const char* sm_get_current_state_name(const SMContext_t* ctx) {
    if (ctx->current_state) {
        return ctx->current_state->name;
    }
    return "UNKNOWN";
}

SMStateID_t sm_get_current_state_id(const SMContext_t* ctx) {
    if (ctx->current_state) {
        return ctx->current_state->id;
    }
    return -1; // 或者定义一个无效状态ID
}

static void sm_context_alloc_registry_table(SMContext_t* ctx, int max_registry_num) {
    ctx->registry_num = max_registry_num;
    ctx->state_registry = malloc(max_registry_num*sizeof(GetStateFunction));
}

static void sm_context_free_registry_table(SMContext_t* ctx) {
    if (ctx->state_registry)
        free(ctx->state_registry);
    ctx->state_registry = NULL;
    ctx->registry_num = 0;
}

static void sm_context_register_state(SMContext_t* ctx, int state_id, GetStateFunction get_state_fun) {
    ctx->state_registry[state_id] = get_state_fun;
}

// Context 的公共接口实现
SMContext_t *sm_context_create(const char *name, GetStateFunction *state_fun_array, int array_size, void *gsf_arg)
{
    SMContext_t *ctx = malloc(sizeof(SMContext_t));
    ctx->current_state = NULL;
    ctx->gsf_args = gsf_arg;
    ctx->on_state_change_callback = NULL;
    ctx->registry_num = array_size;
    ctx->state_registry = NULL;
    ctx->name = name;

    sm_context_alloc_registry_table(ctx, array_size);
    for (int i=0; i < array_size; ++i) {
        sm_context_register_state(ctx, i, state_fun_array[i]);
    }

    return ctx;
}

void sm_context_intial_state(SMContext_t *ctx, SMStateID_t state_id)
{
    sm_context_switch_to_state(ctx, state_id);
}

void sm_context_destroy(SMContext_t *ctx)
{
    // 检查当前状态是否有资源要释放
    if (ctx->current_state &&  ctx->current_state->vptr && ctx->current_state->vptr->handle_leave &&
        ctx->current_state->vptr->handle_leave(ctx->current_state) == LEAVE_FREE_SELF) {
        sm_state_object_destroy(ctx->current_state);
    }
    ctx->current_state = NULL;
    sm_context_free_registry_table(ctx);
    ctx->on_state_change_callback = NULL;
    ctx->gsf_args = NULL;
    free(ctx);
}
