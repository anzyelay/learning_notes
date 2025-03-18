#ifndef __CANFD_PACKET_H__
#define __CANFD_PACKET_H__

#include "canfd_signal.h"
#include "avl_tree.h"

#define CANFD_NAME_MAX_SIZE 64	// canfd报文名字的最大长度
#define SIGNAL_MAX_SIZE 30		// 每条can报文包含的信号量最大数量

typedef struct canfd_packet canfd_packet_struct;

typedef int (*canfd_pkt_changed_cb)(struct canfd_packet *pkt);
// canfd 报文
struct canfd_packet
{
	uint32_t canfd_id; // Canfd ID
	char name[CANFD_NAME_MAX_SIZE]; // canfd 报文名
	uint8_t dlc;
	uint8_t data[64];
	uint8_t signal_num;
	struct canfd_signal signal[SIGNAL_MAX_SIZE];
	canfd_pkt_changed_cb value_changed_cb;
	avl_node_t node;
};
bool canfd_packet_set_sigval(canfd_packet_struct *pkt, const char *name, int value);

canfd_packet_struct *canfd_packet_dup2(canfd_packet_struct *old, canfd_packet_struct *new);

bool canfd_packet_set_cbfun(canfd_packet_struct *pkt, canfd_pkt_changed_cb fun);

canfd_packet_struct *canfd_packet_new(uint32_t canfd_id, uint8_t dlc, uint8_t signal_num);

int canfd_packet_init(canfd_packet_struct *pkt, uint32_t canfd_id, uint8_t dlc, uint8_t signal_num);

bool canfd_packet_free(canfd_packet_struct *pkt);

#endif
