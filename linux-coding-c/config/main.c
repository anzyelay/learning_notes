#include "jsonconfig.h"
#include <stdio.h>

/* actual config variables */
static int g_port = 8080;
static int8_t g_data= 80;
static gboolean g_debug = TRUE;
static char *g_log_path = "/tmpsadafsdfasd";  // to test initial non-NULL value, to verify that old value is freed and replaced by new value when commit_string is called.
static char g_ip[16]="192.168.200.2"; // to test string storage with fixed buffer, which will ignore by cfg system
static float g_version = 1.012f;

CFG_ASSERT_INT_STORAGE(&g_port);
CFG_ASSERT_BOOL_STORAGE(&g_debug);
/* config items */
static cfg_item_t cfg_table[] = {
    CFG_INT_ITEM(
        "server.port"
        , g_port
        , "the port to listen on"
        , CFG_FLAG_RUNTIME
    ),
    CFG_INT_ITEM(
        "server.data"
        , g_data
        , "the data to use"
        , CFG_FLAG_RUNTIME
    ),
    CFG_DOUBLE_ITEM(
        "log.version"
        , g_version
        , "the version of the log"
        , CFG_FLAG_NONE
    ),
    CFG_BOOL_ITEM(
        "server.debug"
        , g_debug
        , "enable debug mode"
        , CFG_FLAG_RUNTIME
    ),
    CFG_STRING_ITEM(
        "log.path"
        , g_log_path
        , "the path to the log file"
        , CFG_FLAG_RESTART | CFG_FLAG_RUNTIME
    ),
    CFG_STRING_ITEM("server.ip", g_ip, "the IP address to bind to", CFG_FLAG_RUNTIME),
};

#define TEST_JSON_STR "{ \"server\": { \"port\": 19090, \"debug\": true, \"path\": \"/tmp/test\" }, \"log\": { \"path\": \"/tmp/log/test\" } }"

int main(int argc, char **argv)
{
    char *progname = g_path_get_basename(argv[0]);
    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        cfg_cli_client_run(progname);
        return 0;
    }


    cfg_system_init();

    // g_log_path = g_strdup("/tmp/default.log");

    for (int i=0; i < sizeof(cfg_table)/sizeof(cfg_item_t); i++) {
        if (cfg_register(&cfg_table[i])) {
            printf("Failed to register config item: %s\n", cfg_table[i].key);
        }
    }

    if (cfg_load_file("/tmp/config.json")) {
        printf("Failed to load config file\n");
        return -1;
    }

    char * ret = cfg_cli_read("server.port");
    if (ret) {
        printf("server.port = %s\n", ret);
        g_free(ret);
    }

    int tmp = g_port;
    // g_port = 3030;
    printf("commit server.port to 3030\n");
    cfg_commit_int("server.port", 3030);
    ret = cfg_cli_read("server.port");
    if (ret) {
        printf("server.port = %s\n", ret);
        g_free(ret);
    }

    printf("commit server.port back to %d\n", tmp);
    cfg_commit_int("server.port", tmp);

    int64_t port;
    gboolean debug;
    const gchar *log_path;
    cfg_read_int("server.port", &port);
    printf("server.port = %ld\n", port);
    cfg_read_bool("server.debug", &debug);
    printf("server.debug = %s\n", debug ? "true" : "false");
    cfg_read_string("log.path", &log_path);
    printf("log.path = %s\n", log_path);
    log_path = "dfasfds";
    log_path = cfg_cli_read("log.path");
    printf("log.path = %s\n", log_path);
    if (log_path) {
        g_free((gchar *)log_path);
    }

    cfg_generate_to_json_data("server", &ret);
    if (ret) {
        printf("server config in json format:\n%s\n", ret);
        g_free(ret);
    }

    cfg_generate_to_json_data(NULL, &ret);
    if (ret) {
        printf("all config in json format:\n%s\n", ret);
        g_free(ret);
    }

    printf("after parsing json:\n");
    cfg_parse_json_to_vars(TEST_JSON_STR, (cfg_parse_map_t[]) {
        { "server.port", CFG_INT, &port },
        { "server.debug", CFG_BOOL, &debug },
        { "log.path", CFG_STRING, &log_path },
    }, 3);

    printf("port = %ld\n", port);
    printf("debug = %s\n", debug ? "true" : "false");
    printf("log_path = %s\n", log_path);


    if (1) {
        cfg_cli_server_run_loop(progname);
    }
    else {

        cfg_cli_server_run(progname);

        sleep(5);

        cfg_cli_server_stop();

    }

    return 0;
}
