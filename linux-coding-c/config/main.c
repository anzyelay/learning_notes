#include "jsonconfig.h"
#include <stdio.h>

/* actual config variables */
static int g_port = 8080;
static gboolean g_debug = FALSE;
static char *g_log_path;
static char *g_server_path;

CFG_ASSERT_INT_STORAGE(&g_port);
CFG_ASSERT_BOOL_STORAGE(&g_debug);
/* config items */
static cfg_item_t cfg_table[] = {
    CFG_INT_ITEM(
        "server.port"
        , &g_port
        , "the port to listen on"
        , CFG_FLAG_RUNTIME
    ),
    CFG_BOOL_ITEM(
        "server.debug"
        , &g_debug
        , "enable debug mode"
        , CFG_FLAG_RUNTIME
    ),
    CFG_STRING_ITEM(
        "log.path"
        , &g_log_path
        , "the path to the log file"
        , CFG_FLAG_RESTART | CFG_FLAG_RUNTIME
    ),
    CFG_STRING_ITEM("server.path", &g_server_path, NULL, CFG_FLAG_RUNTIME),
};

int main(int argc, char **argv)
{
    char *progname = g_path_get_basename(argv[0]);
    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        cfg_cli_client_run(progname);
        return 0;
    }

    cfg_system_init();

    for (int i=0; i < sizeof(cfg_table)/sizeof(cfg_item_t); i++) {
        cfg_register(&cfg_table[i]);
    }

    cfg_load_file("/tmp/config.json");

    char * ret = cfg_cli_read("server.port");
    if (ret) {
        printf("server.port = %s\n", ret);
        g_free(ret);
    }

    // g_port = 3030;
    cfg_commit_int("server.port", 3030);

    ret = cfg_cli_read("server.port");
    if (ret) {
        printf("server.port = %s\n", ret);
        g_free(ret);
    }

    int port;
    cfg_read_int("server.port", &port);
    printf("g_port = %d\n", port);
    gboolean debug;
    cfg_read_bool("server.debug", &debug);
    printf("g_debug = %s\n", debug ? "true" : "false");
    const gchar *log_path;
    cfg_read_string("log.path", &log_path);
    printf("g_log_path = %s\n", log_path);

    cfg_generate_to_json_data("server", &ret);
    if (ret) {
        printf("current config in json format:\n%s\n", ret);
        g_free(ret);
    }

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
