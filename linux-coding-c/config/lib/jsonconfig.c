#include "jsonconfig.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib.h>
#include <poll.h>
#include <stdint.h>

// command and key autocomplete
#include <readline/readline.h>
#include <readline/history.h>

#define CFG_TAG "JsonCfg"

#define _LOG_LEVEL_EMERG          (0x1 << 0)
#define _LOG_LEVEL_ERR            (0x1 << 1)
#define _LOG_LEVEL_WARN           (0x1 << 2)
#define _LOG_LEVEL_INFO           (0x1 << 3)

#define cfg_info(fmt, ...) \
            cfg_printf_impl(_LOG_LEVEL_INFO, __func__, __LINE__, "%s: "fmt, CFG_TAG, ##__VA_ARGS__)
#define cfg_warning(fmt, ...) \
            cfg_printf_impl(_LOG_LEVEL_WARN, __func__, __LINE__, "%s: "fmt, CFG_TAG, ##__VA_ARGS__)
#define cfg_error(fmt, ...) \
            cfg_printf_impl(_LOG_LEVEL_ERR, __func__, __LINE__, "%s: "fmt, CFG_TAG, ##__VA_ARGS__)

static log_hook_t g_log_hook = NULL;
void cfg_set_log_hook(log_hook_t log_handle)
{
    g_log_hook = log_handle;
}
static void cfg_printf_impl(unsigned int level, const char *func, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (g_log_hook) {
        g_log_hook(level, func, line, fmt, ap);
    } else {
        // g_logv(0, level<<2, fmt, ap);
        g_print("[%s:%d] ", func, line);
        vprintf(fmt, ap);
    }

    va_end(ap);
}

#define FOREACH_CFG_ITEM(it) \
    for (cfg_item_t *it = cfg_list; it; it = it->next)

typedef enum {
    CFG_TASK_CHANGE,
    CFG_TASK_QUIT
} cfg_change_type_t;
typedef struct {
    cfg_change_type_t type;
    cfg_item_t *item;

    union {
        gint64 i;
        gboolean b;
        double d;
        char *s;
    } old_value;

    union {
        gint64 i;
        gboolean b;
        double d;
        char *s;
    } new_value;
} cfg_change_task_t;

/* audit */
typedef struct {
    char *key;
    char *old_value;
    char *new_value;
    time_t ts;
} cfg_audit_entry_t;

/* 输出模式（预留扩展） */
typedef enum {
    CFG_FMT_PLAIN = 0,   /* 普通 CLI / 日志 */
    CFG_FMT_DEBUG,       /* debug / verbose */
    CFG_FMT_MASKED       /* 敏感信息（未来用） */
} cfg_format_t;

/* command line help */
typedef struct {
    const char *cmd;
    const char *args;
    const char *desc;
} cli_cmd_desc_t;

static const cli_cmd_desc_t g_cli_cmds[] = {
    { "get",        "<key>",            "Get config value" },
    { "set",        "<key> <value>",    "Set config value (runtime)" },
    { "status",     "",                 "Show system status" },
    { "history",    "",                 "Show config change history" },
    { "save",       "",                 "Save config to file" },
    { "save_as",     "<file>",          "Save config to specified file" },
    { "show_json",  "",                 "Show all config values with json style" },
    { "show",       "",                 "Show all config values with plain text" },
    { "help",       "[cmd]",            "Show help for special command or all commands" },
    { "man",        "item",             "Show the description for item" },
    { "quit",       "",                 "Exit CLI" },
    { NULL, NULL, NULL }
};

typedef void (*cfg_iter_fn)(cfg_item_t *it, void *user_data);

/* 1. 从“原始值副本” stringify（事务 / rollback / audit） */
static char *cfg_raw_to_string(cfg_type_t type,
                         const void *value,
                         cfg_format_t fmt);

/* ---------- globals ---------- */

static cfg_item_t *cfg_list = NULL;
static GHashTable *cfg_items_map;      // 新增哈希索引
static GRWLock cfg_lock;
static GAsyncQueue *cfg_change_queue;
static GThread *cfg_change_thread = NULL;
static gsize cfg_initialized = 0;

static gboolean g_needs_restart = FALSE;
static GPtrArray *g_restart_reasons = NULL;   // cfg_item_t*

static GQueue cfg_audit_log;   // 有上限，比如 1000
gboolean is_changed_from_last_save = TRUE;

static char *cfg_file_path = NULL;

#include <sys/socket.h>
#include <sys/un.h>
#define SOCK_PATH_FORMAT "/tmp/%s.socket"

typedef struct {
    int fd;
} client_info_t;

static GThread *cfg_cli_server_thread = NULL;
static int listen_fd = -1;
static char g_sockpath_name[108];
static volatile gboolean cfg_cli_listening_running = TRUE;
static void cfg_set_sockpath(const char *name)
{
    snprintf(g_sockpath_name+1, sizeof(g_sockpath_name)-1, SOCK_PATH_FORMAT, name);
    g_sockpath_name[0] = '\0'; // abstract socket
}
static const char *get_socket_path(void)
{
    return g_sockpath_name;
}

static int connect_daemon()
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", get_socket_path());

    if (connect(fd,
                (struct sockaddr *)&addr,
                sizeof(addr)) < 0) {
        cfg_warning("failed to connect\n");
        close(fd);
        return -1;
    }
    return fd;
}

static int create_listen_socket(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        cfg_warning("failed to create socket\n");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", get_socket_path());

    unlink(get_socket_path()); // remove old socket if exists

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        cfg_warning("failed to bind\n");
        close(fd);
        return -1;
    }

    chmod(addr.sun_path, 0660);

    if (listen(fd, 5) < 0) {
        cfg_warning("failed to listen\n");
        close(fd);
        return -1;
    }
    return fd;
}

/* log audit */
static void cfg_audit_record(cfg_item_t *it, const void *oldv, const void *newv)
{
    cfg_audit_entry_t *e = g_new0(cfg_audit_entry_t, 1);
    e->key = g_strdup(it->key);
    e->old_value = cfg_raw_to_string(it->type, oldv, CFG_FMT_PLAIN);
    e->new_value = cfg_raw_to_string(it->type, newv, CFG_FMT_PLAIN);
    e->ts = time(NULL);

    if (g_queue_get_length(&cfg_audit_log) == 1000) {
        cfg_audit_entry_t *old = g_queue_pop_head(&cfg_audit_log);
        g_free(old->key);
        g_free(old->old_value);
        g_free(old->new_value);
        g_free(old);
    }
    g_queue_push_tail(&cfg_audit_log, e);
    is_changed_from_last_save = TRUE;
}

static void cfg_audit_entry_print(cfg_audit_entry_t *entry, void *user_data)
{
    FILE *out = (FILE *)user_data;
    gchar log_time[64];
    strftime(log_time, sizeof(log_time), "%Y-%m-%d %H:%M:%S", localtime(&entry->ts));
    fprintf(out, "[%s] %s: %s -> %s\n", log_time, entry->key, entry->old_value, entry->new_value);
}

#ifdef CFG_DEBUG
static inline size_t cfg_expected_size_for_int_type(cfg_type_t type)
{
    switch (type) {
    case CFG_INT8:  return sizeof(gint8);
    case CFG_INT16: return sizeof(gint16);
    case CFG_INT32: return sizeof(gint32);
    case CFG_INT64: return sizeof(gint64);
    default:        return 0;
    }
}
static inline size_t cfg_expected_size_for_double_type(cfg_type_t type)
{
    switch (type) {
    case CFG_FLOAT:     return sizeof(float);
    case CFG_DOUBLE:    return sizeof(double);
    default:            return 0;
    }
}
#endif

static int cfg_item_set_int64(cfg_item_t *item, gint64 v)
{
    if (!item || !item->storage) {
        cfg_warning("Invalid item or storage in cfg_item_set_int64\n");
        return -1;
    }

#ifdef CFG_DEBUG
    /* defensive invariant check */
    size_t expected = cfg_expected_size_for_int_type(item->type);
    if (expected == 0) {
        cfg_warning("cfg_item_set_int64: invalid type %d\n", item->type);
        return -EINVAL;
    }
    if (item->storage_size < expected) {
        cfg_warning("cfg_item_set_int64: storage_size %zu < expected %zu (type=%d) for %s\n",
                item->storage_size, expected, item->type, item->key);
        return -EINVAL;
    }
#endif

    switch (item->type) {
    case CFG_INT8: {
        if (v < INT8_MIN || v > INT8_MAX) {
            cfg_warning("%s: Value out of range for CFG_INT8: %ld\n", item->key, v);
            return -1;
        }
        gint8 tmp = (gint8)v;
        memcpy(item->storage, &tmp, sizeof(tmp));
        break;
    }
    case CFG_INT16: {
        if (v < INT16_MIN || v > INT16_MAX) {
            cfg_warning("%s: Value out of range for CFG_INT16: %ld\n", item->key, v);
            return -1;
        }
        gint16 tmp = (gint16)v;
        memcpy(item->storage, &tmp, sizeof(tmp));
        break;
    }
    case CFG_INT32: {
        if (v < INT32_MIN || v > INT32_MAX) {
            cfg_warning("%s: Value out of range for CFG_INT32: %ld\n", item->key, v);
            return -1;
        }
        gint32 tmp = (gint32)v;
        memcpy(item->storage, &tmp, sizeof(tmp));
        break;
    }
    case CFG_INT64: {
        if (v < INT64_MIN || v > INT64_MAX) {
            cfg_warning("%s: Value out of range for CFG_INT64: %ld\n", item->key, v);
            return -1;
        }
        gint64 tmp = (gint64)v;
        memcpy(item->storage, &tmp, sizeof(tmp));
        break;
    }
    default:
        cfg_warning("%s: Unsupported type for int storage: %d\n", item->key, item->type);
        return -1;
    }
    return 0;
}

static int cfg_item_get_int64(const cfg_item_t *item, gint64 *out)
{
    if (!item || !out || item->storage == NULL) {
        cfg_warning("Invalid arguments to cfg_item_get_int64\n");
        return -1;
    }

#ifdef CFG_DEBUG
    /* defensive invariant check */
    size_t expected = cfg_expected_size_for_int_type(item->type);
    if (expected == 0) {
        cfg_warning("cfg_item_get_int64: invalid type %d for %s\n", item->type, item->key);
        return -EINVAL;
    }
    if (item->storage_size < expected) {
        cfg_warning("cfg_item_get_int64: storage_size %zu < expected %zu (type=%d) for %s\n",
                item->storage_size, expected, item->type, item->key);
        return -EINVAL;
    }
#endif

    *out = 0;
    switch (item->type) {
    case CFG_INT8: {
        gint8 v = 0;
        memcpy(&v, item->storage, sizeof(v));
        *out = (gint64)v;
        break;
    }
    case CFG_INT16: {
        gint16 v = 0;
        memcpy(&v, item->storage, sizeof(v));
        *out = (gint64)v;
        break;
    }
    case CFG_INT32: {
        gint32 v = 0;
        memcpy(&v, item->storage, sizeof(v));
        *out = (gint64)v;
        break;
    }
    case CFG_INT64: {
        gint64 v = 0;
        memcpy(&v, item->storage, sizeof(v));
        *out = v;
        break;
    }
    default:
        cfg_warning("%s: Unsupported type for int value: %d\n", item->key, item->type);
        return -1;
    }
    return 0;
}

static int cfg_item_set_bool(const cfg_item_t *item, gboolean v)
{
    if (!item || item->type != CFG_BOOL || item->storage == NULL) {
        cfg_warning("Invalid item for cfg_item_set_bool\n");
        return -1;
    }

#ifdef CFG_DEBUG
    if (item->storage_size < sizeof(gboolean)) {
        cfg_warning("cfg_item_set_bool: storage_size %zu < expected %zu for %s\n"
            , item->storage_size, sizeof(gboolean), item->key);
        return -EINVAL;
    }
#endif

    memcpy(item->storage, &v, sizeof(v));
    return 0;
}

static int cfg_item_get_bool(const cfg_item_t *item, gboolean *out)
{
    if (!item || !out || item->type != CFG_BOOL || item->storage == NULL) {
        cfg_warning("Invalid arguments to cfg_item_get_bool\n");
        return -1;
    }
#ifdef CFG_DEBUG
    if (item->storage_size < sizeof(gboolean)) {
        cfg_warning("cfg_item_get_bool: storage_size %zu < expected %zu for %s\n"
            , item->storage_size, sizeof(gboolean), item->key);
        return -EINVAL;
    }
#endif
    memcpy(out, item->storage, sizeof(gboolean));
    return 0;
}

static int cfg_item_set_double(cfg_item_t *item, double v)
{
    if (!item || !item->storage) {
        cfg_warning("Invalid item for cfg_item_set_double\n");
        return -1;
    }

#ifdef CFG_DEBUG
    size_t expected = cfg_expected_size_for_double_type(item->type);
    if (expected == 0) {
        cfg_warning("cfg_item_set_double: invalid type %d for %s\n", item->type, item->key);
        return -EINVAL;
    }
    if (item->storage_size < expected) {
        cfg_warning("cfg_item_set_double: storage_size %zu < expected %zu (type=%d) for %s\n",
                item->storage_size, expected, item->type, item->key);
        return -EINVAL;
    }
#endif
    switch (item->type) {
    case CFG_FLOAT: {
        if (v < -FLT_MAX || v > FLT_MAX) {
            cfg_warning("%s: Value out of range for CFG_FLOAT: %f\n", item->key, v);
            return -1;
        }
        float tmp = (float)v;
        memcpy(item->storage, &tmp, sizeof(tmp));
        break;
    }
    case CFG_DOUBLE: {
        if (v < -DBL_MAX || v > DBL_MAX) {
            cfg_warning("%s: Value out of range for CFG_DOUBLE: %f\n", item->key, v);
            return -1;
        }
        memcpy(item->storage, &v, sizeof(v));
        break;
    }
    default:
        cfg_warning("%s: Unsupported type for double storage: %d\n", item->key, item->type);
        return -1;
    }
    return 0;
}
static int cfg_item_get_double(const cfg_item_t *item, double *out)
{
    if (!item || !item->storage || !out) {
        cfg_warning("Invalid arguments to cfg_item_get_double\n");
        return -1;
    }

#ifdef CFG_DEBUG
    size_t expected = cfg_expected_size_for_double_type(item->type);
    if (expected == 0) {
        cfg_warning("cfg_item_get_double: invalid type %d for %s\n", item->type, item->key);
        return -EINVAL;
    }
    if (item->storage_size < expected) {
        cfg_warning("cfg_item_get_double: storage_size %zu < expected %zu (type=%d) for %s\n",
                item->storage_size, expected, item->type, item->key);
        return -EINVAL;
    }
#endif
    *out = 0;
    switch (item->type) {
    case CFG_FLOAT: {
        float v = 0;
        memcpy(&v, item->storage, sizeof(v));
        *out = (double)v;
        break;
    }
    case CFG_DOUBLE: {
        double v = 0;
        memcpy(&v, item->storage, sizeof(v));
        *out = v;
        break;
    }
    default:
        cfg_warning("%s: Unsupported type for double storage: %d\n", item->key, item->type);
        return -1;
    }
    return 0;
}

/**
 * @brief Copy a value of a specific type from src to dst, with optional deep copy for strings.
 *
 * @param type
 * @param dst
 * @param src
 * @param need_free : if TRUE and the type is CFG_STRING, it will free the existing string in dst before copying. If FALSE, it will not free dst (used for read operations where dst is an output parameter).
 * @return int
 * NOTE: YOU MUST ENSURE THAT dst AND src POINT TO VALID STORAGE, AND THAT THE TYPE MATCHES THE CONFIG ITEM'S TYPE. THIS FUNCTION DOES NOT PERFORM ANY TYPE CHECKING OR VALIDATION.
 */
static int cfg_copy_value(cfg_type_t type,
                           void *dst,
                           const void *src,
                           gboolean need_free)
{
    switch (type) {
    case CFG_INT8:
        *(gint8 *)dst = *(const gint8 *)src;
        break;
    case CFG_INT16:
        *(gint16 *)dst = *(const gint16 *)src;
        break;
    case CFG_INT32:
        *(gint32 *)dst = *(const gint32 *)src;
        break;
    case CFG_INT64:
        *(gint64 *)dst = *(const gint64 *)src;
        break;
    case CFG_BOOL:
        *(gboolean *)dst = *(const gboolean *)src;
        break;
    case CFG_FLOAT:
        *(float *)dst = *(const float *)src;
        break;
    case CFG_DOUBLE:
        *(double *)dst = *(const double *)src;
        break;
    case CFG_STRING:
        if (need_free && *(char **)dst)
            g_free(*(char **)dst);
        *(char **)dst = g_strdup(*(char * const *)src);
        break;
    default:
        cfg_warning("Unsupported config type: %d\n", type);
        return -1;
    }
    return 0;
}

/* worker thread */
static gpointer cfg_change_worker(gpointer data)
{
    (void)data; // unused
    while (1) {
        cfg_change_task_t *task =
            g_async_queue_pop(cfg_change_queue);

        // cfg_info("Worker thread processing task for item: %s\n", task->item ? task->item->key : "NULL");

        if (!task)
            continue;

        if (task->type == CFG_TASK_QUIT) {
            g_free(task);
            break;
        }

        cfg_item_t *it = task->item;

        /* 执行 on_change（不持有锁） */
        int ret = 0;
        if (it->on_change)
            ret = it->on_change(&task->old_value,
                                &task->new_value);

        if (ret != 0) {
            /* rollback */
            g_rw_lock_writer_lock(&cfg_lock);
            cfg_copy_value(it->type, it->storage, &task->old_value, TRUE);
            g_rw_lock_writer_unlock(&cfg_lock);

            char *old_str = cfg_raw_to_string(it->type, &task->old_value, CFG_FMT_PLAIN);
            char *new_str = cfg_raw_to_string(it->type, &task->new_value, CFG_FMT_PLAIN);
            cfg_info(
              "rollback: %s %s -> %s\n", it->key, new_str, old_str);
            g_free(old_str);
            g_free(new_str);
        }
        else {
            cfg_audit_record(it, &task->old_value, &task->new_value);
        }

        /* cleanup */
        if (it->type == CFG_STRING) {
            g_free(task->old_value.s);
            g_free(task->new_value.s);
        }
        g_free(task);
    }
    return NULL;
}

static void cfg_change_worker_run(void)
{
    cfg_change_queue = g_async_queue_new();
    cfg_change_thread =
        g_thread_new("cfg-worker",
                     cfg_change_worker,
                     NULL);
}

/* Called during daemon shutdown; hook later */
__attribute__((unused))
static void cfg_change_worker_stop(void)
{
    if (cfg_change_thread) {
        cfg_change_task_t *quit_task = g_new0(cfg_change_task_t, 1);
        quit_task->type = CFG_TASK_QUIT;
        g_async_queue_push(cfg_change_queue, quit_task);
        g_thread_join(cfg_change_thread);
        cfg_change_thread = NULL;
    }
    if (cfg_change_queue) {
        g_async_queue_unref(cfg_change_queue);
        cfg_change_queue = NULL;
    }
}

/* enqueue config change */
static void cfg_enqueue_change(cfg_item_t *it,
                               const void *oldv,
                               const void *newv)
{
    cfg_change_task_t *task = g_new0(cfg_change_task_t, 1);
    task->item = it;
    task->type = CFG_TASK_CHANGE;

    cfg_copy_value(it->type, &task->old_value, oldv, FALSE);
    cfg_copy_value(it->type, &task->new_value, newv, FALSE);

    g_async_queue_push(cfg_change_queue, task);
}

/* ---------- type checking and normalization ---------- */
static int cfg_type_check_normalize(cfg_item_t *item)
{
    struct type_size_map {
        int size;
        char *type_name;
    } type_size_maps[CFG_TYPE_NUMBER] = {
        [CFG_BOOL] = { sizeof(gboolean), "CFG_BOOL" },
        [CFG_INT8] = { sizeof(gint8), "CFG_INT8" },
        [CFG_INT16] = { sizeof(gint16), "CFG_INT16" },
        [CFG_INT32] = { sizeof(gint32), "CFG_INT32" },
        [CFG_INT64] = { sizeof(gint64), "CFG_INT64" },
        [CFG_FLOAT] = { sizeof(float), "CFG_FLOAT" },
        [CFG_DOUBLE] = { sizeof(double), "CFG_DOUBLE" },
        [CFG_STRING] = { sizeof(char *), "CFG_STRING" }
    };

    /** make sure the storage size matches the type with the maps to normalize*/
    if (item->storage_size == 0) {
        cfg_warning("Storage size must be specified for item '%s'\n", item->key);
        return -1;
    }
    if (!item->storage) {
        cfg_warning("Storage pointer must be specified for item '%s'\n", item->key);
        return -1;
    }
    if (item->type >= CFG_TYPE_NUMBER) {
        cfg_warning("Invalid config type for item '%s': %d\n", item->key, item->type);
        return -1;
    }

    /** normalize the type based on storage size */
    if (item->type == CFG_INT_AUTO) {
        if (item->storage_size == sizeof(gint8)) {
            item->type = CFG_INT8;
        }
        else if (item->storage_size == sizeof(gint16)) {
            item->type = CFG_INT16;
        }
        else if (item->storage_size == sizeof(gint32)) {
            item->type = CFG_INT32;
        }
        else if (item->storage_size == sizeof(gint64)) {
            item->type = CFG_INT64;
        }
        else {
            cfg_warning("Invalid storage size for CFG_INT_AUTO item '%s': %d\n",
                       item->key, item->storage_size);
            return -1;
        }
    }

    if (item->type == CFG_DOUBLE) {
        if (item->storage_size == sizeof(double)) {
            item->type = CFG_DOUBLE;
        }
        else if (item->storage_size == sizeof(float)) {
            item->type = CFG_FLOAT;
        }
        else {
            cfg_warning("Invalid storage size for CFG_DOUBLE item '%s': %d\n",
                       item->key, item->storage_size);
            return -1;
        }
    }

    /** check storage size against normalized type */
    if (item->storage_size != type_size_maps[item->type].size) {
        cfg_warning("Storage size %d does not match expected size %d for type %s for item '%s'\n",
                   item->storage_size, type_size_maps[item->type].size
                   , type_size_maps[item->type].type_name, item->key);
        return -1;
    }

    if (item->type == CFG_STRING && item->storage && *(char **)item->storage) {
        /* ensure string storage is initialized to NULL as storage must always hold heap-allocated string*/
        cfg_info("CFG_STRING item '%s' has non-NULL initial storage which may cause issues."
             "It should be initialized to NULL or a valid heap-allocated string.\n"
             , item->key);
        // return -1;

        // Note: if the initial value is important, it should be set in code after registration, rather than relying on non-NULL initial storage.
        // This is a workaround to allow registration of items with non-NULL initial storage, but it is recommended to initialize string storage to NULL before registration to avoid potential issues.
        // The potential ISSUES include:
        // 1. If the initial string is a string literal or statically allocated, the g_free() call in cfg_from_json_string will cause a crash.
        // 2. If the initial string is heap-allocated but not duplicated, it may lead to double free or memory corruption if the same string pointer is shared among multiple items or used elsewhere in the code.
        // By setting it to NULL first and then strdup the original value, we ensure that the storage holds a heap-allocated string as expected, and avoid potential crashes or memory issues. However, it is still recommended to initialize string storage to NULL before registration to avoid unnecessary overhead and potential confusion.
        *(char **)item->storage = g_strdup(*(char **)item->storage);
    }


    return 0;
}

/* ---------- registry ---------- */

int cfg_register(cfg_item_t *item)
{
    FOREACH_CFG_ITEM(it) {
        if (g_strcmp0(it->key, item->key) == 0) {
            cfg_info("Duplicate config key: %s\n", item->key);
            return -1;
        }
    }

    if (cfg_type_check_normalize(item) != 0) {
        return -1;
    }

    item->next = cfg_list;
    cfg_list = item;

    // 同步更新哈希索引
    g_hash_table_insert( cfg_items_map, (gpointer)item->key, item);

    return 0;
}

static inline cfg_item_t *cfg_find(const char *key)
{
    return g_hash_table_lookup(cfg_items_map, key);
}

/* ---------- json helpers ---------- */

static JsonNode *json_get_by_path(JsonNode *root, const char *path)
{
    JsonNode *node = root;
    JsonObject *obj;
    char **tokens = g_strsplit(path, ".", -1);

    for (int i = 0; tokens[i]; i++) {
        if (!JSON_NODE_HOLDS_OBJECT(node)) {
            node = NULL;
            break;
        }
        obj = json_node_get_object(node);
        node = json_object_get_member(obj, tokens[i]);
        if (!node)
            break;
    }

    g_strfreev(tokens);
    return node;
}

static JsonObject *json_get_or_create(JsonObject *root,
                                      const char *path,
                                      char **leaf)
{
    char **tokens = g_strsplit(path, ".", -1);
    JsonObject *obj = root;

    for (int i = 0; tokens[i + 1]; i++) {
        if (!json_object_has_member(obj, tokens[i])) {
            JsonObject *n = json_object_new();
            json_object_set_object_member(obj, tokens[i], n);
        }
        obj = json_object_get_object_member(obj, tokens[i]);
    }

    *leaf = g_strdup(tokens[g_strv_length(tokens) - 1]);
    g_strfreev(tokens);
    return obj;
}

static GType cfg_type_to_gtype(cfg_type_t type)
{
    switch (type) {
    case CFG_INT8:
    case CFG_INT16:
    case CFG_INT32:
    case CFG_INT64:  return G_TYPE_INT64;
    case CFG_BOOL:   return G_TYPE_BOOLEAN;
    case CFG_FLOAT:
    case CFG_DOUBLE: return G_TYPE_DOUBLE;
    case CFG_STRING: return G_TYPE_STRING;
    default:         return G_TYPE_INVALID;
    }
}

static gboolean cfg_value_equal(cfg_type_t type, const void *a, const void *b)
{
    switch (type) {
    case CFG_INT8:
        return *(gint8 *)a == *(gint8 *)b;
    case CFG_INT16:
        return *(gint16 *)a == *(gint16 *)b;
    case CFG_INT32:
        return *(gint32 *)a == *(gint32 *)b;
    case CFG_INT64:
        return *(gint64 *)a == *(gint64 *)b;

    case CFG_BOOL:
        return *(gboolean *)a == *(gboolean *)b;

    case CFG_FLOAT:
        return *(float *)a == *(float *)b;
    case CFG_DOUBLE:
        return *(double *)a == *(double *)b;

    case CFG_STRING: {
        const char *sa = *(char **)a;
        const char *sb = *(char **)b;
        if (sa == sb)      /* same pointer or both NULL */
            return TRUE;
        if (!sa || !sb)
            return FALSE;
        return strcmp(sa, sb) == 0;
    }

    default:
        return FALSE;
    }
}

/**
 * @brief
 *
 * @param it
 * @param node
 * @return int
 */
static int cfg_commit_value(cfg_item_t *it, JsonNode *node)
{
    int ret = 0;

    GType expect = cfg_type_to_gtype(it->type);
    GType actual = json_node_get_value_type(node);

    if (actual != expect) {
        cfg_warning("Not expected type for item %s, return -EINVAL\n", it->key);
        return -EINVAL;
    }

    /* backup */
    union {
        gint64 i;
        gboolean b;
        double d;
        char *s;
    } old = {0};

    cfg_copy_value(it->type, &old, it->storage, FALSE);

    /* apply */
    if (it->from_json(node, it) != 0) {
        ret = -1;
        goto rollback;
    }

    /* check whether the value has changed */
    if (cfg_value_equal(it->type, &old, it->storage)) {
        ret = 0; // OK, no changed
        goto rollback;
    }

    /* validate */
    if (it->validator && it->validator(it->storage) != 0) {
        ret = -2;
        goto rollback;
    }

    /* on_change / audit */
    if (it->on_change)
        cfg_enqueue_change(it, &old, it->storage);
    else
        cfg_audit_record(it, &old, it->storage);

    /* restart */
    if (it->flags & CFG_FLAG_RESTART) {
        g_needs_restart = TRUE;
        g_ptr_array_add(g_restart_reasons, it);
    }

    if (it->type == CFG_STRING)
        g_free(old.s);

    return ret;

rollback:
    cfg_copy_value(it->type, it->storage, &old, TRUE);
    if (it->type == CFG_STRING)
        g_free(old.s);

    return ret;
}

/**
 * @brief  typed / file enterpoint
 *
 */
static int cfg_commit_node_by_key(const char *key, JsonNode *node)
{
    int ret = 0;

    g_rw_lock_writer_lock(&cfg_lock);

    cfg_item_t *it = cfg_find(key);
    if (!it || !(it->flags & CFG_FLAG_RUNTIME)) {
        cfg_warning("%s: %s\n", key, it ? "Not support change in RUNTIME" : "Unregistered key");
        ret = -1;
        goto out;
    }

    ret = cfg_commit_value(it, node);

out:
    g_rw_lock_writer_unlock(&cfg_lock);
    return ret;
}
/* ---------- type handlers ---------- */

int cfg_from_json_int(JsonNode *n, cfg_item_t *p)
{
    gint64 v = json_node_get_int(n);
    return cfg_item_set_int64(p, v);
}

int cfg_to_json_int(const cfg_item_t *p, JsonNode *n)
{
    gint64 v;
    if (cfg_item_get_int64(p, &v) == 0) {
        json_node_set_int(n, v);
        return 0;
    }
    return -1;
}

int cfg_from_json_bool(JsonNode *n, cfg_item_t *p)
{
    gboolean v = json_node_get_boolean(n);
    return cfg_item_set_bool(p, v);
}

int cfg_to_json_bool(const cfg_item_t *p, JsonNode *n)
{
    gboolean v;
    if (cfg_item_get_bool(p, &v) == 0) {
        json_node_set_boolean(n, v);
        return 0;
    }
    return -1;
}

int cfg_from_json_double(JsonNode *n, cfg_item_t *p)
{
    double d = json_node_get_double(n);
    return cfg_item_set_double(p, d);
}

int cfg_to_json_double(const cfg_item_t *p, JsonNode *n)
{
    double v;
    if (cfg_item_get_double(p, &v) == 0) {
        json_node_set_double(n, v);
        return 0;
    }
    return -1;
}

int cfg_from_json_string(JsonNode *n, cfg_item_t *p)
{

    if (!n || !p)
        return -EINVAL;

    cfg_item_t *it = p;

    /* JSON null => string = NULL */
    if (json_node_get_node_type(n) == JSON_NODE_NULL) {
        return 0;
    }

    /* must be a string value */
    if (json_node_get_node_type(n) != JSON_NODE_VALUE ||
        json_node_get_value_type(n) != G_TYPE_STRING) {
        return -EINVAL;
    }

    const char *s = json_node_get_string(n);
    if (s) {
        /* defensive: free old value first */
        return cfg_copy_value(it->type, it->storage, &s, TRUE);
    }

    return -1;
}

int cfg_to_json_string(const cfg_item_t *p, JsonNode *n)
{
    const cfg_item_t *it = p;
    char *dst = *(char **)it->storage;
    if (dst) {
        json_node_set_string(n, dst);
    }
    else {
        json_node_init_null(n);
    }
    return 0;
}

/* ---------- typed GET (非 CLI) ---------- */

#define CFG_GET_COMMON(_type_) \
    g_rw_lock_reader_lock(&cfg_lock);             \
    cfg_item_t *it = cfg_find(key);               \
    if (!it || it->type != _type_) {                \
        g_rw_lock_reader_unlock(&cfg_lock);       \
        return -1;                                \
    }

int cfg_read_int8(const char *key, gint8 *out)
{
    CFG_GET_COMMON(CFG_INT8)
    gint64 v;
    if (cfg_item_get_int64(it, &v) == 0) {
        *out = (gint8)v;
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_int16(const char *key, gint16 *out)
{
    CFG_GET_COMMON(CFG_INT16)
    gint64 v;
    if (cfg_item_get_int64(it, &v) == 0) {
        *out = (gint16)v;
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_int32(const char *key, gint32 *out)
{
    CFG_GET_COMMON(CFG_INT32)
    gint64 v;
    if (cfg_item_get_int64(it, &v) == 0) {
        *out = (gint32)v;
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_int64(const char *key, gint64 *out)
{
    CFG_GET_COMMON(CFG_INT64)
    if (cfg_item_get_int64(it, out) == 0) {
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_int(const char *key, gint64 *out)
{
    g_rw_lock_reader_lock(&cfg_lock);
    cfg_item_t *it = cfg_find(key);
    if (!it || cfg_type_to_gtype(it->type) != G_TYPE_INT64) {
        g_rw_lock_reader_unlock(&cfg_lock);
        cfg_warning("%s: %s\n", key, it ? "Not an integer type" : "Not found");
        return -1;
    }
    if (cfg_item_get_int64(it, out) == 0) {
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_bool(const char *key, gboolean *out)
{
    CFG_GET_COMMON(CFG_BOOL)
    if (cfg_item_get_bool(it, out) == 0) {
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_float(const char *key, float *out)
{
    CFG_GET_COMMON(CFG_FLOAT)
    double v;
    if (cfg_item_get_double(it, &v) == 0) {
        *out = (float)v;
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

int cfg_read_double(const char *key, double *out)
{
    CFG_GET_COMMON(CFG_DOUBLE)
    if (cfg_item_get_double(it, out) == 0) {
        g_rw_lock_reader_unlock(&cfg_lock);
        return 0;
    }
    g_rw_lock_reader_unlock(&cfg_lock);
    return -1;
}

/**
 * @brief Read a string configuration value
 *
 * @param key : config key
 * @param out : output pointer to receive the string value; the caller is responsible for freeing the string with g_free() when no longer needed
 * @return * int : 0 on success, -1 if key not found or type mismatch, -2 on other errors
 */
int cfg_read_string(const char *key, const char **out)
{
    CFG_GET_COMMON(CFG_STRING)
    *out = g_strdup(*(char **)it->storage);
    g_rw_lock_reader_unlock(&cfg_lock);
    return 0;
}

/* ---------- typed SET (非 CLI) ---------- */

int cfg_commit_int(const char *key, gint64 value)
{
    JsonNode *node = json_node_new(JSON_NODE_VALUE);
    json_node_set_int(node, value);

    int ret = cfg_commit_node_by_key(key, node);

    json_node_free(node);
    return ret;
}

int cfg_commit_bool(const char *key, gboolean value)
{
    JsonNode *node = json_node_new(JSON_NODE_VALUE);
    json_node_set_boolean(node, value);

    int ret = cfg_commit_node_by_key(key, node);

    json_node_free(node);
    return ret;
}

int cfg_commit_double(const char *key, double value)
{
    JsonNode *node = json_node_new(JSON_NODE_VALUE);
    json_node_set_double(node, value);

    int ret = cfg_commit_node_by_key(key, node);

    json_node_free(node);
    return ret;
}

int cfg_commit_string(const char *key, const char *value)
{
    JsonNode *node = json_node_new(JSON_NODE_VALUE);
    json_node_set_string(node, value);

    int ret = cfg_commit_node_by_key(key, node);

    json_node_free(node);
    return ret;
}

/* ---------- CLI SET ---------- */

static JsonNode *cfg_parse_string_to_node(cfg_item_t *it,
                               const char *value,
                               int *err)
{
    JsonNode *node = json_node_new(JSON_NODE_VALUE);

    switch (it->type) {
    case CFG_INT8:
    case CFG_INT16:
    case CFG_INT32:
    case CFG_INT64: {
        char *end;
        long v = strtol(value, &end, 0);
        if (*end) goto fail;
        json_node_set_int(node, v);
        break;
    }
    case CFG_BOOL: {
        gboolean b =
            !g_ascii_strcasecmp(value, "true") ||
            !g_ascii_strcasecmp(value, "yes") ||
            !strcmp(value, "1");
        json_node_set_boolean(node, b);
        break;
    }
    case CFG_FLOAT:
    case CFG_DOUBLE: {
        char *end;
        double d = g_ascii_strtod(value, &end);
        if (*end) goto fail;
        json_node_set_double(node, d);
        break;
    }
    case CFG_STRING:
        json_node_set_string(node, value);
        break;
    default:
        cfg_warning("%s: Unknown cfg_type: %d", it->key, it->type);
        goto fail;
    }

    return node;

fail:
    json_node_free(node);
    if (err) *err = -EINVAL;
    return NULL;
}

/* ---------- CLI set ---------- */
int cfg_cli_commit(const char *key, const char *value)
{
    int ret = 0;

    g_rw_lock_writer_lock(&cfg_lock);

    cfg_item_t *it = cfg_find(key);
    if (!it || !(it->flags & CFG_FLAG_RUNTIME)) {
        cfg_warning("%s: %s\n", key, it ? "Not support change in RUNTIME" : "Unregistered key");
        ret = -1;
        goto out;
    }

    JsonNode *node = cfg_parse_string_to_node(it, value, &ret);
    // JsonNode *node = json_from_string(value, NULL);
    if (!node)
        goto out;

    ret = cfg_commit_value(it, node);
    json_node_free(node);

out:
    g_rw_lock_writer_unlock(&cfg_lock);
    return ret;
}

static char *cfg_format_value(cfg_type_t type,
                              const void *v,
                              cfg_format_t fmt)
{
    char buf[256];

    switch (type) {
    case CFG_INT8:
        g_snprintf(buf, sizeof(buf),
                   "%d", *(const gint8 *)v);
        break;
    case CFG_INT16:
        g_snprintf(buf, sizeof(buf),
                   "%d", *(const gint16 *)v);
        break;
    case CFG_INT32:
        g_snprintf(buf, sizeof(buf),
                   "%d", *(const gint32 *)v);
        break;
    case CFG_INT64:
        g_snprintf(buf, sizeof(buf),
                   "%ld", *(const gint64 *)v);
        break;

    case CFG_BOOL:
        g_snprintf(buf, sizeof(buf),
                   "%s",
                   *(const gboolean *)v
                       ? "true" : "false");
        break;
    case CFG_FLOAT:
         g_snprintf(buf, sizeof(buf),
                   "%f", *(const float *)v);
        break;
    case CFG_DOUBLE:
        g_snprintf(buf, sizeof(buf),
                   "%g", *(const double *)v);
        break;

    case CFG_STRING: {
        const char *s = *(char * const *)v;
        if (!s) s = "";

        /* 预留 masking */
        if (fmt == CFG_FMT_MASKED) {
            g_snprintf(buf, sizeof(buf), "******");
        } else {
            g_snprintf(buf, sizeof(buf), "%s", s);
        }
        break;
    }

    default:
        g_snprintf(buf, sizeof(buf), "<unknown>");
        break;
    }

    return g_strdup(buf);
}


/* 原始值副本 stringify */
static char *cfg_raw_to_string(cfg_type_t type,
                         const void *value,
                         cfg_format_t fmt)
{
    if (!value)
        return g_strdup("<null>");

    return cfg_format_value(type, value, fmt);
}

static int cfg_read_node_from_item(const cfg_item_t *it, JsonNode **out)
{
    if (!it || !out)
        return -EINVAL;

    *out = NULL;

    JsonNode *node = json_node_new(JSON_NODE_VALUE);
    if (it->to_json) {
        /* custom representation */
        int ret = it->to_json(it, node);
        if (ret) {
            json_node_free(node);
            return ret;
        }
    }
    else {
        /* default primitive fallback */
        switch (it->type) {
        case CFG_INT8:
        case CFG_INT16:
        case CFG_INT32:
        case CFG_INT64:
            cfg_to_json_int(it, node);
            break;
        case CFG_BOOL:
            cfg_to_json_bool(it, node);
            break;
        case CFG_FLOAT:
        case CFG_DOUBLE:
            cfg_to_json_double(it, node);
            break;
        case CFG_STRING: {
            cfg_to_json_string(it, node);
            break;
        }
        default:
            json_node_free(node);
            return -EINVAL;
        }
    }

    *out = node;
    return 0;
}

int cfg_read_node(const char *key, JsonNode **out)
{
    if (!out)
        return -EINVAL;

    *out = NULL;

    g_rw_lock_reader_lock(&cfg_lock);

    cfg_item_t *it = cfg_find(key);
    if (!it) {
        g_rw_lock_reader_unlock(&cfg_lock);
        cfg_warning("%s: unregistered key\n", key);
        return -ENOENT;
    }

    int ret = cfg_read_node_from_item(it, out);

    g_rw_lock_reader_unlock(&cfg_lock);

    return ret;
}

/* ---------- generic get (string form, used by CLI) ---------- */
char *cfg_cli_read(const char *key)
{
    JsonNode *node;
    int ret = cfg_read_node(key, &node);
    if (ret != 0)
        return NULL;

    /* stringify */
    char *s = json_to_string(node, FALSE);
    json_node_free(node);

    return s;
}

/**
 * @brief Iterate over all configuration items with a given prefix.
 *
 * @param prefix : if not NULL or empty, only iterate over items whose keys start with this prefix. If NULL or empty, iterate over all items.
 * @param fn : the function to call for each matching item.
 * @param usr_data : user data to pass to the iteration function.
 */
static void cfg_foreach_item_with_prefix(const char *prefix, cfg_iter_fn fn, void *usr_data)
{
    if (!fn)
        return;

    FOREACH_CFG_ITEM(it) {
        if (prefix == NULL || strlen(prefix) == 0
             || g_str_has_prefix(it->key, prefix)) {
            fn(it, usr_data);
        }
    }
}

static void dump_one(cfg_item_t *it, void *user_data)
{
    JsonObject *root = user_data;

    #if 0
    /* dump out with style "key.subkey": "value" */
    JsonNode *node = NULL;
    if (cfg_read_node_from_item(it, &node) == 0) {
        json_object_set_member(root, it->key, node);
        /* ownership transferred to object */
    }
    #else
    /* dump out with style { "key": { "subkey": "value" } }*/
    JsonNode *node = NULL;
    if (cfg_read_node_from_item(it, &node) == 0) {
        char *leaf;
        JsonObject *obj = json_get_or_create(root, it->key, &leaf);
        json_object_set_member(obj, leaf, node);
        /* ownership transferred to object */
        g_free(leaf);
    }
    #endif
}

static int cfg_dump_nodes(const char *parent, JsonNode **out)
{
    if (!out)
        return -EINVAL;

    *out = NULL;

    JsonObject *obj = json_object_new();
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    json_node_take_object(root, obj);

    g_rw_lock_reader_lock(&cfg_lock);
    cfg_foreach_item_with_prefix(parent, dump_one, obj);
    g_rw_lock_reader_unlock(&cfg_lock);

    *out = root;
    return 0;
}

static int cfg_show_sub_json(const char *parent, FILE *out)
{
    if (!out)
        out = stdout;
    JsonNode *root;
    int ret = cfg_dump_nodes(parent, &root);
    if (ret != 0)
        return ret;

    char *s = json_to_string(root, TRUE);
    fprintf(out, "%s\n", s);

    g_free(s);
    json_node_free(root);
    return 0;
}

int cfg_show_all_json(FILE *out)
{
    return cfg_show_sub_json(NULL, out);
}

static void cfg_show_one(cfg_item_t *it, void *user_data)
{
    JsonNode *node;
    FILE *out = (FILE *)user_data;
    if (cfg_read_node(it->key, &node) != 0)
        return;

    char *s = json_to_string(node, FALSE);
    fprintf(out, "%s = %s\n", it->key, s);

    g_free(s);
    json_node_free(node);
}

void cfg_show_all(FILE *out)
{
    if (!out)
        out = stdout;
    g_rw_lock_reader_lock(&cfg_lock);
    cfg_foreach_item_with_prefix(NULL, cfg_show_one, out);
    g_rw_lock_reader_unlock(&cfg_lock);
    return;
}

int cfg_generate_to_json_data(const char *parent, char **out)
{
    size_t len = 0;

    /* 打开一个“写到内存”的 FILE* */
    FILE *mem = open_memstream(out, &len);
    if (!mem) {
        cfg_warning("failed to open_memstream\n");
        return len;
    }

    /* 直接复用现有函数 */
    cfg_show_sub_json(parent, mem);

    /* 必须 flush / fclose 才会更新 buf */
    fclose(mem);

    return len;
}

void cfg_show_status(FILE *out)
{
    if (!out)
        out = stdout;
    fprintf(out, "running\n");
    fprintf(out, "config-file: %s\n", cfg_file_path ? cfg_file_path : "<none>");
    fprintf(out, "total-items: %d\n", g_hash_table_size(cfg_items_map));
    fprintf(out, "total-audit-entries: %d\n", g_queue_get_length(&cfg_audit_log));
    fprintf(out, "value-changed: %s\n", is_changed_from_last_save ? "true" : "false");
    fprintf(out, "restart-required: %s\n",
           g_needs_restart ? "yes" : "no");

    if (g_needs_restart) {
        fprintf(out, "reasons:\n");
        for (guint i = 0; i < g_restart_reasons->len; i++) {
            cfg_item_t *reason = g_ptr_array_index(g_restart_reasons, i);
            char *v = cfg_cli_read(reason->key);
            if (v) {
                fprintf(out, "\t%s  - %s\n", reason->key, v);
                g_free(v);
            }
        }
    }
}

void cfg_history_show(FILE *out)
{
    if (!out)
        out = stdout;
    g_queue_foreach(&cfg_audit_log, (GFunc)cfg_audit_entry_print, out);
}

static void cfg_cli_help(const char *cmd, FILE *out)
{
    if (!cmd) {
        fprintf(out, "Available commands:\n");
        for (int i = 0; g_cli_cmds[i].cmd; i++) {
            fprintf(out, "  %-10s %-18s %s\n",
                   g_cli_cmds[i].cmd,
                   g_cli_cmds[i].args,
                   g_cli_cmds[i].desc);
        }
        fprintf(out, "\nUse: help <command>\n");
        return;
    }

    for (int i = 0; g_cli_cmds[i].cmd; i++) {
        if (strcmp(g_cli_cmds[i].cmd, cmd) == 0) {
            fprintf(out, "%s %s\n  %s\n",
                   g_cli_cmds[i].cmd,
                   g_cli_cmds[i].args,
                   g_cli_cmds[i].desc);
            return;
        }
    }

    fprintf(out, "Unknown command: %s\n", cmd);
}

static void cfg_cli_man(const char *item, FILE *out)
{
    if (!item) {
        fprintf(out, "Usage: man <item>\n");
        fprintf(out, "Type 'help' for available commands.\n");
        return;
    }

    struct type_size_map {
        size_t size;
        char *type_name;
    } type_size_maps[CFG_TYPE_NUMBER] = {
        [CFG_BOOL] = { sizeof(gboolean), "CFG_BOOL" },
        [CFG_INT8] = { sizeof(gint8), "CFG_INT8" },
        [CFG_INT16] = { sizeof(gint16), "CFG_INT16" },
        [CFG_INT32] = { sizeof(gint32), "CFG_INT32" },
        [CFG_INT64] = { sizeof(gint64), "CFG_INT64" },
        [CFG_FLOAT] = { sizeof(float), "CFG_FLOAT" },
        [CFG_DOUBLE] = { sizeof(double), "CFG_DOUBLE" },
        [CFG_STRING] = { sizeof(char *), "CFG_STRING" }
    };

    FOREACH_CFG_ITEM(it) {
        if (strcmp(it->key, item) == 0) {
            fprintf(out, ">> key: %s\n  type: %s(%ld)\n  flags: %s%s\n  desc: %s\n",
                   it->key,
                   type_size_maps[it->type].type_name,
                   type_size_maps[it->type].size,
                   (it->flags & CFG_FLAG_RUNTIME) ? "runtime " : "",
                   (it->flags & CFG_FLAG_RESTART) ? "restart " : "",
                   it->desc ? it->desc : "");
            return;
        }
    }

    fprintf(out, "Unknown item: %s\n", item);
}

/* cmd autocomplete function */
static char *cmd_generator(const char *text, int state)
{
    static int idx;
    if (state == 0)
        idx = 0;

    while (g_cli_cmds[idx].cmd) {
        const char *cmd = g_cli_cmds[idx++].cmd;
        if (g_str_has_prefix(cmd, text))
            return strdup(cmd);
    }
    return NULL;
}

static char **request_key_completion(const char *prefix)
{
    /* connect socket */
    int fd = connect_daemon();
    if (fd < 0)
        return NULL;

    FILE *in = fdopen(fd, "r");
    FILE *out = fdopen(dup(fd), "w");

    fprintf(out, "__complete_keys:%s\n", prefix);
    fflush(out);

    /* 读返回，逐行收集 */
    GPtrArray *list = g_ptr_array_new_with_free_func(free);
    char line[256];

    while (fgets(line, sizeof(line), in)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;

        if (strcmp(line, "END_OF_FILE") == 0)
            break;

        g_ptr_array_add(list, strdup(line));
    }

    fclose(in);
    fclose(out);
    close(fd);

    /* readline 需要 NULL 结尾 */
    g_ptr_array_add(list, NULL);

    return (char **)g_ptr_array_free(list, FALSE);
}

static char *key_generator(const char *text, int state)
{
    static char **matches = NULL;
    if (state == 0) {
       if (matches)
            g_strfreev(matches);
        matches = request_key_completion(text);
    }

    if (matches && matches[state])
        return strdup(matches[state]);
    return NULL;
}

static char **cli_completion(const char *text,
                             int start,
                             int end)
{
    (void)end;
    if (start == 0)
        return rl_completion_matches(text, cmd_generator);
    else {
        return rl_completion_matches(text, key_generator);
    }
}

#if 0
/* readline wrapper with support for "?" autocomplete */

static char *cli_complete_command(const char *prefix, char *(p_match_fun)(const char *, int))
{
    int match_cnt = 0;
    char *matches[100]; // 假设不超过 100 个命令
    do {
        matches[match_cnt] = p_match_fun(prefix, match_cnt);
    } while (matches[match_cnt] && ++match_cnt < 100);
    if (match_cnt == 0) {
        return NULL;
    }
    else if (match_cnt == 1) {
        return matches[0];
    } else {
        printf("\n");
        for (int i = 0; i < match_cnt; i++) {
            printf("%s  ", matches[i]);
            free(matches[i]);
        }
        printf("\n");
    }
    return NULL;
}

char *readline(const char *prompt)
{
    char line[256];
    int line_start = 0;
    char *completed_cmd = NULL;
    do {
        if (line_start > 0) {
            printf("%s%s", prompt, line);
        }
        else
            printf(prompt);

        if (!fgets(line+line_start, sizeof(line)-line_start, stdin))
            break;

        char *cmd = g_strstrip(line);
        if (strchr(cmd, '?')) {
            char *q = strchr(cmd, '?');
            *q = '\0';

            char **tok = g_strsplit(cmd, " ", 2);
            if (!tok[1]) {
                completed_cmd = cli_complete_command(tok[0], cmd_generator);
                if (completed_cmd) {
                    snprintf(line, sizeof(line), "%s", completed_cmd);
                    line_start = strlen(line);
                    g_free(completed_cmd);
                }
            }
            else {
                completed_cmd = cli_complete_command(tok[1], key_generator);
                if (completed_cmd) {
                    snprintf(line, sizeof(line), "%s %s", tok[0], completed_cmd);
                    line_start = strlen(line);
                    g_free(completed_cmd);
                }
                else {
                    snprintf(line, sizeof(line), "%s %s", tok[0], tok[1]);
                    line_start = strlen(line);
                }
            }

            g_strfreev(tok);
            continue;
        }
        else {
            return cmd;
        }
    } while (1);

    return NULL;
}
#endif

static void cfg_exec_line(const char *line, FILE *out)
{
    char cmdline[256];
    strncpy(cmdline, line, sizeof(cmdline));
    cmdline[sizeof(cmdline) - 1] = '\0';

    /* remove newline character */
    char *nl = strchr(cmdline, '\n');
    if (nl)
        *nl = '\0';

    char *cmd = g_strstrip(cmdline);

    if (strncmp(cmd, "__complete_keys:", 16) == 0) {
        cmd = g_strstrip(cmd + 16);
        const char *prefix = cmd;

        FOREACH_CFG_ITEM(it) {
            if (strncmp(it->key, prefix, strlen(prefix)) == 0)
                fprintf(out, "%s\n", it->key);
        }
        return;
    }

    /* process the command */
    if (g_str_has_suffix(cmd, "--help") || g_str_has_suffix(cmd, "-h")) {
        gchar **tokens = g_strsplit(cmd, " ", 2);
        cfg_cli_help(tokens[0], out);
        g_strfreev(tokens);
    } else if (g_str_has_prefix(cmd, "help ")) {
        cmd = g_strstrip(cmd + 5);
        cfg_cli_help(cmd, out);
    } else if (g_str_has_prefix(cmd, "man ")) {
        cmd = g_strstrip(cmd + 4);
        cfg_cli_man(cmd, out);
    } else if (g_str_has_prefix(cmd, "get ")) {
        cmd = g_strstrip(cmd + 4);
        char *v = cfg_cli_read(cmd);
        if (v) {
            fprintf(out, "%s\n", v);
            g_free(v);
        }
        else {
            fprintf(out, "unregistered for %s? 'show' to see all registered items.\n", cmd);
        }
    } else if (g_str_has_prefix(cmd, "set ")) {
        cmd = g_strstrip(cmd + 4);
        char **kv = g_strsplit(cmd, " ", 2);
        if (kv[0] && kv[1]) {
            if (cfg_cli_commit(kv[0], kv[1]) != 0)
                fprintf(out, "Set failed (maybe out of type range), 'man %s' for help\n", kv[0]);
        }
        g_strfreev(kv);
    } else if (strcmp(cmd, "show") == 0) {
        cfg_show_all(out);
    } else if (strcmp(cmd, "show_json") == 0) {
        cfg_show_all_json(out);
    } else if (strcmp(cmd, "save") == 0) {
        if (!cfg_file_path) {
            fprintf(out, "The saved path is NULL, checking whether cfg_load_file failed to executed before\n");
            return;
        }
        if (cfg_save_file(NULL, FALSE) == 0)
            fprintf(out, "Config saved successfully.\n");
        else
            fprintf(out, "Failed to save config.(maybe no changes to save)\n");
    } else if (g_str_has_prefix(cmd, "save_as ")) {
        cmd = g_strstrip(cmd + strlen("save_as "));
        if (cfg_save_file(cmd, TRUE) == 0)
            fprintf(out, "Config saved successfully to %s.\n", cmd);
        else
            fprintf(out, "Failed to save config to %s.\n", cmd);
    } else if (strcmp(cmd, "status") == 0) {
        cfg_show_status(out);
    } else if (strcmp(cmd, "history") == 0) {
        cfg_history_show(out);
    } else if (strcmp(cmd, "quit") == 0) {
        fprintf(out, "Bye!\n");
    } else {
        cfg_cli_help(NULL, out);
    }
}

static void *handle_client(void *user_data)
{
    client_info_t *info = user_data;
    FILE *in = fdopen(info->fd, "r");
    FILE *out = fdopen(dup(info->fd), "w");
    char line[512];

    if (!in || !out) {
        close(info->fd);
        return NULL;
    }

   while (cfg_cli_listening_running && fgets(line, sizeof(line), in)) {
        cfg_exec_line(line, out);
        fflush(out);

        /* indicate end of result */
        fprintf(out, "END_OF_FILE\n");
        fflush(out);

        if (strncmp(line, "quit", 4) == 0)
            break;
    }

    fclose(in);
    fclose(out);
    if (info->fd >= 0)
        close(info->fd);
    g_free(info);
    return NULL;
}

static void *cfg_cli_server_run_loop(char *listen_name)
{
    if (!listen_name) {
        cfg_error("No listen name provided, skipping daemon mode\n");
        return NULL;
    }

    cfg_set_sockpath(listen_name);
    g_free(listen_name);

    listen_fd = create_listen_socket();
    if (listen_fd < 0) {
        cfg_error("Failed to create listen socket\n");
        return NULL;
    }

    int flags = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

    struct threads_list {
        GThread *cur;
        client_info_t *info;
        struct threads_list *next;
        struct threads_list *prev;
    } threads = {0};

    cfg_info("Daemon running, socket: %s\n", (char *)get_socket_path()+1);
    while (cfg_cli_listening_running) {
        struct pollfd pfd = {
            .fd = listen_fd,
            .events = POLLIN
        };

        int ret = poll(&pfd, 1, 50);  // 50ms timeout
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            cfg_error("Failed to poll\n");
            break;
        }

        if (ret == 0)
            continue;  // timeout，go back to check running flag

        if (pfd.revents & POLLIN) {
            for(;;) {
                int client_fd = accept(listen_fd, NULL, NULL);
                if (client_fd < 0) {
                    if (cfg_cli_listening_running == FALSE) {
                        break;
                    }
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    cfg_warning("Accept failed: %s\n", strerror(errno));
                    break;
                }

                /* handle client in a separate thread or process */
                // For simplicity, we just close the connection here
                client_info_t *info = g_new0(client_info_t, 1);
                info->fd = client_fd;
                struct threads_list *t = g_new0(struct threads_list, 1);
                t->cur = g_thread_new("cfg_cli_handle", (GThreadFunc)handle_client, info);
                t->info = info;
                t->next = threads.next;
                t->prev = &threads;
                if (threads.next)
                    threads.next->prev = t;
                threads.next = t;
            }
        }
    }

    cfg_info("Server stopping, waiting for client threads to exit...\n");
    while (threads.next) {
        struct threads_list *t = threads.next;

        if (t->info->fd >= 0) {
            shutdown(t->info->fd, SHUT_RDWR);  // wakeup clients fgets, make client thread to exit
            close(t->info->fd);                // release fd resource in server side
            t->info->fd = -1;
        }

        g_thread_join(t->cur);
        threads.next = t->next;
        if (t->next)
            t->next->prev = &threads;

        g_free(t);
    }
    cfg_info("All client threads exited, server stopped\n");
    return NULL;
}

void cfg_cli_server_run(char *listen_name, gboolean in_thread)
{
    if (!in_thread) {
        cfg_cli_server_run_loop(g_strdup(listen_name));
        return;
    }
    if (cfg_cli_server_thread) {
        cfg_info("Server is already running\n");
        return;
    }
    cfg_cli_server_thread = g_thread_new("cfg_cli_server", (GThreadFunc)cfg_cli_server_run_loop, g_strdup(listen_name));
}

void cfg_cli_server_stop(void)
{
    cfg_cli_listening_running = FALSE;
    if (listen_fd >= 0) {
        close(listen_fd);
        listen_fd = -1;
    }
    if (cfg_cli_server_thread) {
        cfg_info("Waiting for server thread to exit...\n");
        g_thread_join(cfg_cli_server_thread);
        cfg_cli_server_thread = NULL;
        cfg_info("Server thread exited\n");
    }
}

void cfg_cli_client_run(char *server_name)
{
    /*
    #define HISTORY_FILE "~/.cfg_ctl_history"`
    // read at startup
    using_history();
    read_history(HISTORY_FILE);
    // save after quit
    write_history(HISTORY_FILE);
    */
    if (!server_name) {
        cfg_error("No server name provided, skipping client mode\n");
        return;
    }

    rl_attempted_completion_function = cli_completion;

    cfg_set_sockpath(server_name);
    int fd = connect_daemon();
    if (fd < 0) {
        cfg_error("Failed to connect to daemon: %s\n", get_socket_path()+1);
        return;
    }

    FILE *in = fdopen(fd, "r");
    FILE *out = fdopen(dup(fd), "w");

    while (1) {
        char *line = readline("> ");
        if (!line) break;
        add_history(line);

        char *cmd = g_strstrip(line);
        if (*cmd) {
            fprintf(out, "%s\n", cmd);
            fflush(out);

            char response[1024];
            while (fgets(response, sizeof(response), in)) {
                /* indicate end of result */
                if (strcmp(response, "END_OF_FILE\n") == 0)
                    break;
                printf("%s", response);
            }

            if (strncmp(cmd, "quit", 4) == 0) {
                free(line);
                break;
            }
        }

        free(line);
    }
    fclose(in);
    fclose(out);
    close(fd);
}

/* ---------- load / save ---------- */

int cfg_load_file(const char *path)
{
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_file(parser, path, NULL)) {
        cfg_error("Failed to parse a json file: %s\n", path);
        return -1;
    }

    int ret = 0;
    JsonNode *root = json_parser_get_root(parser);

    g_rw_lock_writer_lock(&cfg_lock);
    FOREACH_CFG_ITEM(it) {
        JsonNode *n = json_get_by_path(root, it->key);
        if (n && JSON_NODE_HOLDS_VALUE(n)) {
            GType expect = cfg_type_to_gtype(it->type);
            GType actual = json_node_get_value_type(n);
            if (actual == expect) {
                ret += it->from_json(n, it);
            }
            else {
                cfg_warning("Type mismatch for key '%s': expected %s but got %s\n",
                       it->key,
                       g_type_name(expect),
                       g_type_name(actual));
                ret += -1;
            }
        }
    }
    g_rw_lock_writer_unlock(&cfg_lock);

    g_object_unref(parser);

    // means some keys are not match with the file so that let it be overwriting
    if (!ret)
        is_changed_from_last_save = FALSE;

    if ( cfg_file_path == NULL || g_strcmp0(cfg_file_path, path) != 0) {
        if (cfg_file_path)
            g_free(cfg_file_path);
        cfg_file_path = g_strdup(path);
    }

    return ret;
}

/*
 * Atomically write data to `path`.
 *
 * Guarantee:
 *  - Either old file or full new file is visible
 *  - No partial writes
 *  - Safe against crashes and power loss
 *
 * Return:
 *  - 0 on success
 *  - -1 on error
 */
static int atomic_write_file(const char *path,
                             const char *data,
                             gsize len)
{
    int ret = -1;
    int fd = -1;
    int dirfd = -1;

    if (!path || !data) {
        cfg_warning("Invalid arguments to atomic_write_file (path: %s, data: %p)\n"
            , path ? path : "null", (void*)data);
        return -1;
    }

    gchar *tmp_path = g_strconcat(path, ".tmp", NULL);

    /* Open temporary file */
    fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        cfg_warning("Failed to open temp file: %s\n", strerror(errno));
        goto out;
    }

    /* Write full buffer */
    ssize_t written = write(fd, data, len);
    if (written != (ssize_t)len) {
        cfg_warning("Failed to write data: %s\n", strerror(errno));
        goto out;
    }

    /* Flush file contents to disk */
    if (fsync(fd) != 0) {
        cfg_warning("Failed to fsync temp file: %s\n", strerror(errno));
        goto out;
    }

    close(fd);
    fd = -1;

    /* Atomically replace target file */
    if (rename(tmp_path, path) != 0) {
        cfg_warning("Failed to rename temp file: %s\n", strerror(errno));
        goto out;
    }

    /*
     * fsync parent directory to ensure the rename
     * is persisted after power loss
     */
    gchar *dir = g_path_get_dirname(path);
    dirfd = open(dir, O_DIRECTORY | O_RDONLY);
    if (dirfd >= 0) {
        fsync(dirfd);
        close(dirfd);
    }
    g_free(dir);

    ret = 0;

out:
    if (fd >= 0) {
        close(fd);
    }
    if (ret != 0) {
        unlink(tmp_path);
        cfg_warning("Failed to save config file: %s\n", strerror(errno));
    }

    g_free(tmp_path);
    return ret;
}

int cfg_save_file(const char *path, gboolean force)
{
    if (!is_changed_from_last_save && !force) {
        cfg_info("No changes since last save, skipping write.\n");
        return -1;
    }

    if (!path && !cfg_file_path) {
        cfg_error("check whether cfg_load_file failed to executed if path is NULL!\n");
        return -1;
    }

    JsonObject *root = json_object_new();
    JsonNode *node = json_node_new(JSON_NODE_OBJECT);
    json_node_take_object(node, root);

    g_rw_lock_reader_lock(&cfg_lock);
    FOREACH_CFG_ITEM(it) {
        JsonNode *value;
        if (cfg_read_node_from_item(it, &value) == 0) {
            char *leaf;
            JsonObject *obj = json_get_or_create(root, it->key, &leaf);
            json_object_set_member(obj, leaf, value);
            g_free(leaf);
        }
    }
    g_rw_lock_reader_unlock(&cfg_lock);

    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, node);
    json_generator_set_pretty(gen, TRUE);

    gsize data_len;
    char *data = json_generator_to_data(gen, &data_len);

    int ret = atomic_write_file(path ? path : cfg_file_path, data, data_len);
    if (ret == 0) {
        if (!force)
            is_changed_from_last_save = FALSE;
    }

    g_free(data);
    g_object_unref(gen);
    json_node_free(node);
    return ret;
}

/* ---------- system init ---------- */
void cfg_system_init(void)
{
    if (g_once_init_enter(&cfg_initialized)) {
        g_rw_lock_init(&cfg_lock);
        cfg_items_map = g_hash_table_new(
                                g_str_hash, g_str_equal);
        g_restart_reasons = g_ptr_array_new_with_free_func(g_free);
        cfg_change_worker_run();
        g_once_init_leave(&cfg_initialized, TRUE);
    }
}

int cfg_parse_json_to_vars(const char *json_str, cfg_item_t *items, size_t item_size)
{
    JsonParser *parser = json_parser_new();
    JsonNode *root;
    int ret = -1;

    GError *error = NULL;
    if (!json_parser_load_from_data(parser, json_str, -1, &error)) {
        cfg_warning("Failed to parse JSON data: %s\n", error ? error->message : "unknown error");
        if (error) g_error_free(error);
        goto out;
    }

    root = json_parser_get_root(parser);
    if (!root || json_node_get_node_type(root) != JSON_NODE_OBJECT) {
        cfg_warning("Invalid JSON structure\n");
        goto out;
    }

    ret = 0;
    for (size_t i = 0; i < item_size; i++) {
        cfg_item_t *item = &items[i];
        JsonNode *n = json_get_by_path(root, item->key);
        if (n && JSON_NODE_HOLDS_VALUE(n)) {
            if (cfg_type_check_normalize(item)) {
                cfg_warning("Validation failed for key '%s'\n", item->key);
                ret += -1;
                continue;
            }
            GType expect = cfg_type_to_gtype(item->type);
            GType actual = json_node_get_value_type(n);
            if (actual == expect) {
                ret += item->from_json(n, item);
            }
            else {
                cfg_warning("Type mismatch for key '%s': expected %s but got %s\n",
                       item->key,
                       g_type_name(expect),
                       g_type_name(actual));
                ret += -1;
            }
        }
    }

out:
    g_object_unref(parser);
    return ret;
}
