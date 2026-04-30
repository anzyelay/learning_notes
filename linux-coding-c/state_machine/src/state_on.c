#include "state_on.h"
#include <stdio.h>
#include <time.h>

typedef struct {
    StateObject_t base;
    SMContext_t *sub_ctx; // OnState 管理自己的多种模式
    LightStateID_t next_id;
} OnStateData;

typedef struct {
    StateObject_t base;
    OnStateData *parent;
    time_t last_activity_time;      // 切入正常模式下的开始时间
} OnSubNormalStateData;

typedef struct {
    StateObject_t base;
    OnStateData *parent;
    time_t delay_start_time;        // 切入延迟模式下的开始时间
} OnSubDelayStateData;


SMStateID_t on_n_handle_event_impl(const struct StateObject* obj, SMEvent_t event) {
    sm_print_dbg("\t%s handle event %d\n", obj->name, ev);
    OnSubNormalStateData *self_data = (OnSubNormalStateData*)obj;
    OnStateData *parent = self_data->parent;
    LightEvent_t ev = (LightEvent_t)event;
    if (ev == LIGHT_EVENT_TOGGLE) {
        // 如果处于正常模式，按开关则进入延迟模式
        sm_print_info("灯进入关闭延迟...\n");
        parent->next_id = LIGHT_STATE_ID_ON;
        return ON_SUBSTATE_DELAY_TO_OFF;
    }
    else if (ev == LIGHT_EVENT_IDLE) {
        // 如果处于正常模式，检查是否超时
        time_t current_time = time(NULL);
        if(current_time - self_data->last_activity_time > AUTO_OFF_LONG_DELAY_SECONDS) {
            sm_print_info("\n[OnState 内部定时器: 长时间未操作，灯自动熄灭！]\n");
            // 返回下一个状态的ID
            parent->next_id = LIGHT_STATE_ID_OFF;
        }
        else {
            // 定时器未超时，保持当前状态，返回其ID
            parent->next_id = LIGHT_STATE_ID_ON;
        }
    }
    return ON_SUBSTATE_NORMAL;
}

static const StateVTable on_sub_n_vtable = {
    .handle_event = on_n_handle_event_impl
};

StateObject_t *get_on_normal_substate(void *parent) {
    static OnSubNormalStateData instance = {
        .base = {&on_sub_n_vtable, "NORMAL ON", ON_SUBSTATE_NORMAL},
        .parent = NULL,
        .last_activity_time = 0,
    };
    // 切入此状态时设置ON开始时间戳
    instance.parent = (OnStateData *)parent;
    instance.last_activity_time = time(NULL);
    return (StateObject_t *)&instance;
}

SMStateID_t on_d_handle_event_impl(const struct StateObject *obj, SMEvent_t event) {
    sm_print_dbg("\t%s handle event %d\n", obj->name, ev);
    OnSubDelayStateData * self_data = (OnSubDelayStateData*)obj;
    OnStateData *parent = self_data->parent;
    LightEvent_t ev = (LightEvent_t)event;
    if (ev == LIGHT_EVENT_TOGGLE) {
        // 如果正处于延迟模式，按开关会取消关闭
        sm_print_info("延迟关闭被取消，灯继续保持开启！\n");
        parent->next_id = LIGHT_STATE_ID_ON;
        return ON_SUBSTATE_NORMAL;
    }
    else if (ev == LIGHT_EVENT_IDLE) {
        time_t current_time = time(NULL);
        if (current_time - self_data->delay_start_time >= AUTO_OFF_SHORT_DELAY_SECONDS) {
            sm_print_info("\n[OnState 内部定时器: 延迟时间到，灯已关闭]\n");
            // 返回下一个状态的ID
            parent->next_id = LIGHT_STATE_ID_OFF;
        }
        else {
            parent->next_id = LIGHT_STATE_ID_ON;
        }
    }
    return ON_SUBSTATE_DELAY_TO_OFF;
}
static const StateVTable on_sub_d_vtable = {
    .handle_event = on_d_handle_event_impl
};

StateObject_t* get_on_delay_substate(void *parent) {
    static OnSubDelayStateData instance = {
        .base = {&on_sub_d_vtable, "DELAY TO OFF", ON_SUBSTATE_DELAY_TO_OFF},
        .parent = NULL,
        .delay_start_time = 0,
    };
    // 切入此状态时设置DELAY开始时间戳
    instance.delay_start_time = time(NULL);
    instance.parent = (OnStateData *)parent;
    return (StateObject_t *)&instance;
}

static SMStateID_t on_handle_event_impl(const struct StateObject *obj, SMEvent_t event) {
    sm_print_dbg("%s handle event %d\n", obj->name, event);
    OnStateData *self_data = (OnStateData *)obj;

    sm_context_handle_event(self_data->sub_ctx, event);

    return self_data->next_id;
}

static LeaveResult_t on_handle_leave_impl(struct StateObject *obj) {
    sm_print_info("%s handle leave\n", obj->name);
    OnStateData *self_data = (OnStateData *)obj;
    sm_context_destroy(self_data->sub_ctx);
    return LEAVE_DO_NOTHING;
}

static const StateVTable on_vtable = {
    .handle_event = on_handle_event_impl,
    .handle_leave = on_handle_leave_impl
};

static void on_light_state_changed(const char* old_state_name, const char* new_state_name) {
    printf("[子系统] 状态: %s -> %s\n", old_state_name, new_state_name);
}

StateObject_t* get_on_state(void *) {
    // 将状态实例定义为 static const 局部变量，优化内存
    static OnStateData instance = {
        .base = {&on_vtable, "ON", LIGHT_STATE_ID_ON},
        .sub_ctx = NULL
    };

    GetStateFunction sub_state_registry[] = {
        [ON_SUBSTATE_NORMAL] = get_on_normal_substate,
        [ON_SUBSTATE_DELAY_TO_OFF] = get_on_delay_substate
    };

    instance.sub_ctx = sm_context_create("CLOM", sub_state_registry,
                            SM_ARRAY_SIZE(sub_state_registry), &instance);
    sm_context_set_on_state_change_callback(instance.sub_ctx, on_light_state_changed);
    sm_context_intial_state(instance.sub_ctx, ON_SUBSTATE_NORMAL);

    return (StateObject_t *)&instance;
}
