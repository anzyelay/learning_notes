#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>

#define tag "UART Protocl"
/**
 * @struct  uart_msgbuff
 * @brief   UART frame struct
 * @details
 * @note
 * @attention
 */
#define offset_of(mem, type)    ((uintptr_t)&((type *)0)->mem)
#pragma pack(1)
typedef struct __UART_MSG_BUFFER__
{
    uint8_t sof; // Message sof type
    uint16_t len; // real data length + header.id + header.subid + crc
    struct __HEADER__
    {
        uint8_t id;     // Message service id
        uint8_t sub_id; // Message sub service id
    } header;
    uint8_t data[255-2]; // Data bytes of the FlexCAN message
    // uint8_t XOR_data;// XOR value
} uart_msgbuff;
#pragma pack()

enum MSG_ESCAPE_CHAR_TYPE {
    E_MSG_ESCAPE_CHAR1 = 0xda,
    E_MSG_ESCAPE_CHAR2 = 0xdc,
    E_MSG_ESCAPE_CHAR3 = 0xdd
};

enum MSG_SOF_TYPE {
    E_MSG_SOF_A53_TO_MCU = 0X80,                 //0x80:A53->MCU
    E_MSG_SOF_MCU_TO_A53 = 0X60                  //0x60:MCU->A53
};

static int find_index_of_sof(const char *pbuf, int size)
{
	const char *pd = pbuf;
	const char * const pend = pbuf + size;
	do {
		if (*pd == E_MSG_SOF_MCU_TO_A53) {
			return (int)(pd - pbuf);
		}
	} while (++pd < pend);
	return -1;
}

static uint8_t get_crc(void *buf, int len)
{
	uint8_t xor_chk = 0;
	uint8_t *pbuf = (uint8_t *)buf;
	for (int i = 0; i < len; i++){
		xor_chk = xor_chk ^ pbuf[i];
	}
	return xor_chk;
}

int remove_escape_char(char *pdata, int data_len)
{
	char *old = pdata;
	char *new = pdata;
	char *end = pdata + data_len;

	for (; old != end; old++, new++ ) {
		if (*old == ESCAPE_CHAR) {
			if (old + 1 == end) {
				log_warn("%s: the escape char is in the end of data! it should not occur!\n", tag);
				return -1;
			}
			old++;
			char next_char = *old;
			switch (next_char) {
				case E_MSG_ESCAPE_CHAR1:
					*old = E_MSG_SOF_MCU_TO_A53;
					break;
				case E_MSG_ESCAPE_CHAR2:
					*old = E_MSG_SOF_A53_TO_MCU;
					break;
				case E_MSG_ESCAPE_CHAR3:
					*old = ESCAPE_CHAR;
					break;
				default:
					log_warn("%s: escape char is wrong (%x, %x)!\n", tag, *(old-1), *old);
					return -1;
			}
		}
		if (new != old)
			*new = *old;
	}

	return (int)(old-new);
}

int add_escape_char(uart_msgbuff *uart_msg)
{
	int k = 0;
	uint32_t len = 0;
	char *buf = NULL;
	int escape_char_num = 0;

	len = uart_msg->len - UART_FIXED_LENGTH;
	// 考虑可能存在转义字符，最大长度为2*len
	buf = malloc(2 * len);
	if (NULL == buf){
		log_err("%s: buf malloc allocate failed\n", tag);
		return 1;
	}
	memset(buf, 0, 2 * len);

	// 转义消息体中的0x60、0x80 或 0xdb字符
	for(int i = 0; i < len; i++) {
		switch (uart_msg->data[i]) {
			case E_MSG_SOF_MCU_TO_A53:
				escape_char_num++;
				buf[k++] = ESCAPE_CHAR;
				buf[k++] = E_MSG_ESCAPE_CHAR1;
				break;
			case E_MSG_SOF_A53_TO_MCU:
				escape_char_num++;
				buf[k++] = ESCAPE_CHAR;
				buf[k++] = E_MSG_ESCAPE_CHAR2;
				break;
			case ESCAPE_CHAR:
				escape_char_num++;
				buf[k++] = ESCAPE_CHAR;
				buf[k++] = E_MSG_ESCAPE_CHAR3;
				break;
			default:
				buf[k++] = uart_msg->data[i];
				break;
		}
	}
	if (uart_msg->header.sub_id == E_SUB_SERVICE_ID_CANFD) {
		buf[OFFSET_OF_CANFD_LEN] += escape_char_num;
	}
	memcpy(uart_msg->data, buf, k);
	uart_msg->len += escape_char_num;
	free(buf);
	buf = NULL;
	return 0;
}
void *rpmsg_recv_proc(void *args)
{
	int iret;
	fd_set fdset;
	uint8_t buffer[512] = {0};
	int pkg_len = 0;
	int recv_len = 0;
	queue_buf ul_msg;
	int read_failed_flag = 0;

	while (!get_exit_flag() && get_rpmsg_fd() > 0 && ul_queue != NULL)
	{
		struct timeval timeout = {3, 0}; // timeout 3s
		FD_ZERO(&fdset);
		FD_SET(rcdev.fd, &fdset);
		iret = select(rcdev.fd + 1, &fdset, NULL, NULL, &timeout);
		if (-1 == iret)
		{
			log_warn("%s: failed to select rpmsg fd\n", tag);
			break;
		}
		else if (0 == iret)    // no data in Rx buffer
		{
			log_verbose("%s: rx: no data\n", tag);
		}
		else    // data is in Rx data
		{
			if (FD_ISSET(rcdev.fd, &fdset))
			{
				iret = read(rcdev.fd, buffer + recv_len, sizeof(buffer) - recv_len);
				if (iret <= 0)
				{
					if (!read_failed_flag) {
						log_warn("%s: can't read data from rpmsg.\n", tag);
						read_failed_flag = 1;
					}
					continue;
				}
				else {
					read_failed_flag = 0;
				}

				recv_len += iret;
				if (recv_len < offset_of(data, uart_msgbuff))
				{
					log_warn("%s: data length is not enough from rpmsg.\n", tag);
					continue;
				}

				iret = find_index_of_sof((char *)buffer, recv_len);
				if (iret < 0) {
					log_warn("%s: the sof is not found, throw the whole data.\n", tag);
					recv_len = 0;
					continue;
				}
				else if (iret > 0) {
					log_warn("%s: the sof is at %ds byte, throw the data before, and move sof to the first.\n", tag, iret);
					recv_len -= iret;
					memmove(buffer, buffer + iret, recv_len);
					if (recv_len < offset_of(data, uart_msgbuff)) {
						log_warn("%s: data length is not enough after moving, continue recv.\n", tag);
						continue;
					}
				}

				uart_msgbuff *pkg = (uart_msgbuff *)buffer;
				pkg->len = ntohs(pkg->len);
				pkg_len = pkg->len + offset_of(header, uart_msgbuff);

				if (pkg_len > sizeof(uart_msgbuff) || pkg_len <= offset_of(data, uart_msgbuff)) {
					log_warn("%s: the pkg len(%d) is out of range, it is wrong data. throw it!\n", tag, pkg_len);
					recv_len = 0;
					continue;
				}

				if (pkg_len > recv_len) {
					log_warn("%s: the data has recieved %d [total:%d].\n", tag, recv_len, pkg_len);
					continue;
				}
				uint8_t xor_crc = get_crc(&pkg->len, pkg_len - sizeof(pkg->sof) - 1);
				if (xor_crc != buffer[pkg_len - 1]){
					log_warn("%s: crc error, recv data from mcu is wrong, throw these.\n", tag);
					recv_len = 0;
					continue;
				}

				int escape_char_num = remove_escape_char(pkg->data, pkg->len);
				if (escape_char_num < 0) {
					log_warn("%s: escape char wrong, throw this data.\n", tag);
					recv_len = 0;
					continue;
				}
				else if (escape_char_num > 0)
				{
					pkg->len -= escape_char_num;
					recv_len -= escape_char_num;
				}

				pkg_len = pkg->len + offset_of(header, uart_msgbuff);

				pkg = (uart_msgbuff *)malloc(pkg_len);
				if (NULL == pkg)
				{
					log_warn("%s: pkg malloc allocate failed\n", tag);
					continue;
				}
				memcpy(pkg, buffer, pkg_len);

				ul_msg.msg_len = pkg_len;
				ul_msg.msg_buf = pkg;
				push_simple_queue(ul_queue, &ul_msg);

				recv_len -= pkg_len;
				if (recv_len > 0)
					memmove(buffer, buffer + pkg_len, recv_len);
			}
			else {
				log_warn("%s: not rpmsg fd is trigger\n", tag);
			}
		}
	}
	log_info("%s: exit recv msg from mcu thread \n", tag);
	return (void *)1;
}
