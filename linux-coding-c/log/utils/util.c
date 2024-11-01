/*
 * Utility library or Lowlevel interfaces
 *
 * Get FIFO help with: man 2 open, man 7 fifo
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/types.h>          // SIGxxxx types
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdarg.h>
#include <linux/watchdog.h>

#include "common.h"
#include "log.h"

#define tag "Util"

void msleep(int msec)
{
	int sec = msec / 1000;
	int usec = (msec % 1000) * 1000;

	if (sec > 0)
		sleep(sec);
	if (usec > 0)
		usleep(usec);
}

int my_atoi(char * s)
{
	int num = 0;

	// skip blanks
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	while (*s >= '0' && *s <= '9') {
		num *= 10;
		num += (int)(*s - '0');
		s++;
	}

	return num;
}

int my_atox(char * s)
{
	int t, num = 0;

	// skip blanks
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	while (1) {
		if (*s >= '0' && *s <= '9')
			t = *s - '0';
		else if (*s >= 'A' && *s <= 'F')
			t = *s - 'A' + 10;
		else if (*s >= 'a' && *s <= 'f')
			t = *s - 'a' + 10;
		else
			break;
		num *= 16;
		num += t;
		s++;
	}

	return num;
}

long get_time_ms(void)
{
	struct timespec ts;
	long ms;

	memset(&ts, 0, sizeof(ts));
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	ms = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);

	return ms;
}

long get_realtime_sec(void)
{
	struct timespec ts;
	long sec;

	memset(&ts, 0, sizeof(ts));
	clock_gettime(CLOCK_REALTIME, &ts);
	sec = (ts.tv_sec);

	return sec;
}

long get_timestamp_start(void)
{
	static long start = 0;

	if (start == 0)
		start = get_time_ms();

	return start;
}

pid_t my_gettid(void)
{
	return syscall(SYS_gettid);
}

unsigned int be2local32(unsigned int be_value)
{
	unsigned int ret = 0;

	ret |= ((be_value >> 24) & 0xff);
	ret |= ((be_value >> 8) & 0xff00);
	ret |= ((be_value << 8) & 0xff0000);
	ret |= ((be_value << 24) & 0xff000000);

	return ret;
}

unsigned int local2be32(unsigned int local_value)
{
	return be2local32(local_value);
}

int read_file(const char * filename, char * buf, int buf_size, int non_block)
{
	int fd, ret;
	int flags = O_RDONLY | (non_block ? O_NONBLOCK : 0);

	fd = open(filename, flags);
	if (fd < 0) {
		if (errno == EAGAIN) // Resource temporarily unavailable, Try Again!
			log_info("Open file failed: %s, Try Again (%d)\n", filename, errno);
		else
			log_err("Open file failed: %s, err = %d (%s)\n", filename, errno, strerror(errno));
		return fd;
	}

	memset(buf, 0, buf_size);
	ret = read(fd, buf, buf_size);
	if (ret < 0) {
		if (errno == EAGAIN) // Resource temporarily unavailable, Try Again!
			log_info("Read file failed: %s, Try Again (%d)\n", filename, errno);
		else
			log_err("Read file failed: %s, err = %d (%s)\n", filename, errno, strerror(errno));
	}

	close(fd);
	return ret;
}

void clear_fifo_buffer(const char * filename)
{
	int ret, count = 1024 * 1024 * 1024 * 1;  // 1GB should be enough
	char buf[1024];

	while (count > 0) {
		ret = read_file(filename, buf, sizeof(buf), 1);
		if (ret <= (int)sizeof(buf))
			break;
		count -= ret;
	}
}

int write_raw_data(const char * filename, const void * data, int data_len, int non_block, int may_create)
{
	int fd, ret;
	int flags = O_RDWR | (non_block ? O_NONBLOCK : 0);

	if (may_create)
		fd = open(filename, flags | O_CREAT, 0644);
	else
		fd = open(filename, flags);
	if (fd < 0) {
		if (errno == EAGAIN) // Resource temporarily unavailable, Try Again!
			log_info("Open file failed: %s, Try Again (%d)\n", filename, errno);
		else
			log_err("Open file failed: %s, err = %d (%s)\n", filename, errno, strerror(errno));
		return fd;
	}

	ret = write(fd, data, data_len);
	if (ret < 0) {
		if (errno == EAGAIN) // Resource temporarily unavailable, Try Again!
			log_info("Write file failed: %s, Try Again (%d)\n", filename, errno);
		else
			log_err("Write file failed: %s, err = %d (%s)\n", filename, errno, strerror(errno));
	}

	fsync(fd);
	close(fd);

	return ret;
}

int write_file(const char * filename, const char * data, int non_block)
{
	return write_raw_data(filename, (const void *)data, strlen(data), non_block, 0);
}

int write_file_quiet(const char * filename, const void * data, int data_len, int non_block, int may_create)
{
	int fd, ret;
	int flags = O_RDWR | (non_block ? O_NONBLOCK : 0);

	if (may_create)
		fd = open(filename, flags | O_CREAT, 0644);
	else
		fd = open(filename, flags);
	if (fd < 0)
		return fd;

	ret = write(fd, data, data_len);
	if (ret < 0 && errno == EAGAIN) // Resource temporarily unavailable, Try Again!
		ret = write(fd, data, data_len);

	fsync(fd);
	close(fd);

	return ret;
}

int remove_file(const char * filename)
{
	int ret;
	ret = unlink(filename);
	return ret;
}


size_t get_file_size(const char * filename, int quiet)
{
	struct stat s;
	int ret;

	ret = stat(filename, &s);
	if (ret < 0) {
		if (!quiet)
			log_err("Stat file failed: %s, err = %d (%s)\n", filename, errno, strerror(errno));
		return ret;
	}

	return s.st_size;
}

/*int ll_send_cmd_common(const char * filename, char * buf, int non_block)
{
	int retry = 3, ret, size = strlen(buf);

	if (factory_test_mode()) {
		int nl = 1;
		if (size >= 1 && buf[size - 1] == '\n')
			nl = 0;
		log_verbose("Factory Test Mode: Low Level command discarded: %s%s", buf, (nl ? "\n" : ""));
		return 0;
	}

	log_debug("%s: Execute low level command: %s", tag, buf);
	do {
		ret = write_file(filename, buf, non_block);
		retry--;
	} while (retry > 0 && ret < size);
	if (ret <= 0) {
		log_err("%s: Execute low level command failed: %s", tag, buf);
	}

	return ret;
}*/

// This function will place an '\0' in the @end_token location
// so, the @buf will be changed.
char * get_token_string(char * buf, const char * start_token, const char * end_token, char ** new_ptr)
{
	if (!buf)
		return NULL;

	char * start = strstr(buf, start_token);
	if (!start)
		return NULL;
	start += strlen(start_token);

	char * end = strstr(start, end_token);
	if (end) {
		*end = '\0';
		if (new_ptr)
			*new_ptr = end + 1;
	} else {
		if (new_ptr)
			*new_ptr = start + 1;
	}

	return start;
}

void trim_spaces(char * buf)
{
	char * s, * e;

	if (!buf)
		return;

	s = buf;
	while (*s) {
		if (*s == ' ' || *s == '\n' ||
			*s == '\r' || *s == '\t')
			s++;
		else
			break;
	}

	e = s + strlen(s) - 1;
	while (*e && e > s) {
		if (*e == ' ' || *e == '\n' ||
			*e == '\r' || *e == '\t')
			e--;
		else
			break;
	}

	e++;
	*e = '\0';

	if (buf == s)
		return;

	while (1) {
		*buf = *s;
		if (!*buf)
			break;
		buf++;
		s++;
	}
}

/*
 * CRC32 Algorithm, has the same result as Ubuntu command `crc32'
 * Codes ported from https://github.com/Michaelangel007/crc32/blob/master/src/crc32.cpp
 */
static unsigned int * crc32_init(void)
{
	static const unsigned int CRC32_REVERSE = 0xEDB88320;
	static unsigned int CRC32_TABLE[256] = {0};
	static int init_done = 0;
	int byte;
	char bit;

	if (init_done)
		return CRC32_TABLE;

	for (byte = 0; byte < 256; byte++) {
		unsigned int crc = (unsigned int) byte; // reflected form
		for (bit = 0; bit < 8; bit++) {
			if (crc & 1)
				crc = (crc >> 1) ^ CRC32_REVERSE; // reflected form
			else
				crc = (crc >> 1);
		}

		CRC32_TABLE[ byte ] = crc;
	}
	init_done = 1;

	if (CRC32_TABLE[128] != CRC32_REVERSE) {
		log_err("%s: ERROR: CRC32 Table not initialized properly!\n", tag);
		init_done = 0;
	}

	return CRC32_TABLE;
}

static unsigned int crc32_table_reflect(const unsigned char *buf, int len)
{
	unsigned int * table = crc32_init();
	unsigned int crc = -1;

	while (len-- > 0)
		crc = table[ (crc ^ *buf++) & 0xFF ] ^ (crc >> 8); // reflect: >> 8

	return ~crc;
}

unsigned int crc32(const unsigned char * buf, int len)
{
	unsigned int ret;

	if (len < 1)
		return 0;

	log_debug_vv("%s: crc32: buf: %d bytes: %02x %02x ... %02x\n", tag, len, buf[0], buf[1], buf[len - 1]);
	ret = crc32_table_reflect(buf, len);
	log_debug_vv("%s: crc32: buf: %d bytes: %02x %02x ... %02x, crc32 = 0x%08x\n", tag, len, buf[0], buf[1], buf[len - 1], ret);

	return ret;
}

int crc32_size(void)
{
	return 4;
}

/*
 * socket server & client routines
 */
struct service_client_private_struct {
	int sock;
	void * private_data;
	int (* client_thread_fn)(int sock, void * private_data);
};

static void * service_client_thread_fn(void * data)
{
	struct service_client_private_struct * client_tmp;

	pthread_detach(pthread_self()); // Need NO pthread_join()

	client_tmp = (struct service_client_private_struct *)data;
	if (client_tmp) {
		if (client_tmp->client_thread_fn)
			client_tmp->client_thread_fn(client_tmp->sock, client_tmp->private_data);
		close(client_tmp->sock);
		free(client_tmp);
	}

	return NULL;
}

/*
 * Wait clients to connect in.
 *
 * @client_thread_fn: callback function which will be called when new client connected
 *                    running in a seperated thread
 *
 * Note that, @client_thread_fn() should not close the ->sock file descriptor, which will be closed
 * in thread function service_client_thread_fn().
 */
void service_waiting_for_clients(int listen_sock, int (* client_thread_fn)(int sock, void * private_data), void * private_data,
		const char * private_tag)
{
	union sockaddr_ext {
		struct sockaddr_in sk_in;
		struct sockaddr_un sk_un;
	};
	int client_sock, ret;
	union sockaddr_ext sock_clt;
	socklen_t len = sizeof(sock_clt);
	pthread_t client_thread;
	struct service_client_private_struct * client_tmp;

	if (!private_tag)
		private_tag = tag;

	while (1) {
		// log_debug_v2("%s: waiting for incoming connections...\n", private_tag);
		len = sizeof(sock_clt);
		client_sock = accept(listen_sock, (struct sockaddr *)&sock_clt, &len);
		if (client_sock < 0 && errno == EINTR)
			continue;
		if (client_sock < 0 && errno == EAGAIN)
			continue;
		if (client_sock < 0) {
			log_err("%s: accept reurn error: %d (%s)\n", private_tag, errno, strerror(errno));
			return;
		}

		client_tmp = (struct service_client_private_struct *)malloc(sizeof(*client_tmp));
		if (!client_tmp) {
			log_err("%s: out of memory: service_client_private_struct! Kill the service!\n", private_tag);
			close(client_sock);
			break; // Quit service
		}
		client_tmp->sock = client_sock;
		client_tmp->private_data = private_data;
		client_tmp->client_thread_fn = client_thread_fn;
		ret = pthread_create(&client_thread, NULL, service_client_thread_fn, (void *)client_tmp);
		if (ret != 0) {
			log_err("%s: create client thread failed: %d\n", private_tag, ret);
			free(client_tmp);
			close(client_sock);
		}
	}
}

int service_sock_open(int port, const char * private_tag)
{
	int sock_fd, ret;
	struct sockaddr_in service_addr;
	int sockaddr_len;
	int enable_sockopt = 1;

	if (!private_tag)
		private_tag = tag;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		log_err("%s: service socket create failed, port = %d, error = %d (%s)\n", private_tag, port, errno, strerror(errno));
		return -1;
	}
	enable_sockopt = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable_sockopt, sizeof(enable_sockopt));
	enable_sockopt = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable_sockopt, sizeof(enable_sockopt));

	service_addr.sin_family = AF_INET;
	service_addr.sin_addr.s_addr = INADDR_ANY;
	service_addr.sin_port = htons(port);
	sockaddr_len = sizeof(service_addr);

	ret = bind(sock_fd, (struct sockaddr *)&service_addr, sockaddr_len);
	if (ret < 0) {
		log_err("%s: service socket bind failed, port = %d, error = %d (%s)\n", private_tag, port, errno, strerror(errno));
		close(sock_fd);
		return -1;
	}

	ret = listen(sock_fd, 10);
	if (ret < 0) {
		log_err("%s: service socket listen failed, port = %d, error = %d (%s)\n", private_tag, port, errno, strerror(errno));
		close(sock_fd);
		return -1;
	}

	log_info("%s: Service Started (sock = %d, port = %d) ......\n", private_tag, sock_fd, port);

	return sock_fd;
}

void service_sock_close(int sock, const char * private_tag)
{
	if (!private_tag)
		private_tag = tag;

	if (sock >= 0)
		close(sock);
}

int service_try_connect_self(int port, const char * private_tag)
{
	int sock_fd;
	struct sockaddr_in service_addr;
	int sockaddr_len;

	if (!private_tag)
		private_tag = tag;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		log_err("%s: client socket create failed, error = %d (%s)\n", private_tag, errno, strerror(errno));
		return -1;
	}

	service_addr.sin_family = AF_INET;
	service_addr.sin_addr.s_addr = INADDR_ANY;
	service_addr.sin_port = htons(port);
	sockaddr_len = sizeof(service_addr);
	int ret = connect(sock_fd, (struct sockaddr*)&service_addr, sockaddr_len);
	if (ret < 0) {
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

static void setup_unix_socket(struct sockaddr_un * addr, const char * socket_name, const int socket_name_len)
{
	size_t len = socket_name_len;

	if (!addr)
		return;

	if (len > sizeof(addr->sun_path))
		len = sizeof(addr->sun_path);

	addr->sun_family = AF_UNIX;
	memset(addr->sun_path, 0, sizeof(addr->sun_path));
	memcpy(addr->sun_path, socket_name, len);
}

const char * get_printable_unix_socket_name(const char * socket_name)
{
	if (socket_name[0] == 0)
		return socket_name + 1;

	return socket_name;
}

int service_unix_sock_open(const char * socket_name, const int socket_name_len)
{
	int sock_fd, ret;
	struct sockaddr_un service_addr = {0};

	log_info("%s: service starting...\n", tag);
	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		log_err("%s: socket create failed, path = \'\\0%s\', error = %d (%s)\n", tag,
				get_printable_unix_socket_name(socket_name), errno, strerror(errno));
		return -1;
	}

	setup_unix_socket(&service_addr, socket_name, socket_name_len);
	ret = bind(sock_fd, (struct sockaddr *)&service_addr, sizeof(service_addr));
	if (ret < 0) {
		log_err("%s: socket bind failed, path = \'\\0%s\', error = %d (%s)\n", tag,
				get_printable_unix_socket_name(socket_name), errno, strerror(errno));
		close(sock_fd);
		return -1;
	}

	ret = listen(sock_fd, 10);
	if (ret < 0) {
		log_err("%s: socket listen failed, path = \'\\0%s\', error = %d (%s)\n", tag,
				get_printable_unix_socket_name(socket_name), errno, strerror(errno));
		close(sock_fd);
		return -1;
	}

	log_info("%s: Service Started (sock = %d, path = \'\\0%s\') ......\n", tag, sock_fd,
			get_printable_unix_socket_name(socket_name));

	return sock_fd;
}

void service_unix_sock_close(int sock)
{
	if (sock >= 0)
		close(sock);
}

int service_unix_try_to_connect(const char * name, const int name_len)
{
	struct sockaddr_un service_addr;
	int sockaddr_len;
	int sock_fd;

	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		log_err("%s: client socket create failed, error = %d (%s)\n", tag, errno, strerror(errno));
		return -1;
	}

	setup_unix_socket(&service_addr, name, name_len);
	sockaddr_len = sizeof(service_addr);
	int ret = connect(sock_fd, (struct sockaddr*)&service_addr, sockaddr_len);
	if (ret < 0) {
		log_debug_v2("%s: connect failed: \'\\0%s\', error: %d (%s)!\n", tag,
				get_printable_unix_socket_name(name), errno, strerror(errno));
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

static int __wdt_open(void)
{
	const char * filename = "/dev/watchdog";
	static int fd = -1;

	if (fd >= 0)
		return fd;

	fd = open(filename, O_RDWR);
	if (fd < 0 && errno == EAGAIN) {
		sleep(1);
		fd = open(filename, O_RDWR);
	}
	if (fd < 0) {
		if (errno == EAGAIN) // Resource temporarily unavailable, Try Again!
			log_info("%s: Open file failed: %s, Try Again (%d)\n", tag, filename, errno);
		else
			log_err("%s: Open file failed: %s, err = %d (%s)\n", tag, filename, errno, strerror(errno));
		return fd;
	}

	return fd;
}

int wdt_start_timeout(int timeout)
{
	int fd, ret;

	if (timeout < 1) {
		log_err("%s: %s(), timeout invalid: %d\n", tag, __func__, timeout);
		return -1;
	}

	fd = __wdt_open();
	if (fd < 0)
		return fd;

	ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);

	if (ret < 0) {
		log_err("%s: %s(), ioctl err = %d (%s)\n", tag, __func__, errno, strerror(errno));
	} else {
		log_emerg("%s: Software Watchdog Started [ %d seconds ]\n", tag, timeout);
	}

	return ret;
}

int wdt_start(void)
{
	int timeout = 60;
	return wdt_start_timeout(timeout);
}

int wdt_stop(void)
{
	int fd, option = WDIOS_DISABLECARD;
	int ret;

	fd = __wdt_open();
	if (fd < 0)
		return fd;

	ret = ioctl(fd, WDIOC_SETOPTIONS, &option);

	if (ret < 0) {
		log_err("%s: %s(), ioctl err = %d (%s)\n", tag, __func__, errno, strerror(errno));
	} else {
		log_emerg("%s: Software Watchdog Stopped!\n", tag);
	}

	return ret;
}

int wdt_feed(void)
{
	char food[] = "food";
	int fd, ret;

	fd = __wdt_open();
	if (fd < 0)
		return fd;

	ret = write(fd, food, sizeof(food));

	if (ret < 0) {
		log_err("%s: %s(), write err = %d (%s)\n", tag, __func__, errno, strerror(errno));
	} else {
		log_debug("%s: Software Watchdog Feeded!\n", tag);
	}

	return ret;
}

void safely_quit(const char * reason, int need_watchdog_fire)
{
	const char * quit_tag = "Safely Quit";

	if (logcat_mode())
		goto quit_program;

	log_emerg("%s: reason: %s!\n", quit_tag, reason ? reason : "<null>");

	if (need_watchdog_fire) {
		log_info("%s: Watchdog Fire!\n", quit_tag);
		// wdt_start();
	} else {
		log_info("%s: Stop the Hardware Watchdog...\n", quit_tag);
		// wdt_stop();
	}

	log_info("%s: Stop the Threads...\n", quit_tag);

quit_program:
	log_info("%s: exiting...\n\n", quit_tag);
	exit(0);
}
