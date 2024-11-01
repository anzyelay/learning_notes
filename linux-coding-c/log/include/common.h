#ifndef COMMON_HEADER
#define COMMON_HEADER

#include <unistd.h>
#include <errno.h>
#include "log.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;
typedef unsigned int    uint;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;

#define set_single_called(return_statement) do { static int __called = 0; if (__called) return_statement; __called = 1; } while (0)

long get_time_ms(void);
long get_realtime_sec(void);
long get_timestamp_start(void);
void msleep(int msec);
int my_atoi(char * s);
int my_atox(char * s);
pid_t my_gettid(void);
int remove_file(const char * filename);
int read_file(const char * filename, char * buf, int buf_size, int non_block);
int write_file(const char * filename, const char * data, int non_block);
int write_raw_data(const char * filename, const void * data, int data_len, int non_block, int may_create);
int write_file_quiet(const char * filename, const void * data, int data_len, int non_block, int may_create);
size_t get_file_size(const char * filename, int quiet);

int service_sock_open(int port, const char * private_tag);
void service_sock_close(int sock, const char * private_tag);
void service_waiting_for_clients(int listen_sock, int (* client_thread_fn)(int sock, void * private_data), void * private_data,
		const char * private_tag);
int service_try_connect_self(int port, const char * private_tag);
int service_unix_try_to_connect(const char * name, const int name_len);
int service_unix_sock_open(const char * socket_name, const int socket_name_len);
void service_unix_sock_close(int sock);
const char * get_printable_unix_socket_name(const char * socket_name);

void setup_signals(void);
void safely_quit(const char * reason, int need_watchdog_fire);

#endif
