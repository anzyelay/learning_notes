#include "jsonconfig.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib.h>

// command and key autocomplete
#include <readline/readline.h>
#include <readline/history.h>

#define FOREACH_CFG_ITEM(it) \
    for (cfg_item_t *it = cfg_list; it; it = it->next)
typedef struct {
    cfg_item_t *item;

    union {
        int i;
        gboolean b;
        double d;
        char *s;
    } old_value;

    union {
        int i;
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
    { "show_json",  "",                 "Show all config values with json style" },
    { "show",       "",                 "Show all config values with plain text" },
    { "help",       "[cmd]",            "Show help for special command or all commands" },
    { "man",        "item",             "Show the description for item" },
    { "quit",       "",                 "Exit CLI" },
    { NULL, NULL, NULL }
};

typedef void (*cfg_iter_fn)(cfg_item_t *it, void *user_data);

void cfg_foreach_item(cfg_iter_fn fn, void *user_data);


/* 1. 从“原始值副本” stringify（事务 / rollback / audit） */
static char *cfg_raw_to_string(cfg_type_t type,
                         const void *value,
                         cfg_format_t fmt);

/* ---------- globals ---------- */

static cfg_item_t *cfg_list = NULL;
static GRWLock cfg_lock;
static GAsyncQueue *cfg_change_queue;
static GThread *cfg_change_thread;
static gboolean cfg_worker_running = TRUE;

static gboolean g_needs_restart = FALSE;
static GPtrArray *g_restart_reasons;   // cfg_item_t*

static GQueue cfg_audit_log;   // 有上限，比如 1000
gboolean is_changed_from_last_save = TRUE;


#include <sys/socket.h>
#include <sys/un.h>
#define SOCK_PATH_FORMAT "\0/tmp/%s.socket"

typedef struct {
    int fd;
    pthread_t pid;
} client_info_t;

static int listen_fd = -1;
static char g_sockpath_name[108];
static gboolean cfg_cli_listening_running = TRUE;
static void cfg_set_sockpath(const char *name)
{
    snprintf(g_sockpath_name, sizeof(g_sockpath_name), SOCK_PATH_FORMAT, name);
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
        perror("connect");
        close(fd);
        return -1;
    }
    return fd;
}

static int create_listen_socket(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", get_socket_path());

    unlink(get_socket_path()); // remove old socket if exists

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    chmod(addr.sun_path, 0660);

    if (listen(fd, 5) < 0) {
        perror("listen");
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

static void cfg_copy_value(cfg_type_t type,
                           void *dst,
                           const void *src,
                           gboolean need_free)
{
    switch (type) {
    case CFG_INT:
        *(int *)dst = *(const int *)src;
        break;
    case CFG_BOOL:
        *(gboolean *)dst = *(const gboolean *)src;
        break;
    case CFG_DOUBLE:
        *(double *)dst = *(const double *)src;
        break;
    case CFG_STRING:
        if (need_free && *(char **)dst)
            g_free(*(char **)dst);
        *(char **)dst = g_strdup(*(char * const *)src);
        break;
    }
}

/* worker thread */
static gpointer cfg_change_worker(gpointer data)
{
    while (cfg_worker_running) {
        cfg_change_task_t *task =
            g_async_queue_pop(cfg_change_queue);

        // Check for quit signal
        if (!cfg_worker_running)
            break;
        // printf("Worker thread processing task for item: %s\n", task->item ? task->item->key : "NULL");

        if (!task)
            continue;

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

            g_printerr(
              "rollback: %s\n", it->key);
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

/* enqueue config change */
static void cfg_enqueue_change(cfg_item_t *it,
                               const void *oldv,
                               const void *newv)
{
    cfg_change_task_t *task = g_new0(cfg_change_task_t, 1);
    task->item = it;

    cfg_copy_value(it->type, &task->old_value, oldv, FALSE);
    cfg_copy_value(it->type, &task->new_value, newv, FALSE);

    g_async_queue_push(cfg_change_queue, task);
}

/* ---------- registry ---------- */

void cfg_register(cfg_item_t *item)
{
    item->next = cfg_list;
    cfg_list = item;
}

static cfg_item_t *cfg_find(const char *key)
{
    FOREACH_CFG_ITEM(it) {
        if (g_strcmp0(it->key, key) == 0)
            return it;
    }
    return NULL;
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
    case CFG_INT:    return G_TYPE_INT64;
    case CFG_BOOL:   return G_TYPE_BOOLEAN;
    case CFG_DOUBLE: return G_TYPE_DOUBLE;
    case CFG_STRING: return G_TYPE_STRING;
    default:         return G_TYPE_INVALID;
    }
}

static gboolean cfg_value_equal(cfg_type_t type, const void *a, const void *b)
{
    switch (type) {
    case CFG_INT:
        return *(int *)a == *(int *)b;

    case CFG_BOOL:
        return *(gboolean *)a == *(gboolean *)b;

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
        /* not expected type，return -EINVAL */
        return -EINVAL;
    }

    /* backup */
    union {
        int i;
        gboolean b;
        double d;
        char *s;
    } old;

    cfg_copy_value(it->type, &old, it->storage, FALSE);

    /* apply */
    if (it->from_json(node, it->storage) != 0) {
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
        ret = -1;
        goto out;
    }

    ret = cfg_commit_value(it, node);

out:
    g_rw_lock_writer_unlock(&cfg_lock);
    return ret;
}
/* ---------- type handlers ---------- */

int cfg_from_json_int(JsonNode *n, void *p)
{
    *(int *)p = json_node_get_int(n);
    return 0;
}

int cfg_to_json_int(const void *p, JsonNode *n)
{
    json_node_set_int(n, *(int *)p);
    return 0;
}

int cfg_from_json_bool(JsonNode *n, void *p)
{
    *(gboolean *)p = json_node_get_boolean(n);
    return 0;
}

int cfg_to_json_bool(const void *p, JsonNode *n)
{
    json_node_set_boolean(n, *(gboolean *)p);
    return 0;
}

int cfg_from_json_double(JsonNode *n, void *p)
{
    *(double *)p = json_node_get_double(n);
    return 0;
}

int cfg_to_json_double(const void *p, JsonNode *n)
{
    json_node_set_double(n, *(double *)p);
    return 0;
}

int cfg_from_json_string(JsonNode *n, void *p)
{
    char **dst = (char **)p;

    /* defensive: free old value first */
    g_free(*dst);
    *dst = NULL;

    if (!n)
        return -EINVAL;

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
    if (s)
        *dst = g_strdup(s);

    return 0;
}

int cfg_to_json_string(const void *p, JsonNode *n)
{
    char *dst = *(char **)p;
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

int cfg_read_int(const char *key, int *out)
{
    CFG_GET_COMMON(CFG_INT)
    *out = *(int *)it->storage;
    g_rw_lock_reader_unlock(&cfg_lock);
    return 0;
}

int cfg_read_bool(const char *key, gboolean *out)
{
    CFG_GET_COMMON(CFG_BOOL)
    *out = *(gboolean *)it->storage;
    g_rw_lock_reader_unlock(&cfg_lock);
    return 0;
}

int cfg_read_double(const char *key, double *out)
{
    CFG_GET_COMMON(CFG_DOUBLE)
    *out = *(double *)it->storage;
    g_rw_lock_reader_unlock(&cfg_lock);
    return 0;
}

int cfg_read_string(const char *key, const char **out)
{
    CFG_GET_COMMON(CFG_STRING)
    *out = *(char **)it->storage;
    g_rw_lock_reader_unlock(&cfg_lock);
    return 0;
}

/* ---------- typed SET (非 CLI) ---------- */

int cfg_commit_int(const char *key, int value)
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
    case CFG_INT: {
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
        ret = -1;
        goto out;
    }

    JsonNode *node = cfg_parse_string_to_node(it, value, &ret);
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
    case CFG_INT:
        g_snprintf(buf, sizeof(buf),
                   "%d", *(const int *)v);
        break;

    case CFG_BOOL:
        g_snprintf(buf, sizeof(buf),
                   "%s",
                   *(const gboolean *)v
                       ? "true" : "false");
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
        int ret = it->to_json(it->storage, node);
        if (ret) {
            json_node_free(node);
            return ret;
        }
    }
    else {
        /* default primitive fallback */
        switch (it->type) {
        case CFG_INT:
            json_node_set_int(node, *(int *)it->storage);
            break;
        case CFG_BOOL:
            json_node_set_boolean(node, *(gboolean *)it->storage);
            break;
        case CFG_DOUBLE:
            json_node_set_double(node, *(double *)it->storage);
            break;
        case CFG_STRING: {
            char *s = *(char **)it->storage;
            if (s) {
                json_node_set_string(node, s);
            } else {
                json_node_init_null(node);
            }
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

void cfg_foreach_item(cfg_iter_fn fn, void *usr_data)
{
    if (!fn)
        return;

    FOREACH_CFG_ITEM(it) {
        fn(it, usr_data);
    }
}

static void dump_one(cfg_item_t *it, void *user_data)
{
    JsonObject *root = user_data;

    #if 1
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

static int cfg_dump_all_nodes(JsonNode **out)
{
    if (!out)
        return -EINVAL;

    *out = NULL;

    JsonObject *obj = json_object_new();
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    json_node_take_object(root, obj);

    g_rw_lock_reader_lock(&cfg_lock);
    cfg_foreach_item(dump_one, obj);
    g_rw_lock_reader_unlock(&cfg_lock);

    *out = root;
    return 0;
}

int cfg_show_all_json(FILE *out)
{
    if (!out)
        out = stdout;
    JsonNode *root;
    int ret = cfg_dump_all_nodes(&root);
    if (ret != 0)
        return ret;

    char *s = json_to_string(root, TRUE);
    fprintf(out, "%s\n", s);

    g_free(s);
    json_node_free(root);
    return 0;
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
    cfg_foreach_item(cfg_show_one, out);
    g_rw_lock_reader_unlock(&cfg_lock);
    return;
}

int cfg_show_to_buffer(char **out)
{
    size_t len = 0;

    /* 打开一个“写到内存”的 FILE* */
    FILE *mem = open_memstream(out, &len);
    if (!mem) {
        perror("open_memstream");
        return len;
    }

    /* 直接复用现有函数 */
    cfg_show_all_json(mem);

    /* 必须 flush / fclose 才会更新 buf */
    fclose(mem);

    return len;
}

void cfg_show_status(FILE *out)
{
    if (!out)
        out = stdout;
    fprintf(out, "running\n");
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

    FOREACH_CFG_ITEM(it) {
        if (strcmp(it->key, item) == 0) {
            fprintf(out, "%s\n  type: %s\n  flags: %s%s\n  desc: %s\n",
                   it->key,
                   (it->type == CFG_INT) ? "int" :
                   (it->type == CFG_BOOL) ? "bool" :
                   (it->type == CFG_DOUBLE) ? "double" :
                   (it->type == CFG_STRING) ? "string" : "unknown",
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

    fprintf(out, "__complete_keys %s\n", prefix);
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
    char cmd[256];
    strncpy(cmd, line, sizeof(cmd));
    cmd[sizeof(cmd) - 1] = '\0';

    /* remove newline character */
    char *nl = strchr(cmd, '\n');
    if (nl)
        *nl = '\0';

    if (strncmp(cmd, "__complete_keys ", 16) == 0) {
        const char *prefix = cmd + 16;

        FOREACH_CFG_ITEM(it) {
            if (strncmp(it->key, prefix, strlen(prefix)) == 0)
                fprintf(out, "%s\n", it->key);
        }
        return;
    }

    /* process the command */
    if (g_str_has_prefix(cmd, "help ")) {
        cfg_cli_help(cmd + 5, out);
    } else if (g_str_has_prefix(cmd, "man ")) {
        cfg_cli_man(cmd + 4, out);
    }
    else if (g_str_has_suffix(cmd, "--help") || g_str_has_suffix(cmd, "-h")) {
        gchar **tokens = g_strsplit(cmd, " ", 2);
        cfg_cli_help(tokens[0], out);
        g_strfreev(tokens);
    }
    else if (g_str_has_prefix(cmd, "get ")) {
        char *v = cfg_cli_read(cmd + 4);
        if (v) {
            fprintf(out, "%s\n", v);
            g_free(v);
        }
    } else if (g_str_has_prefix(cmd, "set ")) {
        char **kv = g_strsplit(cmd + 4, " ", 2);
        if (kv[0] && kv[1]) {
            if (cfg_cli_commit(kv[0], kv[1]) != 0)
                fprintf(out, "set failed (maybe restart required)\n");
        }
        g_strfreev(kv);
    } else if (strcmp(cmd, "show") == 0) {
        cfg_show_all(out);
    } else if (strcmp(cmd, "show_json") == 0) {
        cfg_show_all_json(out);
    } else if (strcmp(cmd, "save") == 0) {
        cfg_save_file("config.json");
    } else if (strcmp(cmd, "status") == 0) {
        cfg_show_status(out);
    } else if (strcmp(cmd, "history") == 0) {
        cfg_history_show(out);
    }
    else if (strcmp(cmd, "quit") == 0) {
        fprintf(out, "Bye!\n");
    } else {
        cfg_cli_help(NULL, out);
    }
}

static int handle_client(void *user_data)
{
    client_info_t *info = user_data;
    FILE *in = fdopen(info->fd, "r");
    FILE *out = fdopen(dup(info->fd), "w");
    char line[512];

    if (!in || !out) {
        close(info->fd);
        return -1;
    }

    while (fgets(line, sizeof(line), in)) {
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
    close(info->fd);
    g_free(info);
    return 0;
}

static void cleanup(int signum)
{
    cfg_worker_running = FALSE;
    cfg_cli_listening_running = FALSE;
    close(listen_fd);
    listen_fd = -1;
    // Use a special pointer value to signal the worker thread to exit
    int quit_signal = 1;
    g_async_queue_push(cfg_change_queue, &quit_signal); // wake up worker thread
    g_usleep(100 * 1000); // wait for worker thread to exit
    exit(0);
}

void cfg_cli_daemon_run(char *listen_name)
{
    if (!listen_name) {
        fprintf(stderr, "No listen name provided, skipping daemon mode\n");
        return;
    }

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    cfg_set_sockpath(listen_name);
    listen_fd = create_listen_socket();
    if (listen_fd < 0) {
        fprintf(stderr, "Failed to create listen socket\n");
        return;
    }

    printf("daemon running, socket: %s\n", get_socket_path());
    while (cfg_cli_listening_running) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
             if (errno == EBADF) { // 套接字被关闭了
                printf("listen socket closed, exiting daemon loop\n");
                break;
            }
            perror("accept");
            continue;
        }

        /* handle client in a separate thread or process */
        // For simplicity, we just close the connection here
        client_info_t *info = g_new0(client_info_t, 1);
        info->fd = client_fd;
        // pthread_t pid;
        info->pid = g_thread_new("cfg_cli_handle", (GThreadFunc)handle_client, info);
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
        fprintf(stderr, "No server name provided, skipping client mode\n");
        return;
    }

    rl_attempted_completion_function = cli_completion;

    cfg_set_sockpath(server_name);
    int fd = connect_daemon();
    if (fd < 0) {
        fprintf(stderr, "Failed to connect to daemon\n");
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
    if (!json_parser_load_from_file(parser, path, NULL))
        return -1;

    JsonNode *root = json_parser_get_root(parser);

    g_rw_lock_writer_lock(&cfg_lock);
    FOREACH_CFG_ITEM(it) {
        JsonNode *n = json_get_by_path(root, it->key);
        if (n)
            it->from_json(n, it->storage);
    }
    g_rw_lock_writer_unlock(&cfg_lock);

    g_object_unref(parser);

    is_changed_from_last_save = FALSE;
    return 0;
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

    gchar *tmp_path = g_strconcat(path, ".tmp", NULL);

    /* Open temporary file */
    fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("Failed to open temp file: %s\n", strerror(errno));
        goto out;
    }

    /* Write full buffer */
    ssize_t written = write(fd, data, len);
    if (written != (ssize_t)len) {
        printf("Failed to write data: %s\n", strerror(errno));
        goto out;
    }

    /* Flush file contents to disk */
    if (fsync(fd) != 0) {
        printf("Failed to fsync temp file: %s\n", strerror(errno));
        goto out;
    }

    close(fd);
    fd = -1;

    /* Atomically replace target file */
    if (rename(tmp_path, path) != 0) {
        printf("Failed to rename temp file: %s\n", strerror(errno));
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
        printf("Failed to save config file: %s\n", strerror(errno));
    }

    g_free(tmp_path);
    return ret;
}

int cfg_save_file(const char *path)
{
    if (!is_changed_from_last_save) {
        printf("No changes since last save, skipping write.\n");
        return 0;
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

    int data_len;
    char *data = json_generator_to_data(gen, &data_len);

    int ret = atomic_write_file(path, data, data_len);
    if (ret == 0) {
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
    g_rw_lock_init(&cfg_lock);
    g_restart_reasons = g_ptr_array_new_with_free_func(g_free);

    cfg_change_queue = g_async_queue_new();
    cfg_change_thread =
        g_thread_new("cfg-worker",
                     cfg_change_worker,
                     NULL);
}
