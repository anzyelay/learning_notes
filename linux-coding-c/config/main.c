#include "jsonconfig.h"
#include <stdio.h>

/* actual config variables */
static int g_port = 8080;
static int8_t g_data= 80;
static gboolean g_debug = TRUE;
static char *g_log_path = "/tmpsadafsdfasd";  // to test initial non-NULL value, to verify that old value is freed and replaced by new value when commit_string is called.
static char g_ip[16]="192.168.200.2"; // to test string storage with fixed buffer, which will ignore by cfg system
static double g_version = 1.012;
static float g_float_data = 1.11;

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
        , CFG_FLAG_RUNTIME | CFG_FLAG_TEMPORARY
    ),
    CFG_DOUBLE_ITEM(
        "server.float_data"
        , g_float_data
        , "some float data for test"
        , CFG_FLAG_RUNTIME
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


void test_log(unsigned int level, const char *func, int line, const char *fmt, ...)
{
    char buf[1024] = {0};
    va_list args;
    va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
    printf("[%d:%s:%d]: %s", level, func, line, buf);
}


int main(int argc, char **argv)
{
    char *progname = g_path_get_basename(argv[0]);
    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        cfg_cli_client_run(progname);
        return 0;
    }

    cfg_set_log_hook(test_log);
    cfg_system_init();

    // g_log_path = g_strdup("/tmp/default.log");

    for (u_int64_t i=0; i < sizeof(cfg_table)/sizeof(cfg_item_t); i++) {
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
    if (!cfg_read_string("log.path", &log_path)) {
        printf("log.path = %s\n", log_path);
        g_free((gchar *)log_path);
        log_path = NULL;
    }

    log_path = "dfasfds";
    log_path = cfg_cli_read("log.path");
    printf("log.path = %s\n", log_path);
    if (log_path) {
        g_free((gchar *)log_path);
        log_path = NULL;
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
    cfg_parse_json_to_vars(TEST_JSON_STR, (cfg_item_t[]) {
        CFG_INT_ITEM("server.port", port, NULL, CFG_FLAG_RUNTIME),
        CFG_BOOL_ITEM("server.debug", debug, NULL, CFG_FLAG_RUNTIME),
        CFG_STRING_ITEM("log.path", log_path, NULL, CFG_FLAG_RUNTIME),
     }, 3);

    printf("port = %ld\n", port);
    printf("debug = %s\n", debug ? "true" : "false");
    printf("log_path = %s\n", log_path);


    if (1) {
        cfg_cli_server_run(progname, FALSE);
    }
    else {

        cfg_cli_server_run(progname, TRUE);

        sleep(5);

        cfg_cli_server_stop();

    }

    return 0;
}
