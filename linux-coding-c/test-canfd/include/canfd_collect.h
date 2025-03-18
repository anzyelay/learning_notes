#ifndef __CANFD_COLLECT_H__
#define __CANFD_COLLECT_H__

#include "canfd_parse.h"

#define CAN_PATH "/etc/CanData.json"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define SUBSCRIBE_CANFD_PKT 1

int canfd_init();

int canfd_uninit();

canfd_struct *get_canfd();

int handle_can_ivi1_id(canfd_packet_struct *can_pkt);

int handle_can_ivi2_id(canfd_packet_struct *can_pkt);

int handle_can_all_id(canfd_packet_struct *can_pkt);

int canfd_msg_parse(canfd_msgbuff *buffer);

#endif
