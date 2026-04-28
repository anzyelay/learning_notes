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
#include <stdarg.h>

/* log hook struct definition */
typedef void (*log_hook_t)(unsigned int level, const char *func, int line, const char *fmt, va_list ap);
void cfg_set_log_hook(log_hook_t handle);

/* ---------- types ---------- */

typedef enum {
    CFG_BOOL,
    CFG_INT8,
    CFG_INT16,
    CFG_INT32,
    CFG_INT64,
    CFG_FLOAT,
    CFG_DOUBLE,
    CFG_STRING, // it recommends using char* (pointer to a string) for storage, and the CFG system will manage the memory. The caller should not modify or free the string directly. you'd better keep storage be NULL and not assignment a non-heap-allocated string to initial the storage as CFG will free the storage before give a new string, and also the storage should not be a fixed-size array
    CFG_INT_AUTO, // auto-detect int type based on storage size, must be used with caution as it may lead to unexpected behavior if storage type is not standard int sizes
    CFG_TYPE_NUMBER
} cfg_type_t;

typedef enum {
    CFG_FLAG_NONE    = 0,
    CFG_FLAG_RUNTIME = 1 << 0,   /* CLI / runtime: Can be runtime overrride */
    CFG_FLAG_RESTART = 1 << 1,   /* Indicating changed value is applied after restart. it just a indicate flag but the coder is responsible to do this function*/
    CFG_FLAG_TEMPORARY = 1 << 2    /* This parameter will be reset to default (coded) after reboot, won't be saved and loaded from file */
} cfg_flag_t;

typedef int (*cfg_on_change_fn)(
    const void *oldv,
    const void *newv
);

typedef struct cfg_item {
    const char *key; // unique key, e.g. "server.port", used for lookup and CLI commands
    const char *desc; // description, can be used for CLI help text
    cfg_type_t  type; // type of the config item
    void       *storage; // pointer to the actual variable that holds the value
    int         storage_size; // size of the storage type, used for validation

    int (*from_json)(JsonNode *, struct cfg_item *); // must implement
    int (*to_json)(const struct cfg_item *, JsonNode *); // optional
    int (*validator)(void *); // optional, validate the new value before commit, return 0 if valid, -1 if invalid
    cfg_on_change_fn on_change; // optional, called after value is changed, with old and new value

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
#define CFG_ITEM_VC(_type_, _key_, _storage_, _desc_, _flags_, _from_json_, _valid_fun_, _on_change_)        \
    {                                            \
        .key = (_key_),                            \
        .desc = (_desc_),                          \
        .type = (_type_),                         \
        .flags = (_flags_),                       \
        .storage = &(_storage_),                    \
        .storage_size = sizeof(typeof(_storage_)),       \
        .from_json = (_from_json_),          \
        .validator = _valid_fun_,          \
        .on_change = _on_change_,          \
        .next = NULL                            \
    }

#define CFG_ITEM_V(_type_, _key_, _storage_, _desc_, _flags_, _from_json_, valid_fun)        \
    CFG_ITEM_VC(_type_, _key_, _storage_, _desc_, _flags_, _from_json_, valid_fun, NULL)

#define CFG_ITEM(_type_, _key_, _storage_, _desc_, _flags_, _from_json_)        \
    CFG_ITEM_V(_type_, _key_, _storage_, _desc_, _flags_, _from_json_, NULL)

#define CFG_INT_AUTO_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)        \
    CFG_ITEM_V(CFG_INT_AUTO, _key_, _storage_, _desc_, _flags_, cfg_from_json_int, valid_fun)

#define CFG_BOOL_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)       \
    CFG_ITEM_V(CFG_BOOL, _key_, _storage_, _desc_, _flags_, cfg_from_json_bool, valid_fun)

#define CFG_FLOAT_ITEM_V(_key_, _storage_, _desc_, _flags_)     \
    CFG_ITEM_V(CFG_FLOAT, _key_, _storage_, _desc_, _flags_, cfg_from_json_double, valid_fun)

#define CFG_DOUBLE_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)     \
    CFG_ITEM_V(CFG_DOUBLE, _key_, _storage_, _desc_, _flags_, cfg_from_json_double, valid_fun)

#define CFG_STRING_ITEM_V(_key_, _storage_, _desc_, _flags_, valid_fun)     \
    CFG_ITEM_V(CFG_STRING, _key_, _storage_, _desc_, _flags_, cfg_from_json_string, valid_fun)

#define CFG_INT_ITEM(_key_, _storage_, _desc_, _flags_)        \
    CFG_INT_AUTO_ITEM_V(_key_, _storage_, _desc_, _flags_, NULL)

#define CFG_BOOL_ITEM(_key_, _storage_, _desc_, _flags_)       \
    CFG_BOOL_ITEM_V(_key_, _storage_, _desc_, _flags_, NULL)

#define CFG_FLOAT_ITEM(_key_, _storage_, _desc_, _flags_)     \
    CFG_FLOAT_ITEM_V(_key_, _storage_, _desc_, _flags_, NULL)

#define CFG_DOUBLE_ITEM(_key_, _storage_, _desc_, _flags_)     \
    CFG_DOUBLE_ITEM_V(_key_, _storage_, _desc_, _flags_, NULL)

#define CFG_STRING_ITEM(_key_, _storage_, _desc_, _flags_)     \
    CFG_STRING_ITEM_V(_key_, _storage_, _desc_, _flags_, NULL)


/* ---------- type handlers ---------- */

int cfg_from_json_int(JsonNode *n, struct cfg_item *p);
int cfg_to_json_int(const struct cfg_item *p, JsonNode *n);
int cfg_from_json_bool(JsonNode *n, struct cfg_item *p);
int cfg_to_json_bool(const struct cfg_item *p, JsonNode *n);
int cfg_from_json_double(JsonNode *n, struct cfg_item *p);
int cfg_to_json_double(const struct cfg_item *p, JsonNode *n);
int cfg_from_json_string(JsonNode *n, struct cfg_item *p);
int cfg_to_json_string(const struct cfg_item *p, JsonNode *n);

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

// Initialize the configuration system. This function is idempotent and thread-safe.
// It must be called before any other cfg_* functions are used.
// This function will initialize internal data structures, start background threads,
// and perform any necessary setup for the configuration system to operate correctly.
void cfg_system_init(void);

/**
 * @brief : Register a configuration item. This function should be called during application initialization
 *          to register all available configuration items before loading any configuration values.

 * @param item : the CFG SYSTEM only focus on the registered item. Unregistered items will be ignored
 * @return int : 0 on success, -1 on failure
 *
 * NOTE:
 * 1. The caller is responsible for ensuring that the cfg_item_t structure remains valid for the lifetime
 *   of the application, as the configuration system will reference it directly.
 *
 * 2. The register function will check the storage size and type consistency,
 *    - For CFG_INT_AUTO/CFG_DOUBLE type, it will auto-detect the int type based on storage size,
 *      and set the item type to the detected int type. If the storage size does
 *      not match any standard int sizes, it will return an error.
 *
 *    - For CFG_STRING type, if the initial storage is non-NULL, it will be duplicated and
 *      the CFG system will take ownership of the duplicated string. It is recommended
 *      to initialize string storage to NULL before registration to avoid potential issues,
 *      but if a non-NULL initial value is provided, the register function will handle it
 *      by duplicating the string and managing its memory. The caller should not modify or
 *      free the initial string after registration, as it is the caller's responsibility
 *      to manage the original pointer, while the CFG system will manage the duplicated string in storage.
 */
int cfg_register(cfg_item_t *item);

/* ---------- load / save ---------- */

/**
 * @brief load configuration from a JSON file. The JSON structure should match the expected format for the registered configuration items.
 *        The function will read the file, parse the JSON, and for each registered configuration item,
 *        it will look up the corresponding value in the JSON and update the configuration accordingly.
 *
 * @param path : the file path to load from. If the file does not exist or cannot be read, an error will be returned.
 * @return int : 0 on success, -1 on failure (e.g. file not found, JSON parse error, type mismatch)
 */
int cfg_load_file(const char *path);
/**
 * @brief cfg_save_file saves the current registered items configuration to a file.
 *
 * @param path : the file path to save to. If NULL, it will save to the last loaded file path.
 * @param force : if TRUE, it will save even if there are no changes since the last save. If FALSE, it will skip saving if there are no changes.
 * @return int : 0 on success, -1 on failure (e.g. file write error)
 */
int cfg_save_file(const char *path, gboolean force);

/* ---------- show ----------  */
int cfg_show_all_json(FILE *out);
void cfg_show_all(FILE *out);
void cfg_show_status(FILE *out);
void cfg_history_show(FILE *out);
/**
 * @brief : Generate a JSON string representing the current configuration, optionally filtered by a key prefix.
 *
 * @param prefix : if not NULL or empty, only include configuration items whose keys start with this prefix. If NULL or empty, include all items.
 * @param out : Output pointer, will be set to a newly allocated string containing the JSON data. Caller must free it with g_free().
 * @return int : 0 on failure, or the length of the generated JSON string on success.
 */
int cfg_generate_to_json_data(const char *prefix, char **out);

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

int cfg_read_int8(const char *key, gint8 *out);
int cfg_read_int16(const char *key, gint16 *out);
int cfg_read_int32(const char *key, gint32 *out);
int cfg_read_int64(const char *key, gint64 *out);
int cfg_read_int(const char *key, gint64 *out);
int cfg_read_bool(const char *key, gboolean *out);
int cfg_read_float(const char *key, float *out);
int cfg_read_double(const char *key, double *out);
// For string, the caller is responsible for freeing the returned string with g_free() when no longer needed
int cfg_read_string(const char *key, const char **out);

/* ---------- typed set (not used by CLI) ---------- */

int cfg_commit_int(const char *key, gint64 value);
int cfg_commit_bool(const char *key, gboolean value);
int cfg_commit_double(const char *key, double value);
int cfg_commit_string(const char *key, const char *value);

/* ---------- CLI commit ---------- */
// cfg_cli_commit: commit a string value from CLI, with parsing and validation
// returns 0 on success, -1 on failure (e.g. key not found, type mismatch, validation failure)
int cfg_cli_commit(const char *key, const char *value);

// for CLI read, returns a newly allocated string representation of the value
// caller must free the returned string with g_free()
char *cfg_cli_read(const char *key);

/* ---------- run for command line----------
 These functions are for demonstration purposes and are NOT required for the core configuration system.
*/

/**
 * @brief Start the CLI server in a separate thread. The server will listen for incoming connections and handle CLI commands.
 *
 * @param server_name : the name to listen on, which will be used by CLI clients to connect.
 * @param in_thread : whether to run the server in a separate thread
 * Note: if in_thread is FALSE, this function will block and run the server loop in the current thread. If TRUE, it will start a new thread for the server and return immediately.
 */
void cfg_cli_server_run(char *server_name, gboolean in_thread);
// This function will connect to the server and run a simple CLI loop, allowing the user to input commands.
void cfg_cli_client_run(char *server_name);
// This function will stop the CLI server loop and clean up resources.
void cfg_cli_server_stop(void);

/* ------------ a tool for parsing JSON  ---------------*/
/**
 * @brief Parse JSON string and populate configuration variables.
 *
 * @param json_str The JSON string to parse.
 * @param items An array of cfg_item_t that defines the mapping between JSON keys and configuration variables.
 * @param item_size The size of each cfg_item_t in the array.
 * @return int 0 on success, -1 on failure.
 * NOTE: This function is a utility for quickly populating configuration variables from a JSON string,
 *       and is NOT part of the core configuration system.
 */
int cfg_parse_json_to_vars(const char *json_str, cfg_item_t *items, size_t item_size);

#endif
