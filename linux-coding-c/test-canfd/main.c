#include <string.h>
#include "canfd_parse.h"
#include "canfd_log.h"

int main(int argc, char** argv)
{
    char buf[1000]={0};
    // canfd_init();

    // while(0) {
    //     canfd_msg_parse(buf);
    //     sleep(1);
    // }

    // canfd_uninit();
    avl_test();

    return 0;
}
