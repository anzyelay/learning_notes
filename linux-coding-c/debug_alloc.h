// 参考文档 dbg-内存异常调试方法
// 包含 TLS 优化的内存调试器
#ifndef DEBUG_ALLOC_TLS_H
#define DEBUG_ALLOC_TLS_H

#ifdef DEBUG_MEMORY_ENABLED

// #define MODE_DEBUG_ABORT_ON_INVALID_FREE    1  // 调试模式：非法释放时中止程序
// #define MODE_PROD_IGNORE_INVALID_FREE     1  // 生产模式：非法释放时忽略，不中止程序
// 注意：两种模式只能启用一种, 否则默认使用第三种方式（填充特殊模式）


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <pthread.h>
#include <dlfcn.h>

#define FILL_ALLOC_PATTERN 0xAA
#define FILL_FREE_PATTERN  0xDD
#define MAX_BACKTRACE_DEPTH 10
#define MAX_STACK_DEPTH 10
#define MAX_THREAD_NAME 32

typedef struct AllocInfo {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    void *stack_addrs[MAX_STACK_DEPTH];
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

/* 删除分配记录 */
static void remove_alloc(void *ptr) {
    AllocInfo **curr = &thread_alloc_list;
    while (*curr) {
        if ((*curr)->ptr == ptr) {
            AllocInfo *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            break;
        }
        curr = &(*curr)->next;
    }
}

/* 查找分配大小 */
static size_t find_alloc_size(void *ptr) {
    AllocInfo *curr = thread_alloc_list;
    while (curr) {
        if (curr->ptr == ptr) {
            size_t size = curr->size;
            return size;
        }
        curr = curr->next;
    }
    return 0;
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

            curr = curr->next;
            thread_leaks++;
        }

        printf("Thread %s has %d leaks.\n", thread_rec->name, thread_leaks);
        total_leaks += thread_leaks;
        thread_rec = thread_rec->next;
    }

    if (total_leaks == 0) {
        printf("No memory leaks detected across all threads.\n");
    } else {
        printf("\nTotal leaks: %d\n", total_leaks);
    }
    pthread_mutex_unlock(&global_list_lock);
}

static void *debug_malloc_impl(size_t size, const char *file, int line) {
    void *ptr = malloc(size);
    if (!ptr) return NULL;
    memset(ptr, FILL_ALLOC_PATTERN, size);
    printf("[ALLOC] ptr=%p size=%zu at %s:%d\n", ptr, size, file, line);
    // print_backtrace();
    add_alloc(ptr, size, file, line);
    return ptr;
}

static void debug_free_impl(void *ptr, const char *file, int line) {
    if (!ptr) return;
    size_t size = find_alloc_size(ptr);
    if (size <= 0) {
        printf("[FREE] ptr=%p NOT FOUND in alloc records at %s:%d\n", ptr, file, line);
        // way 1: 非法释放，直接中止程序, 适用于调试阶段,生产环境慎用
        #if MODE_DEBUG_ABORT_ON_INVALID_FREE
            abort();
        // way 2: 继续释放，但不做任何填充和记录删除, 适用于生产环境
        #elif MODE_PROD_IGNORE_INVALID_FREE
            free(ptr);
            return;
        // way 3: 继续释放，但填充为特殊模式，便于事后分析
        #else
            memset(ptr, FILL_FREE_PATTERN, 16); // 填充前16字节
            free(ptr);
            return;
        #endif
    }

    memset(ptr, FILL_FREE_PATTERN, size);
    printf("[FREE] ptr=%p size=%zu at %s:%d\n", ptr, size, file, line);
    // print_backtrace();
    remove_alloc(ptr);
    free(ptr);
}

static void *debug_calloc_impl(size_t nmemb, size_t size, const char *file, int line) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) return NULL;
    memset(ptr, FILL_ALLOC_PATTERN, nmemb * size);
    printf("[CALLOC] ptr=%p size=%zu at %s:%d\n", ptr, nmemb * size, file, line);
    // print_backtrace();
    add_alloc(ptr, nmemb * size, file, line);
    return ptr;
}

static void *debug_realloc_impl(void *ptr, size_t size, const char *file, int line) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) return NULL;
    memset(new_ptr, FILL_ALLOC_PATTERN, size);
    printf("[REALLOC] old_ptr=%p new_ptr=%p size=%zu at %s:%d\n", ptr, new_ptr, size, file, line);
    // print_backtrace();
    remove_alloc(ptr);
    add_alloc(new_ptr, size, file, line);
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
