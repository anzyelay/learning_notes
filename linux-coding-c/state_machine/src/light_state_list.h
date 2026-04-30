#ifndef STATE_LIST_HEADER
#define STATE_LIST_HEADER

#include "state_off.h"
#include "state_on.h"

// --- 状态注册表的实现 ---
// 这是将所有状态“注册”到中央位置的关键部分
GetStateFunction g_state_registry[] = {
    [LIGHT_STATE_ID_OFF] = get_off_state,
    [LIGHT_STATE_ID_ON] = get_on_state,
    [LIGHT_STATE_ID_SUM] = NULL
    // 当添加新状态时，在这里追加，例如：
    // [LIGHT_STATE_ID_SLEEPING] = get_sleeping_state,
    // [LIGHT_STATE_ID_ERROR] = get_error_state,
};


#endif
