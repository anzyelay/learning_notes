#ifndef STATE_ON_H
#define STATE_ON_H

// 引入注册表和ID
#include "state_machine.h"
#include "light_state_id.h"

// 时间常量
#define AUTO_OFF_LONG_DELAY_SECONDS (30 * 60) // 30 分钟
#define AUTO_OFF_SHORT_DELAY_SECONDS 5        // 5 秒

StateObject_t* get_on_state(void *);

#endif // STATE_ON_H
