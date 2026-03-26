#include "config.h"
#include <stdio.h>

/* actual config variables */
static int g_port = 8080;
static gboolean g_debug = FALSE;
static char *g_log_path;

CFG_ASSERT_INT_STORAGE(&g_port);
CFG_ASSERT_BOOL_STORAGE(&g_debug);
/* config items */
static cfg_item_t cfg_table[] = {
    CFG_INT_ITEM("server.port", &g_port, CFG_FLAG_RUNTIME),
    CFG_BOOL_ITEM("server.debug", &g_debug, CFG_FLAG_RUNTIME),
    CFG_STRING_ITEM("log.path", &g_log_path, CFG_FLAG_RESTART | CFG_FLAG_RUNTIME),
};

int main(void)
{
    cfg_system_init();

    for (int i=0; i < sizeof(cfg_table)/sizeof(cfg_item_t); i++) {
        cfg_register(&cfg_table[i]);
    }

    cfg_load_file("./config.json");

    // g_port = 3030;
    cfg_commit_int("server.port", 3030);

    cfg_cli_run();

    return 0;
}
