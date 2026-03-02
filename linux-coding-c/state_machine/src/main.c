#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    #define SLEEP(x) Sleep(x)
#else
    #include <sys/select.h>
    #include <unistd.h>
    #define SLEEP(x) do { \
        struct timespec ts = {x / 1000, (x % 1000) * 1000000L}; \
        nanosleep(&ts, NULL); \
    } while(0)
#endif

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

int kbhit() {
#ifdef _WIN32
    return _kbhit();
#else
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout) > 0;
#endif
}

int main() {
    LightContext light_ctx;
    light_context_init(&light_ctx);

    printf("带双重定时器的精简版开关模拟器 (0=退出, 1=切换)\n");
    printf("规则1: 灯开启后，若 %d 分钟内无操作则自动关闭。\n", AUTO_OFF_LONG_DELAY_SECONDS / 60);
    printf("规则2: 按下关闭键后，灯会延迟 %d 秒后关闭。在此期间按任意键可取消。\n", AUTO_OFF_SHORT_DELAY_SECONDS);
    printf("当前状态: %s\n", get_current_state_name(&light_ctx));

    int event_input;
    bool running = true;
    const char* previous_state_name = get_current_state_name(&light_ctx);

    while (running) {
        light_context_update_timer(&light_ctx);

        const char* current_state_name = get_current_state_name(&light_ctx);
        if (strcmp(current_state_name, previous_state_name) != 0) {
            printf("[系统] 状态已自动切换 -> %s\n", current_state_name);
            previous_state_name = current_state_name;
        }

        if (kbhit()) {
            if (scanf("%d", &event_input) == 1) {
                if (event_input == 0) {
                    running = false;
                } else if (event_input == 1) {
                    printf("--- 用户操作 ---\n");
                    light_context_handle_event(&light_ctx, LIGHT_EVENT_TOGGLE);
                    const char* new_state_name = get_current_state_name(&light_ctx);
                    printf("当前状态: %s\n", new_state_name);
                    previous_state_name = new_state_name;
                }
            }
        }

        SLEEP(100);
    }

    return 0;
}

void light_context_init(LightContext* ctx) {
    // 通过注册表初始化
    ctx->current_state = g_state_registry[LIGHT_STATE_ID_OFF]();
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

// 新增：通过ID切换状态的通用接口
void light_context_switch_to_state(LightContext* ctx, LightStateId_t state_id) {
    if (state_id < sizeof(g_state_registry) / sizeof(GetStateFunction)) {
        ctx->current_state = g_state_registry[state_id]();
    }
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
