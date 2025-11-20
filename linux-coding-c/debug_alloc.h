// 参考文档 dbg-内存异常调试方法
// 包含 TLS 优化的内存调试器
#ifndef DEBUG_ALLOC_TLS_H
#define DEBUG_ALLOC_TLS_H

#ifdef DEBUG_MEMORY_ENABLED

#define MODE_DEBUG_ABORT_ON_INVALID_FREE    1  // 调试模式：非法释放时中止程序


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <pthread.h>
#include <dlfcn.h>

#define FILL_ALLOC_PATTERN 0xAA
#define FILL_FREE_PATTERN  0xDD
#define FILL_REDZONE_PATTERN 0xEE
// memory struct: redzone + usrer data + redzone
#define REDZONE_SIZE 16 // 每个分配块前后的红区大小, 用于检测越界写入
#define MAX_BACKTRACE_DEPTH 10
#define MAX_STACK_DEPTH 10
#define MAX_THREAD_NAME 32

typedef struct AllocInfo {
    void *ptr; // 用户实际使用的指针（跳过红区）
    size_t size; // 用户请求的大小
    const char *file;
    int line;
    void *stack_addrs[MAX_STACK_DEPTH];
    int freed; // 是否已释放
    int stack_depth;
    struct AllocInfo *next;
} AllocInfo;

// 每个线程独立的分配记录链表
__thread AllocInfo *thread_alloc_list = NULL;

// 全局线程注册表（用于最终合并）
typedef struct ThreadRecord {
    pthread_t tid;
    AllocInfo *alloc_list;  // 从TLS复制的记录
    char name[MAX_THREAD_NAME];
    struct ThreadRecord *next;
} ThreadRecord;

static ThreadRecord *global_thread_list = NULL;
static pthread_mutex_t global_list_lock = PTHREAD_MUTEX_INITIALIZER;

static void print_backtrace() {
    void *buffer[MAX_BACKTRACE_DEPTH];
    int nptrs = backtrace(buffer, MAX_BACKTRACE_DEPTH);
    char **symbols = backtrace_symbols(buffer, nptrs);
    for (int i = 0; i < nptrs; i++) {
        printf("    [%d] %s\n", i, symbols[i]);
    }
    free(symbols);
}

/* 添加分配记录 */
static void add_alloc(void *ptr, size_t size, const char *file, int line) {

    AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo));
    if (!info) {
        return;
    }

    info->ptr = ptr;
    info->size = size;
    info->file = file;
    info->line = line;
    info->freed = 0;

    // 获取调用栈地址， 不解析为符号以节省开销, 在报告时再解析, 因为解析符号开销较大
    info->stack_depth = backtrace(info->stack_addrs, MAX_STACK_DEPTH);
    // 跳过前两帧（add_alloc 和调用的分配函数）
    if (info->stack_depth > 2) {
        memmove(info->stack_addrs, info->stack_addrs + 2,
                (info->stack_depth - 2) * sizeof(void *));
        info->stack_depth -= 2;
    }

    info->next = thread_alloc_list;
    thread_alloc_list = info;

}

/* 查找alloc信息 */
static AllocInfo *find_alloc_info(void *ptr) {
    AllocInfo *curr = thread_alloc_list;
    while (curr) {
        if (curr->ptr == ptr) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

static void register_current_thread() {
    pthread_mutex_lock(&global_list_lock);
    ThreadRecord *record = (ThreadRecord *)malloc(sizeof(ThreadRecord));
    record->tid = pthread_self();
    record->alloc_list = thread_alloc_list;

    pthread_getname_np(pthread_self(), record->name, MAX_THREAD_NAME);
    if (record->name[0] == '\0') {
        snprintf(record->name, MAX_THREAD_NAME, "thread_%lu", (unsigned long)pthread_self());
    }

    record->next = global_thread_list;
    global_thread_list = record;
    pthread_mutex_unlock(&global_list_lock);
}

/*泄漏报告*/
static void report_leaks() {
    register_current_thread();

    printf("\n[LEAK REPORT - TLS Optimized]\n");

    pthread_mutex_lock(&global_list_lock);
    ThreadRecord *thread_rec = global_thread_list;
    int total_leaks = 0;

    while (thread_rec) {
        printf("\n--- Leaks in %s (TID: %lu) ---\n",
               thread_rec->name, (unsigned long)thread_rec->tid);

        AllocInfo *curr = thread_rec->alloc_list;
        int thread_leaks = 0;

        while (curr) {
            if (!curr->freed) { // 仅报告未释放的块
                printf("Leak: ptr=%p size=%zu at %s:%d\n",
                    curr->ptr, curr->size, curr->file, curr->line);
                printf("Backtrace:\n");
                // 此时才解析泄漏块符号
                for (int i = 0; i < curr->stack_depth; i++) {
                    Dl_info dlinfo;
                    if (dladdr(curr->stack_addrs[i], &dlinfo) && dlinfo.dli_sname) {
                        printf("    [%d] %s + %lu\n", i,
                            dlinfo.dli_sname,
                            (unsigned long)((char *)curr->stack_addrs[i] - (char *)dlinfo.dli_saddr));
                    } else {
                        printf("    [%d] %p\n", i, curr->stack_addrs[i]);
                    }
                }
                thread_leaks++;
            }

            AllocInfo *next = curr->next;
            free(curr); // 释放记录结构体
            curr = next;
        }

        printf("Thread %s has %d leaks.\n", thread_rec->name, thread_leaks);
        total_leaks += thread_leaks;

        ThreadRecord *next_thread = thread_rec->next;
        free(thread_rec); // 释放线程记录结构体
        thread_rec = next_thread;
    }
    global_thread_list = NULL;

    if (total_leaks == 0) {
        printf("No memory leaks detected across all threads.\n");
    } else {
        printf("\nTotal leaks: %d\n", total_leaks);
    }
    pthread_mutex_unlock(&global_list_lock);
}

static void *debug_malloc_impl(size_t size, const char *file, int line) {
    size_t total_size = size + 2 * REDZONE_SIZE;
    void *ptr = malloc(total_size);
    if (!ptr) return NULL;

    // 设置红区
    memset(ptr, FILL_REDZONE_PATTERN, REDZONE_SIZE);
    memset((char *)ptr + REDZONE_SIZE + size, FILL_REDZONE_PATTERN, REDZONE_SIZE);

    // 填充用户数据区
    void *usr_ptr = (char *)ptr + REDZONE_SIZE;
    memset(usr_ptr, FILL_ALLOC_PATTERN, size);

    printf("[ALLOC] ptr=%p size=%zu at %s:%d\n", usr_ptr, size, file, line);

    add_alloc(usr_ptr, size, file, line);
    return usr_ptr;
}

static void debug_free_impl(void *ptr, const char *file, int line) {
    if (!ptr) return;

    AllocInfo *info = find_alloc_info(ptr);
    if (!info) {
        printf("[FREE] ptr=%p NOT FOUND in alloc records at %s:%d\n", ptr, file, line);
        #if MODE_DEBUG_ABORT_ON_INVALID_FREE
            // way 1: 非法释放，直接中止程序, 适用于调试阶段,生产环境慎用
            abort();
        #else
            // way 2: 忽略非法释放, 适用于生产环境, 不中止程序, 继续执行, 但不进行后续检查
            return;
        #endif
    }
    if (info->freed) {
        // 重复释放检测
        printf("[WARNING] Double free detected for ptr=%p allocated at %s:%d\n",
               ptr, info->file, info->line);
        #if MODE_DEBUG_ABORT_ON_INVALID_FREE
            abort();
        #else
            return;
        #endif
    }

    // 计算实际分配的原始指针和红区位置
    size_t size = info->size;
    char *raw_ptr = (char *)ptr - REDZONE_SIZE;
    char *end_redzone = (char *)raw_ptr + REDZONE_SIZE + size;

    // 检查红区是否被破坏
    int underflow = 0, overflow = 0;
    for (size_t i = 0; i < REDZONE_SIZE; i++) {
        if (((unsigned char *)raw_ptr)[i] != FILL_REDZONE_PATTERN) {
            underflow = 1;
            break;
        }
    }
    for (size_t i = 0; i < REDZONE_SIZE; i++) {
        if (((unsigned char *)end_redzone)[i] != FILL_REDZONE_PATTERN) {
            overflow = 1;
            break;
        }
    }
    if (underflow) {
        printf("[ERROR] Buffer underflow detected for ptr=%p allocated at %s:%d\n",
               ptr, file, line);
    }
    if (overflow) {
        printf("[ERROR] Buffer overflow detected for ptr=%p allocated at %s:%d\n",
               ptr, file, line);
    }

    // 填充释放模式(UAF检测)
    memset(ptr, FILL_FREE_PATTERN, size);

    printf("[FREE] ptr=%p size=%zu at %s:%d\n", ptr, size, file, line);

    info->freed = 1;

    free(raw_ptr);
}

static void *debug_calloc_impl(size_t nmemb, size_t size, const char *file, int line) {
    size_t total_size = nmemb * size;
    void *ptr = debug_malloc_impl(total_size, file, line);
    if (ptr) {
        memset(ptr, 0, total_size); // 覆盖为0, 因为 calloc 要求返回的内存初始化为0
    }
    return ptr;
}

static void *debug_realloc_impl(void *ptr, size_t size, const char *file, int line) {
    if (!ptr) {
        return debug_malloc_impl(size, file, line);
    }
    if (size == 0) {
        debug_free_impl(ptr, file, line);
        return NULL;
    }

    // 查找旧分配大小
    AllocInfo *old_info = find_alloc_info(ptr);
    if (!old_info) {
        printf("[REALLOC] ptr=%p NOT FOUND ...\n", ptr, file, line);
        return debug_malloc_impl(size, file, line); // 直接分配新块, 视为未跟踪的 realloc
    }
    size_t old_size = old_info->size;
    // 分配新块
    void *new_ptr = debug_malloc_impl(size, file, line);
    if (!new_ptr) {
        return NULL;
    }

    // 复制旧数据
    size_t copy_size = (size < old_size) ? size : old_size;
    memcpy(new_ptr, ptr, copy_size);

    // 释放旧块
    debug_free_impl(ptr, file, line);

    return new_ptr;
}

/* 宏替换系统分配函数 */
#define malloc(size) debug_malloc_impl(size, __FILE__, __LINE__)
#define free(ptr)    debug_free_impl(ptr, __FILE__, __LINE__)
#define calloc(nmemb, size) debug_calloc_impl(nmemb, size, __FILE__, __LINE__)
#define realloc(ptr, size)  debug_realloc_impl(ptr, size, __FILE__, __LINE__)

/* 自动初始化，清理和泄漏报告
__attribute__((constructor)) 是 GCC 扩展，表示这个函数在程序启动时自动执行, 所以无需手动调用，程序一运行就自动初始化
*/
__attribute__((constructor)) static void init_debug_allocator() {
    printf("[DEBUG_ALLOC] TLS-optimized memory debugger initialized.\n");
    atexit(report_leaks);
    register_current_thread();
}

#endif //DEBUG_MEMORY_ENABLED

#endif
