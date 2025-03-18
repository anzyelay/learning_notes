#ifndef __CANFD_SIGNAL_H__
#define __CANFD_SIGNAL_H__
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct canfd_packet canfd_packet_struct;

#define SIGNAL_NAME_MAX_SIZE 64 // 信号量名字的最大长度

enum SIG_DATA_TYPE {
	SIG_DATA_TYPE_BOOL,
	SIG_DATA_TYPE_UINT8,
	SIG_DATA_TYPE_UINT16,
	SIG_DATA_TYPE_UINT32,
	SIG_DATA_TYPE_UINT64,
	SIG_DATA_TYPE_POINTER
};
// 信号量
typedef struct canfd_signal signal_struct;
typedef void (*canfd_sig_changed_cb)(const struct canfd_signal * const);

typedef union sigvariant sigvariant_un;
union sigvariant
{
	bool b;
	uint8_t u8;					 // 信号量值（hex）
	uint16_t u16;					 // 信号量值（hex）
	uint32_t u32;					 // 信号量值（hex）
	uint64_t u64;					 // 信号量值（hex）
	uint8_t *pu8;
};
struct canfd_signal
{
	char name[SIGNAL_NAME_MAX_SIZE]; // 信号量名
	uint8_t start_byte;				 // 起始字节
	uint16_t start_bit;				 // 起始位
	uint16_t bit_length;			 // 位长度
	pthread_spinlock_t lock;		 // 自旋锁
	int data_len;
	enum SIG_DATA_TYPE data_type;
	sigvariant_un value;
	canfd_sig_changed_cb value_changed_cb; // 在信号量的值发生变化时被调用
	void *user_data;
	struct canfd_packet *parent;		   // 表示该信号量所属的 CAN 报文包
};

int canfd_signal_init(signal_struct *sig, const char *name,
						int start_byte, int start_bit, int bit_length);
void canfd_signal_set_parent(signal_struct *sig,  struct canfd_packet *parent);
bool canfd_signal_set_cbfun(signal_struct *sig,  canfd_sig_changed_cb fun, void *userdata);
bool canfd_signal_release(signal_struct *sig);
signal_struct *canfd_signal_new(const char *name, int start_byte,
							int start_bit, int bit_length);
void canfd_signal_free(signal_struct *sig);
bool canfd_signal_update(signal_struct *sig);
bool canfd_signal_set_value(signal_struct *sig, uint64_t value);
bool canfd_signal_set_values(signal_struct *sig, uint8_t *value, int len);
signal_struct *canfd_signal_dup2(signal_struct *sig, signal_struct *new);


#endif
