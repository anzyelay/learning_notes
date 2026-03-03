// 将状态机的核心逻辑封装到此文件中
#include "state_common.h"
#include "state_off.h"
#include "state_on.h"

// --- 状态注册表的实现 ---
// 这是将所有状态“注册”到中央位置的关键部分
GetStateFunction g_state_registry[] = {
    [LIGHT_STATE_ID_OFF] = get_off_state,
    [LIGHT_STATE_ID_ON] = get_on_state,
    // 当添加新状态时，在这里追加，例如：
    // [LIGHT_STATE_ID_SLEEPING] = get_sleeping_state,
    // [LIGHT_STATE_ID_ERROR] = get_error_state,
};

// Context 的公共接口实现
void light_context_init(LightContext* ctx) {
    // 通过注册表初始化
    ctx->current_state = g_state_registry[LIGHT_STATE_ID_OFF]();
    ctx->on_state_change_callback = NULL;
}

void light_context_handle_event(LightContext* ctx, LightEvent_t event) {
    if (ctx->current_state && ctx->current_state->vptr) {
        // 1. 状态实现返回下一个状态的ID
        LightStateId_t next_state_id = ctx->current_state->vptr->handle_event(ctx, event);
        // 2. 复用统一的状态切换接口
        light_context_switch_to_state(ctx, next_state_id);
    }
}

void light_context_update_timer(LightContext* ctx) {
    if (ctx->current_state && ctx->current_state->vptr && ctx->current_state->vptr->update_timer) {
        // 1. 状态实现返回下一个状态的ID
        LightStateId_t next_state_id = ctx->current_state->vptr->update_timer(ctx);
        // 2. 复用统一的状态切换接口
        light_context_switch_to_state(ctx, next_state_id);
    }
}

// 通过 current_state->id 获取旧ID，与新ID比较来判断是否切换状态的通用接口
void light_context_switch_to_state(LightContext* ctx, LightStateId_t state_id) {
    if (state_id < sizeof(g_state_registry) / sizeof(GetStateFunction)) {
        // 1. 从当前对象中获取旧ID
        LightStateId_t old_state_id = ctx->current_state->id;

        // 2. 比较ID，这是最核心的判断
        if (old_state_id != state_id) {
            // 3. 更新对象指针
            ctx->current_state = g_state_registry[state_id]();

            // 4. 触发回调
            if (ctx->on_state_change_callback) {
                const char* old_state_name = g_state_registry[old_state_id]()->name;
                const char* new_state_name = ctx->current_state->name;

                ctx->on_state_change_callback(old_state_name, new_state_name);
            }
        }
    }
}

void light_context_set_on_state_change_callback(LightContext* ctx, StateChangeCallback callback) {
    ctx->on_state_change_callback = callback;
}

const char* get_current_state_name(const LightContext* ctx) {
    if (ctx->current_state) {
        return ctx->current_state->name;
    }
    return "UNKNOWN";
}

LightStateId_t get_current_state_id(const LightContext* ctx) {
    if (ctx->current_state) {
        return ctx->current_state->id;
    }
    return -1; // 或者定义一个无效状态ID
}
