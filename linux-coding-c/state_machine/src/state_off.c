#include "state_off.h"
#include <stdio.h>

// struct transfer_t {

// }

SMStateID_t off_handle_event_impl(const struct StateObject* obj, SMEvent_t event) {
    sm_print_dbg("%s handle event %d\n", obj->name, ev);
    (void)obj;
    LightEvent_t ev = (LightEvent_t)event;
    if (ev == LIGHT_EVENT_TOGGLE) {
        sm_print_info("灯亮了！\n");
        // 返回下一个状态的ID，而不是状态对象
        return (SMStateID_t)LIGHT_STATE_ID_ON;
    }
    else if (ev == LIGHT_EVENT_IDLE) {
        // 保持当前状态
        return (SMStateID_t)LIGHT_STATE_ID_OFF;
    }
    else {
        sm_print_warn("不识别的事件%d\n", event);
        // 保持当前状态
        return (SMStateID_t)LIGHT_STATE_ID_OFF;
    }
}

static const StateVTable off_vtable = {
    .handle_event = off_handle_event_impl
};

StateObject_t* get_off_state(void *) {
    static struct {
        StateObject_t base;
    } instance = {{&off_vtable, "OFF", LIGHT_STATE_ID_OFF}};
    return (StateObject_t*)&instance;
}
