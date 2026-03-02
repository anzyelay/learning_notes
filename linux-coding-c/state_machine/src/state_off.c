#include "state_off.h"
#include <stdio.h>
#include "state_common.h" // 引入注册表和ID

LightStateId_t off_handle_event_impl(struct LightContext* ctx, LightEvent_t event) {
    if (event == LIGHT_EVENT_TOGGLE) {
        printf("灯亮了！\n");
        // 返回下一个状态的ID，而不是状态对象
        return LIGHT_STATE_ID_ON;
    }
    // 保持当前状态
    return LIGHT_STATE_ID_OFF;
}

LightStateId_t off_update_timer_impl(struct LightContext* ctx) {
    // 保持当前状态
    return LIGHT_STATE_ID_OFF;
}

static const StateVTable off_vtable = {
    .handle_event = off_handle_event_impl,
    .update_timer = off_update_timer_impl
};

const StateObject* get_off_state(void) {
    static const struct {
        StateObject base;
    } instance = {{&off_vtable, "OFF", LIGHT_STATE_ID_OFF}};
    return (const StateObject*)&instance;
}
