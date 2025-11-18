#ifndef DEBUG_ALLOC_H
#define DEBUG_ALLOC_H

// 参考文档 dbg-内存异常调试方法

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <pthread.h>

#define FILL_ALLOC_PATTERN 0xAA
#define FILL_FREE_PATTERN  0xDD
#define MAX_BACKTRACE_DEPTH 10

typedef struct AllocInfo {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    char **stack;
    int stack_depth;
    struct AllocInfo *next;
} AllocInfo;

static AllocInfo *alloc_list = NULL;
static pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;

/* 打印调用栈 */
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
    pthread_mutex_lock(&alloc_lock);
    AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo));
    info->ptr = ptr;
    info->size = size;
    info->file = file;
    info->line = line;

    void *buffer[MAX_BACKTRACE_DEPTH];
    info->stack_depth = backtrace(buffer, MAX_BACKTRACE_DEPTH);
    info->stack = backtrace_symbols(buffer, info->stack_depth);

    info->next = alloc_list;
    alloc_list = info;
    pthread_mutex_unlock(&alloc_lock);
}

/* 删除分配记录 */
static void remove_alloc(void *ptr) {
    pthread_mutex_lock(&alloc_lock);
    AllocInfo **curr = &alloc_list;
    while (*curr) {
        if ((*curr)->ptr == ptr) {
            AllocInfo *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free->stack);
            free(to_free);
            break;
        }
        curr = &(*curr)->next;
    }
    pthread_mutex_unlock(&alloc_lock);
}

/* 查找分配大小 */
static size_t find_alloc_size(void *ptr) {
    pthread_mutex_lock(&alloc_lock);
    AllocInfo *curr = alloc_list;
    while (curr) {
        if (curr->ptr == ptr) {
            size_t size = curr->size;
            pthread_mutex_unlock(&alloc_lock);
            return size;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&alloc_lock);
    return 0;
}

/* 泄漏报告 */
static void report_leaks() {
    pthread_mutex_lock(&alloc_lock);
    printf("\n[LEAK REPORT]\n");
    if (!alloc_list) {
        printf("No memory leaks detected.\n");
    }
    AllocInfo *curr = alloc_list;
    while (curr) {
        printf("Leak: ptr=%p size=%zu at %s:%d\n", curr->ptr, curr->size, curr->file, curr->line);
        for (int i = 0; i < curr->stack_depth; i++) {
            printf("    %s\n", curr->stack[i]);
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&alloc_lock);
}

/* 核心实现 */
static void *debug_malloc_impl(size_t size, const char *file, int line) {
    void *ptr = malloc(size);
    if (!ptr) return NULL;
    memset(ptr, FILL_ALLOC_PATTERN, size);
    printf("[ALLOC] ptr=%p size=%zu at %s:%d\n", ptr, size, file, line);
    print_backtrace();
    add_alloc(ptr, size, file, line);
    return ptr;
}

static void debug_free_impl(void *ptr, const char *file, int line) {
    if (!ptr) return;
    size_t size = find_alloc_size(ptr);
    if (size > 0) {
        memset(ptr, FILL_FREE_PATTERN, size);
    }
    printf("[FREE] ptr=%p size=%zu at %s:%d\n", ptr, size, file, line);
    print_backtrace();
    remove_alloc(ptr);
    free(ptr);
}

static void *debug_calloc_impl(size_t nmemb, size_t size, const char *file, int line) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) return NULL;
    memset(ptr, FILL_ALLOC_PATTERN, nmemb * size);
    printf("[CALLOC] ptr=%p size=%zu at %s:%d\n", ptr, nmemb * size, file, line);
    print_backtrace();
    add_alloc(ptr, nmemb * size, file, line);
    return ptr;
}

static void *debug_realloc_impl(void *ptr, size_t size, const char *file, int line) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) return NULL;
    memset(new_ptr, FILL_ALLOC_PATTERN, size);
    printf("[REALLOC] old_ptr=%p new_ptr=%p size=%zu at %s:%d\n", ptr, new_ptr, size, file, line);
    print_backtrace();
    remove_alloc(ptr);
    add_alloc(new_ptr, size, file, line);
    return new_ptr;
}

/* 宏替换系统分配函数 */
#define malloc(size) debug_malloc_impl(size, __FILE__, __LINE__)
#define free(ptr)    debug_free_impl(ptr, __FILE__, __LINE__)
#define calloc(nmemb, size) debug_calloc_impl(nmemb, size, __FILE__, __LINE__)
#define realloc(ptr, size)  debug_realloc_impl(ptr, size, __FILE__, __LINE__)

/* 自动泄漏报告 */
__attribute__((constructor)) static void init_leak_report() {
    atexit(report_leaks);
}

#endif
