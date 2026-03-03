// 现在 main.c 只负责用户界面交互和主循环驱动
#define _GNU_SOURCE // 在所有其他头文件之前定义，以启用POSIX功能
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

#include "state_common.h" // 引入状态机接口

// 定义一个状态变化回调函数
void on_light_state_changed(const char* old_state_name, const char* new_state_name) {
    printf("[系统] 状态已自动切换 -> %s\n", new_state_name);
}

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

    // 设置状态变化回调
    light_context_set_on_state_change_callback(&light_ctx, on_light_state_changed);

    printf("带双重定时器的精简版开关模拟器 (0=退出, 1=切换)\n");
    printf("规则1: 灯开启后，若 %d 分钟内无操作则自动关闭。\n", AUTO_OFF_LONG_DELAY_SECONDS / 60);
    printf("规则2: 按下关闭键后，灯会延迟 %d 秒后关闭。在此期间按任意键可取消。\n", AUTO_OFF_SHORT_DELAY_SECONDS);
    printf("当前状态: %s\n", get_current_state_name(&light_ctx));

    int event_input;
    bool running = true;

    while (running) {
        // 驱动状态机的定时器更新
        light_context_update_timer(&light_ctx);

        if (kbhit()) {
            if (scanf("%d", &event_input) == 1) {
                if (event_input == 0) {
                    running = false;
                } else if (event_input == 1) {
                    printf("--- 用户操作 ---\n");
                    // 驱动状态机处理事件
                    light_context_handle_event(&light_ctx, LIGHT_EVENT_TOGGLE);
                    const char* new_state_name = get_current_state_name(&light_ctx);
                    printf("当前状态: %s\n", new_state_name);
                }
            }
        }

        SLEEP(100);
    }

    return 0;
}
