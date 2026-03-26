#include "config.h"
#include <stdio.h>

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
    { "get",     "<key>",            "Get config value" },
    { "set",     "<key> <value>",    "Set config value (runtime)" },
    { "status",  "",                 "Show system status" },
    { "history", "",                 "Show config change history" },
    { "save",    "",                 "Save config to file" },
    { "show",  "",                   "Show all config values with json style" },
    { "show_plain",  "",             "Show all config values with plain text" },
    { "help",    "[cmd]",            "Show help" },
    { "quit",    "",                 "Exit CLI" },
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
}

static void cfg_audit_entry_print(cfg_audit_entry_t *entry)
{
    gchar log_time[64];
    strftime(log_time, sizeof(log_time), "%Y-%m-%d %H:%M:%S", localtime(&entry->ts));
    printf("[%s] %s: %s -> %s\n", log_time, entry->key, entry->old_value, entry->new_value);
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
static int cfg_cli_commit(const char *key, const char *value)
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

/* ---------- debug ---------- */
/* ---------- generic get (string form, used by CLI) ---------- */
static int cfg_cli_read(const char *key, char **out)
{
    JsonNode *node;
    int ret = cfg_read_node(key, &node);
    if (ret != 0)
        return ret;

    /* stringify */
    char *s = json_to_string(node, FALSE);
    json_node_free(node);

    *out = s;
    return 0;

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

int cfg_dump_all_nodes(JsonNode **out)
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

int cfg_show_all(void)
{
    JsonNode *root;
    int ret = cfg_dump_all_nodes(&root);
    if (ret != 0)
        return ret;

    char *s = json_to_string(root, TRUE);
    puts(s);

    g_free(s);
    json_node_free(root);
    return 0;
}

static void cli_show_one(cfg_item_t *it, void *unused)
{
    JsonNode *node;
    if (cfg_read_node(it->key, &node) != 0)
        return;

    char *s = json_to_string(node, FALSE);
    printf("%s = %s\n", it->key, s);

    g_free(s);
    json_node_free(node);
}

void cfg_show_all_plain(void)
{
    g_rw_lock_reader_lock(&cfg_lock);
    cfg_foreach_item(cli_show_one, NULL);
    g_rw_lock_reader_unlock(&cfg_lock);
    return;
}

void cfg_show_status(void)
{
    printf("running\n");
    printf("restart-required: %s\n",
           g_needs_restart ? "yes" : "no");

    if (g_needs_restart) {
        printf("reasons:\n");
        for (guint i = 0; i < g_restart_reasons->len; i++) {
            cfg_item_t *reason = g_ptr_array_index(g_restart_reasons, i);
            char *v;
            cfg_cli_read(reason->key, &v);
            printf("\t%s  - %s\n", reason->key, v);
            g_free(v);
        }
    }
}

void cfg_history_show(void)
{
    g_queue_foreach(&cfg_audit_log, (GFunc)cfg_audit_entry_print, NULL);
}

static void cfg_cli_help(const char *cmd)
{
    if (!cmd) {
        printf("Available commands:\n");
        for (int i = 0; g_cli_cmds[i].cmd; i++) {
            printf("  %-10s %-18s %s\n",
                   g_cli_cmds[i].cmd,
                   g_cli_cmds[i].args,
                   g_cli_cmds[i].desc);
        }
        printf("\nUse: help <command>\n");
        return;
    }

    for (int i = 0; g_cli_cmds[i].cmd; i++) {
        if (strcmp(g_cli_cmds[i].cmd, cmd) == 0) {
            printf("%s %s\n  %s\n",
                   g_cli_cmds[i].cmd,
                   g_cli_cmds[i].args,
                   g_cli_cmds[i].desc);
            return;
        }
    }

    printf("Unknown command: %s\n", cmd);
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

static char *key_generator(const char *text, int state)
{
    static cfg_item_t *it;
    if (state == 0)
        it = cfg_list;

    while (it) {
        const char *k = it->key;
        it = it->next;
        if (g_str_has_prefix(k, text))
            return strdup(k);
    }
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

void cfg_cli_run(void)
{
    char line[256];
    /* readline  */
    rl_attempted_completion_function = cli_completion;
    while (1) {
        char *line = readline("> ");
        if (!line) break;
        add_history(line);

        char *cmd = g_strstrip(line);

        if (g_str_has_prefix(cmd, "help ")) {
            cfg_cli_help(cmd + 5);
        }
        else if (g_str_has_suffix(cmd, "--help") || g_str_has_suffix(cmd, "-h")) {
            gchar **tokens = g_strsplit(cmd, " ", 2);
            cfg_cli_help(tokens[0]);
            g_strfreev(tokens);
        }
        else if (g_str_has_prefix(cmd, "get ")) {
            char *v;
            if (cfg_cli_read(cmd + 4, &v) == 0) {
                printf("%s\n", v);
                g_free(v);
            }
        } else if (g_str_has_prefix(cmd, "set ")) {
            char **kv = g_strsplit(cmd + 4, " ", 2);
            if (kv[0] && kv[1]) {
                if (cfg_cli_commit(kv[0], kv[1]) != 0)
                    printf("set failed (maybe restart required)\n");
            }
            g_strfreev(kv);
        } else if (strcmp(cmd, "show") == 0) {
            cfg_show_all();
        } else if (strcmp(cmd, "show_plain") == 0) {
            cfg_show_all_plain();
        } else if (strcmp(cmd, "save") == 0) {
            cfg_save_file("config.json");
        } else if (strcmp(cmd, "status") == 0) {
            cfg_show_status();
        } else if (strcmp(cmd, "history") == 0) {
            cfg_history_show();
        } else if (strcmp(cmd, "quit") == 0) {
            break;
        } else {
            cfg_cli_help(NULL);
        }
    }
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
    return 0;
}

int cfg_save_file(const char *path)
{
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
    json_generator_to_file(gen, path, NULL);

    g_object_unref(gen);
    json_node_free(node);
    return 0;
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
