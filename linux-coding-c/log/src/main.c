#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "log_out.h"

int main(int argc, char ** argv)
{
    int i = 0;

    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("usage: %s [options]\noptions are:\n%s\n",argv[0], log_get_help_info());
        return 0;
    }
    log_init(argc, argv);
    log_info("log_version: %s\n", log_get_version());

    while(1){
        log_emerg("%s, log_emerg\n", argv[0]);
        log_device("log_device %d\n", i++);
        // log_system("log_system\n");
        log_err("log_err\n");
        log_info("log_info\n");
        // log_verbose("log_verbose\n");
        log_debug("log_debug\n");
        log_debug_v1("log_debug_v1\n");
        log_debug_v2("log_debug_v2\n");
        log_debug_vv("log_debug_vv\n\n");
        sleep(1);
    }

    return 0;
}

// This function is used to run the logall example
int main1(int argc, char **argv)
{
    return logall_run(argc, argv);
}
