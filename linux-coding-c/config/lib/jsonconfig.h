#ifndef JSONCONFIG_H
#define JSONCONFIG_H
/*
 * ============================================================================
 *  Configuration Subsystem Design Overview
 * ============================================================================
 *
 *  This configuration system is designed with a SINGLE COMMIT POINT.
 *
 *  Key design goals:
 *    - All configuration changes, regardless of source, MUST go through
 *      the same commit path.
 *    - Storage is written in exactly ONE place.
 *    - Validation, change notification, audit and restart semantics
 *      are applied consistently.
 *
 *  ----------------------------------------------------------------------------
 *  Core Design Rules (IMPORTANT):
 *
 *  1. cfg_commit_node() is the ONLY function allowed to make configuration
 *     changes effective.
 *
 *     - It is the single place where:
 *         * storage is modified
 *         * validators are invoked
 *         * on_change / audit hooks are triggered
 *         * restart flags are updated
 *         * rollback is handled on failure
 *
 *  2. from_json() is the ONLY function allowed to write into item->storage.
 *
 *     - from_json() MUST assume:
 *         * the JsonNode type already matches cfg_item->type
 *     - from_json() MUST NOT:
 *         * perform validation
 *         * trigger side effects
 *         * access global state
 *
 *  3. Front-end APIs (CLI / typed setters / config file loaders)
 *     MUST NOT write storage directly.
 *
 *     They are responsible ONLY for:
 *       - locating the cfg_item
 *       - adapting input into a correctly typed JsonNode
 *       - passing the node to cfg_commit_node()
 *
 *  ----------------------------------------------------------------------------
 *  Data Flow Summary:
 *
 *      CLI (string input)
 *          -> parse string to JsonNode
 *          -> cfg_commit_node()
 *
 *      Typed API (int / bool / double / string)
 *          -> wrap value into JsonNode
 *          -> cfg_commit_node()
 *
 *      Config file / RPC / REST
 *          -> JsonNode
 *          -> cfg_commit_node()
 *
 *  Any code path that bypasses cfg_commit_node() is considered a BUG.
 *
 *
 *  ----------------------------------------------------------------------------
 * NOTE ON GET APIS:
 *
 * Unlike commit APIs, read APIs are intentionally NOT unified
 * into a single entry point, as they have no side effects.
 *
 * Do NOT attempt to mirror commit semantics onto read paths.
 *
 * ============================================================================
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include <stdio.h>

/* ---------- types ---------- */

typedef enum {
    CFG_INT,
    CFG_BOOL,
    CFG_DOUBLE,
    CFG_STRING
} cfg_type_t;

typedef enum {
    CFG_FLAG_NONE    = 0,
    CFG_FLAG_RUNTIME = 1 << 0,   /* CLI / runtime 可修改 */
    CFG_FLAG_RESTART = 1 << 1    /* 需重启生效 */
} cfg_flag_t;

typedef int (*cfg_on_change_fn)(
    const void *oldv,
    const void *newv
);

typedef struct cfg_item {
    const char *key;
    const char *desc;
    cfg_type_t  type;
    void       *storage;

    int (*from_json)(JsonNode *, void *); // must implement
    int (*to_json)(const void *, JsonNode *); // optional
    int (*validator)(void *);
    cfg_on_change_fn on_change;

    unsigned flags;
    struct cfg_item *next;
} cfg_item_t;


#define CFG_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#define CFG_ASSERT_INT_STORAGE(storage) \
    CFG_STATIC_ASSERT(sizeof(*(storage)) == sizeof(int), \
                      "CFG_INT storage must be int")
#define CFG_ASSERT_BOOL_STORAGE(storage) \
    CFG_STATIC_ASSERT(sizeof(*(storage)) == sizeof(gboolean), \
                      "CFG_BOOL storage must be bool")
#define CFG_ASSERT_DOUBLE_STORAGE(storage) \
    CFG_STATIC_ASSERT(sizeof(*(storage)) == sizeof(double), \
                      "CFG_DOUBLE storage must be double")
#define CFG_ASSERT_STRING_STORAGE(storage) \
    CFG_STATIC_ASSERT(sizeof(*(storage)) == sizeof(char *), \
                      "CFG_STRING storage must be string")



/* ---- Basic cfg item declarations ---- */

#define CFG_INT_ITEM(_key_, _storage_, _desc_, _flags_)        \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_INT,                         \
        .flags = (_flags_),                       \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_int,          \
    }

#define CFG_BOOL_ITEM(_key_, _storage_, _desc_, _flags_)       \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_BOOL,                        \
        .flags = (_flags_),                        \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_bool,         \
    }

#define CFG_DOUBLE_ITEM(_key_, _storage_, _desc_, _flags_)     \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_DOUBLE,                      \
        .flags = (_flags_),                        \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_double,       \
    }

#define CFG_STRING_ITEM(_key_, _storage_, _desc_, _flags_)     \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_STRING,                      \
        .flags = (_flags_),                        \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_string,       \
    }

#define CFG_INT_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)        \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_INT,                         \
        .flags = (_flags_),                       \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_int,          \
        .validator = valid_fun,          \
    }

#define CFG_BOOL_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)       \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_BOOL,                        \
        .flags = (_flags_),                        \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_bool,         \
        .validator = valid_fun,          \
    }

#define CFG_DOUBLE_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)     \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_DOUBLE,                      \
        .flags = (_flags_),                        \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_double,       \
        .validator = valid_fun,          \
    }

#define CFG_STRING_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)     \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = CFG_STRING,                      \
        .flags = (_flags_),                        \
        .storage = (_storage_),                    \
        .from_json = cfg_from_json_string,       \
        .validator = valid_fun,          \
    }

/* ---------- type handlers ---------- */

int cfg_from_json_int(JsonNode *n, void *p);
int cfg_to_json_int(const void *, JsonNode *);
int cfg_from_json_bool(JsonNode *n, void *p);
int cfg_to_json_bool(const void *, JsonNode *);
int cfg_from_json_double(JsonNode *n, void *p);
int cfg_to_json_double(const void *, JsonNode *);
int cfg_from_json_string(JsonNode *n, void *p);
int cfg_to_json_string(const void *, JsonNode *);

/*
 * cfg_read_node
 *
 * Read the current effective value of a configuration item
 * and represent it as a JsonNode.
 *
 * This function is READ-ONLY and has NO SIDE EFFECTS.
 *
 * The returned JsonNode:
 *   - has JSON_NODE_VALUE type
 *   - has a value type strictly matching cfg_item->type
 *
 * Caller owns the returned JsonNode and must free it
 * with json_node_free().
 *
 * Returns:
 *   0        success
 *  -ENOENT  key not found
 *  -EINVAL  internal type mismatch
 */
int cfg_read_node(const char *key, JsonNode **out);
/* ---------- lifecycle ---------- */

void cfg_system_init(void);
void cfg_register(cfg_item_t *item);

/* ---------- load / save ---------- */

int cfg_load_file(const char *path);
int cfg_save_file(const char *path);

/* ---------- show ----------  */
int cfg_show_all_json(FILE *out);
void cfg_show_all(FILE *out);
int cfg_show_to_buffer(char **out);
void cfg_show_status(FILE *out);
void cfg_history_show(FILE *out);

/* ---------- typed get (not used by CLI ) ----------
 * Typed read APIs
 *
 * Read the current effective value of a configuration item.
 *
 * These functions:
 *   - are read-only
 *   - do not trigger any side effects
 *   - return a snapshot of the current value
 *
 * The caller is responsible for using the correct typed API.
 */

int cfg_read_int(const char *key, int *out);
int cfg_read_bool(const char *key, gboolean *out);
int cfg_read_double(const char *key, double *out);
int cfg_read_string(const char *key, const char **out);

/* ---------- typed set (not used by CLI) ---------- */

int cfg_commit_int(const char *key, int value);
int cfg_commit_bool(const char *key, gboolean value);
int cfg_commit_double(const char *key, double value);
int cfg_commit_string(const char *key, const char *value);

/* ---------- CLI commit ---------- */
int cfg_cli_commit(const char *key, const char *value);
char *cfg_cli_read(const char *key);
/* ---------- run for command line---------- */
void cfg_cli_daemon_run(char *listen_name);
void cfg_cli_client_run(char *server_name);

#endif
