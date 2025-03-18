#ifndef __CANFD_PARSE_H__
#define __CANFD_PARSE_H__

#include <stdbool.h>
#include <stdint.h>
#include "canfd_packet.h"

#pragma pack(1)
typedef struct __canfd_msgbuff
{
    uint32_t canfd_id; // Canfd ID
    uint8_t canfd_len; // Length of data in Canfd message
    uint8_t data[64];  // Data bytes of the Canfd message, range form 0~64
} canfd_msgbuff;
#pragma pack()

typedef struct __canfd_struct {
	int num; // Canfd 报文的数量
	avl_tree_t *canfd_tree;
} canfd_struct;

canfd_packet_struct *canfd_search_pkt_from_id(canfd_struct *canfd, uint32_t canfd_id);

canfd_packet_struct *canfd_search_dup2_from_id(canfd_struct *canfd, uint32_t canfd_id, canfd_packet_struct *pout);

canfd_packet_struct *canfd_search_pkt_from_name(canfd_struct *canfd, const char *name);

signal_struct *canfd_search_sig_from_name(canfd_struct *canfd, const char *name);

signal_struct *canfd_search_sig_from_pkt(canfd_packet_struct *canfd_pkt , const char *name);

int canfd_load_config(canfd_struct *canfd, const char *config_path);

canfd_struct *canfd_create();

int canfd_destroy(canfd_struct *canfd);

// 解析canfd数据，主要是车机实时信息和车机登录信息
int canfd_parse(canfd_struct *canfd, canfd_msgbuff *buffer);

void canfd_set_handle_for_package(canfd_struct *canfd, canfd_pkt_changed_cb handle);

void canfd_set_handle_for_signal(canfd_struct *canfd, canfd_sig_changed_cb handle);

bool canfd_signal_subscribe(canfd_struct *canfd, const char *signame,
									canfd_sig_changed_cb fun, void *user_data);

bool canfd_package_subscribe(canfd_struct *canfd, uint32_t id, canfd_pkt_changed_cb fun);

bool canfd_packet_set_sigval(canfd_packet_struct *pkt, const char *name, int value);

#endif
