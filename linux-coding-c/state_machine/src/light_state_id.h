#ifndef STATE_COMMON_H
#define STATE_COMMON_H

// --- 状态 ID 枚举：定义所有可用状态 ---
// 这是状态注册表的核心，提供了状态的全局视图
typedef enum {
    LIGHT_STATE_ID_OFF,
    LIGHT_STATE_ID_ON,
    // 当添加新状态时，在这里追加，例如：
    // LIGHT_STATE_ID_SLEEPING,
    // LIGHT_STATE_ID_ERROR,
    LIGHT_STATE_ID_SUM
} LightStateID_t;

typedef enum {
    LIGHT_EVENT_IDLE,
    LIGHT_EVENT_TOGGLE,
    LIGHT_EVENT_SUM
} LightEvent_t;

// 定义 light on 的子状态
typedef enum {
    ON_SUBSTATE_NORMAL,
    ON_SUBSTATE_DELAY_TO_OFF
} OnSubStateID_t;

#endif
