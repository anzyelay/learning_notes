/*
 * Log logcat service -- Shujun 2023
 *
 * Get the log from remote...
 * 管理客户端连接并提供日志的写入和读取功能
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"
#include "log.h"
#include "pipe.h"

#define tag                   "Logcat Service"
#define LOGCAT_SERVICE_PORT   2112	// 日志服务端口号
#define LOGCAT_CLIENTS_NR     16	// 最大客户端数量

#define LOG_PIPE_SIZE         (1024 * 1024 * 2)	// 日志管道的大小
static int log_logcat_ready = 0;
static pipe_t logcat_pipe, * logcat_clients_pipe[LOGCAT_CLIENTS_NR];

#define EARLY_LOG_SIZE        (1024 * 128)
static char cached_early_log_buf[EARLY_LOG_SIZE];// 早期日志缓冲区大小（128KB）
static int  cached_early_log_bytes = 0;

static pthread_mutex_t logcat_service_mutex;
static pthread_mutexattr_t logcat_service_mutex_attr;
#define logcat_service_mutex_init() do { \
	pthread_mutexattr_settype(&logcat_service_mutex_attr, PTHREAD_MUTEX_RECURSIVE); \
	pthread_mutex_init(&logcat_service_mutex, &logcat_service_mutex_attr); } while (0)
#define logcat_service_mutex_destroy() do { pthread_mutex_destroy(&logcat_service_mutex); } while (0)
#define logcat_service_mutex_lock() do { pthread_mutex_lock(&logcat_service_mutex); } while (0)
#define logcat_service_mutex_unlock() do { pthread_mutex_unlock(&logcat_service_mutex); } while(0)

/*
 * Logcat clients manager
 * 初始化客户端管道数组
 */
static void logcat_clients_init()
{
	memset(logcat_clients_pipe, 0, sizeof(logcat_clients_pipe));
	logcat_service_mutex_init();
}

// 释放所有客户端管道，清理资源
static void logcat_clients_exit()
{
	pipe_t * p;
	int i;

	logcat_service_mutex_lock();
	for (i = 0; i < LOGCAT_CLIENTS_NR; i++) {
		if (!logcat_clients_pipe[i])
			continue;
		p = logcat_clients_pipe[i];
		logcat_clients_pipe[i] = NULL;
		pipe_close(p);
		free(p);
	}
	logcat_service_mutex_unlock();
}

// 为新客户端分配管道，检查当前连接的客户端数量以防止超出限制
static pipe_t * logcat_clients_get_new_pipe()
{
	pipe_t * p;
	int i, ret, ok = 0;

	p = (pipe_t *)malloc(sizeof(*p));
	if (!p) {
		log_err("%s: Out of memory to new a logcat client pipe.\n", tag);
		return p;
	}
	pipe_init(p);
	ret = pipe_open(p, LOG_PIPE_SIZE, 0);
	if (ret != PIPE_ERR_OK) {
		log_err("%s: Out of memory to open a logcat client pipe.\n", tag);
		goto fail_out;
	}
	pipe_set_loop_overwrite(p, 1);

	logcat_service_mutex_lock();
	for (i = 0; i < LOGCAT_CLIENTS_NR; i++) {
		if (logcat_clients_pipe[i])
			continue;
		logcat_clients_pipe[i] = p;
		pipe_clone(&logcat_pipe, p);
		ok = 1;
		break;
	}
	logcat_service_mutex_unlock();
	if (ok)
		return p;
	log_err("%s: logcat client pipe create failed: too many clients (%d)\n", tag, LOGCAT_CLIENTS_NR);

fail_out:
	pipe_close(p);
	free(p);
	return NULL;
}

// 释放特定客户端的管道资源
static void logcat_clients_free_pipe(pipe_t * p)
{
	int i, found = 0;

	if (!p)
		return;

	logcat_service_mutex_lock();
	for (i = 0; i < LOGCAT_CLIENTS_NR; i++) {
		if (logcat_clients_pipe[i] != p)
			continue;

		logcat_clients_pipe[i] = NULL;
		found = 1;
		break;
	}
	logcat_service_mutex_unlock();

	if (!found) {
		log_err("%s: logcat client free failed: No such client: %p\n", tag, p);
		return;
	}

	pipe_close(p);
	free(p);
}

/*
 * Send log to logcat service
 * 初始化日志服务的管道，准备供写入使用，并处理早期日志的缓存
 */
static void log_logcat_service_init(void)
{
	int ret;

	set_single_called(return);

	// Initial logcat pipe
	pipe_init(&logcat_pipe);
	ret = pipe_open(&logcat_pipe, LOG_PIPE_SIZE, 0);
	if (ret != PIPE_ERR_OK) {
		log_err("%s: Out of memory for log logcat service init\n", tag);
		goto try_get_early_log;
	}
	pipe_set_loop_overwrite(&logcat_pipe, 1);
	log_logcat_ready = 1;

try_get_early_log:
	if (log_logcat_ready)
		pipe_write(&logcat_pipe, cached_early_log_buf, cached_early_log_bytes);
	cached_early_log_bytes = -1;  // Stop cached early log buffer
}

// 向管道写入日志数据
static void inject_to_pipe(pipe_t * p, const char * extra_prefix, const char * log, int log_bytes)
{
	if (!p)
		return;
	pipe_write(p, extra_prefix, strlen(extra_prefix));
	pipe_write(p, log, log_bytes);
}

// 将日志注入到主管道和所有客户端管道中
static void inject_to_logcat_pipes(unsigned int request_level, const char * extra_prefix, const char * log, int log_bytes)
{
	int i;

	logcat_service_mutex_lock();
	inject_to_pipe(&logcat_pipe, extra_prefix, log, log_bytes);
	for (i = 0; i < LOGCAT_CLIENTS_NR; i++) {
		if (!logcat_clients_pipe[i])
			continue; // logcat clients pipe may be free, so it break down the continueos
		inject_to_pipe(logcat_clients_pipe[i], extra_prefix, log, log_bytes);
	}
	logcat_service_mutex_unlock();
}

// 将日志写入准备好的管道，处理早期日志的缓存
void log_logcat_inject(unsigned int request_level, const char * log, int log_bytes)
{
	const char * extra_tag = log_get_extra_prefix(request_level);
	char extra_prefix[32];

	sprintf(extra_prefix, "<%s>", extra_tag);

	if (cached_early_log_bytes >= 0) {
		int extra_prefix_len, log_len, len;

		extra_prefix_len = strlen(extra_prefix);
		log_len = log_bytes;
		len = cached_early_log_bytes + extra_prefix_len + log_len;

		if (len < EARLY_LOG_SIZE) {
			strcpy(cached_early_log_buf + cached_early_log_bytes, extra_prefix);
			cached_early_log_bytes += extra_prefix_len;
			strncpy(cached_early_log_buf + cached_early_log_bytes, log, log_len);
			cached_early_log_bytes += log_len;
		}
	}

	if (log_logcat_ready)
		inject_to_logcat_pipes(request_level, extra_prefix, log, log_bytes);
}

// 清除日志，读取并丢弃当前管道中的所有数据
void log_logcat_clear(void)
{
	char buf[256];
	int bytes;

	log_logcat_service_init();

	logcat_service_mutex_lock();
	while (1) {
		bytes = pipe_read(&logcat_pipe, buf, sizeof(buf), 0, 0);
		if (bytes <= 0)
			break;
	}
	logcat_service_mutex_unlock();
}

static int server_logcat_to_logall(void)
{
	pipe_t * pipe;
	char buf[1024];
	int ret, bytes;

	pthread_detach(pthread_self()); // Need NO pthread_join()

	pipe = logcat_clients_get_new_pipe();
	if (!pipe)
		return -1;
	// log_debug_v2("%s: new client connected, socket: %d\n", tag, client_sock);
	int local_socket = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in logall_addr;
	logall_addr.sin_family = AF_INET;
	logall_addr.sin_port = htons(LOGCAT_LOGALL_SERVICE_PORT);
	logall_addr.sin_addr.s_addr = INADDR_ANY;

	while(!log_get_exit_flag()) {
		bytes = pipe_read(pipe, buf, sizeof(buf), 0, 0);
		if (bytes < 0)
			break;
		if (bytes > 0) {
			ret = sendto(local_socket, buf, bytes, NULL, &logall_addr, sizeof(logall_addr));
			if (ret < 0) {
				msleep(100);
				log_debug_vv("%s: send to logall service failed, %d bytes, error = %d (%s)\n", tag, bytes, errno, strerro
(errno));
			}
		} else {
			msleep(100);
		}
	}
	close(local_socket);
	logcat_clients_free_pipe(pipe);
	return 0;
}
/*
 * Logcat clients process
 * Each client has a thread
 * 每当有新的客户端连接时，该函数会被调用。负责读取管道数据并将其发送到客户端
 */
static int serve_logcat_client(int client_sock, void * private_data)
{
	pipe_t * pipe;
	char buf[1024];
	int ret, bytes;

	pipe = logcat_clients_get_new_pipe();
	if (!pipe)
		return -1;
	log_debug_v2("%s: new client connected, socket: %d\n", tag, client_sock);

	while(1) {
		bytes = pipe_read(pipe, buf, sizeof(buf), 0, 0);
		if (bytes < 0)
			break;
		if (bytes > 0) {
			ret = write(client_sock, buf, bytes);
			if (ret < 0) {
				log_debug_vv("%s: send to client failed, %d bytes, error = %d (%s)\n", tag, bytes, errno, strerror(errno));
				break;
			}
		} else {
			msleep(100);
		}
	}

	logcat_clients_free_pipe(pipe);
	return 0;
}

// 检查服务是否已经存在并启动日志服务，若失败则记录错误并返回
static int do_logcat_service()
{
	int listen_sock;
	int tmp;

#if CONFIG_LOGCAT_ONLY_LOCAL
	// Check the service if it already exist
	tmp = service_unix_try_to_connect(log_service_name, sizeof(log_service_name));
	if (tmp >= 0) {
		service_unix_sock_close(tmp);
		log_emerg("%s: logcat service already exist.\n", tag);
		exit(EXIT_SUCCESS);
		return -1;
	}

	// Start the Service
	listen_sock = service_unix_sock_open(log_service_name, sizeof(log_service_name));
#else
	// Check the service if it already exist
	tmp = service_try_connect_self(LOGCAT_SERVICE_PORT, tag);
	if (tmp >= 0) {
		service_sock_close(tmp, tag);
		log_emerg("%s: logcat service already exist.\n", tag);
		return -1;
	}

	// Start the Service
	listen_sock = service_sock_open(LOGCAT_SERVICE_PORT, tag);
#endif

	if (listen_sock < 0) {
		log_err("%s: service start failed\n", tag);
		return listen_sock;
	}

	if (!has_log_logall_service) {
		pthread_t log2logall_thread;
		int ret = pthread_create(&log2logall_thread, NULL, server_logcat_to_logall, NULL);
		if (ret != 0) {
			log_err("%s: create log_to_all thread failed: %d\n", tag, ret);
		}
	}
	service_waiting_for_clients(listen_sock, serve_logcat_client, NULL, tag);
	service_sock_close(listen_sock, tag);

	return 0;
}

static void * log_logcat_service_thread(void *param)
{
	int restart_seconds = 3;

	pthread_detach(pthread_self()); // Need NO pthread_join()
	logcat_clients_init();
	while (1) {
		int ret = 0;
		ret = do_logcat_service();
		log_info("%s: service exited (ret = %d), restart after %d seconds!\n", tag, ret, restart_seconds);
		if (ret < 0)
			break;
		sleep(restart_seconds);
	}
	logcat_clients_exit();

	return NULL;
}

static pthread_t log_logcat_service_threadid;

void start_log_logcat_service(void)
{
	int ret;

	set_single_called(return);

	log_logcat_service_init();
	ret = pthread_create(&log_logcat_service_threadid, NULL, log_logcat_service_thread, NULL);
	if (ret != 0)
		log_err("%s: create log logcat service thread failed: %d\n", tag, ret);
}
