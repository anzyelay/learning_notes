#include "state_on.h"
#include <stdio.h>
#include "state_common.h" // 引入注册表和ID

// 定义 OnState 的子状态
typedef enum {
    ON_SUBSTATE_NORMAL,
    ON_SUBSTATE_DELAY_TO_OFF
} OnSubState_t;

// OnState 管理自己的多种模式
typedef struct {
    StateObject base;
    time_t last_activity_time;      // 正常模式下的最后活动时间
    time_t delay_start_time;        // 延迟模式下的开始时间
    OnSubState_t substate;          // 使用枚举，意图更明确
} OnStateData;

LightStateId_t on_handle_event_impl(struct LightContext* ctx, LightEvent_t event) {
    OnStateData* self_data = (OnStateData*)ctx->current_state;

    if (event == LIGHT_EVENT_TOGGLE) {
        if(self_data->substate == ON_SUBSTATE_DELAY_TO_OFF) {
            // 如果正处于延迟模式，按开关会取消关闭
            printf("延迟关闭被取消，灯继续保持开启！\n");
            self_data->substate = ON_SUBSTATE_NORMAL;
            self_data->last_activity_time = time(NULL);
            // 保持当前状态，返回其ID
            return LIGHT_STATE_ID_ON;
        } else {
            // 如果处于正常模式，按开关则进入延迟模式
            printf("灯进入关闭延迟...\n");
            self_data->substate = ON_SUBSTATE_DELAY_TO_OFF;
            self_data->delay_start_time = time(NULL);
            // 依然保持在ON状态，只是内部模式变了
            return LIGHT_STATE_ID_ON;
        }
    }

    // 处理其他事件（如果有），并确保活动时间被更新
    // 只有在 NORMAL 模式下，其他事件才会重置长时间计时器
    if(self_data->substate == ON_SUBSTATE_NORMAL) {
        self_data->last_activity_time = time(NULL);
    }
    // 保持当前状态，返回其ID
    return LIGHT_STATE_ID_ON;
}

LightStateId_t on_update_timer_impl(struct LightContext* ctx) {
    OnStateData* self_data = (OnStateData*)ctx->current_state;
    time_t current_time = time(NULL);

    if(self_data->substate == ON_SUBSTATE_DELAY_TO_OFF) {
        // 如果处于延迟模式，检查延迟是否结束
        if(current_time - self_data->delay_start_time >= AUTO_OFF_SHORT_DELAY_SECONDS) {
            printf("\n[OnState 内部定时器: 延迟时间到，灯已关闭]\n");
            // 返回下一个状态的ID
            return LIGHT_STATE_ID_OFF;
        }
    } else { // ON_SUBSTATE_NORMAL
        // 如果处于正常模式，检查是否超时
        if(current_time - self_data->last_activity_time > AUTO_OFF_LONG_DELAY_SECONDS) {
            printf("\n[OnState 内部定时器: 长时间未操作，灯自动熄灭！]\n");
            // 返回下一个状态的ID
            return LIGHT_STATE_ID_OFF;
        }
    }
    // 定时器未超时，保持当前状态，返回其ID
    return LIGHT_STATE_ID_ON;
}

static const StateVTable on_vtable = {
    .handle_event = on_handle_event_impl,
    .update_timer = on_update_timer_impl
};

const StateObject* get_on_state(void) {
    // 将状态实例定义为 static const 局部变量，优化内存
    static OnStateData instance = {
        .base = {&on_vtable, "ON", LIGHT_STATE_ID_ON},
        .last_activity_time = 0,
        .delay_start_time = 0,
        .substate = ON_SUBSTATE_NORMAL
    };

    // 初始化时设置时间戳
    instance.last_activity_time = time(NULL);
    instance.substate = ON_SUBSTATE_NORMAL;

    return (const StateObject*)&instance;
}
