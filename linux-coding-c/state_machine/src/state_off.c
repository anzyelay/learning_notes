#include "state_off.h"
#include <stdio.h>

extern const StateObject* get_on_state(void);

const StateObject* off_handle_event_impl(struct LightContext* ctx, LightEvent_t event) {
    if (event == LIGHT_EVENT_TOGGLE) {
        printf("灯亮了！\n");
        return get_on_state();
    }
    return get_off_state();
}

const StateObject* off_update_timer_impl(struct LightContext* ctx) {
    return get_off_state();
}

static const StateVTable off_vtable = {
    .handle_event = off_handle_event_impl,
    .update_timer = off_update_timer_impl
};

const StateObject* get_off_state(void) {
    static const struct {
        StateObject base;
    } instance = {{&off_vtable, "OFF"}};
    return (const StateObject*)&instance;
}
