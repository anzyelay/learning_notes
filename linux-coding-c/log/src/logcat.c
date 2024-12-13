/*
 * logcat client -- Shujun, 2023
 *
 * Get the log from remote...
 * 用于从远程获取日志的客户端程序，连接日志服务并处理日志数据
 * 主要包括连接日志服务、接收和解析日志信息、以及将日志信息存储和打印
 * 主要功能是作为一个日志客户端，通过 TCP 连接到一个远程日志服务，实时获取和解析日志数据。
 * 它支持通过特定的端口和主机配置连接设置，并能处理日志信息的格式与级别。
 * 关键的功能包括日志的缓冲处理、错误处理、以及从文件中直接读取日志
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>

#include "parse_cmdline.h"
#include "common.h"
#include "log.h"
#include "pipe.h"
#include "log_out.h"

#define tag                  "Logcat client"
#define LOGCAT_SERVICE_PORT   2112		// default logcat service port
static int logcat_mode_en = 0;			// logcat mode 是否启用
static int logcat_server_port = 0;		// 连接的日志服务的主机
static const char * logcat_server_host = NULL;	// 连接的日志服务的端口

static char cached_log[1024 * 256];		// 缓存接收到的日志数据
static int cached_log_bytes = 0;		// 当前缓存的日志数据长度
static unsigned int loglevel = _LOG_LEVEL_INFO;	// 当前的日志级别

int logcat_clear_logs = 0;				// 清除logcat日志缓存标志
char * logcat_cat_log_file = NULL;     // cat logs from a log file, instead of the logcat service
char log_service_name[128] = {0};

static void sock_close(int sock)
{
	close(sock);
}

// 连接到指定host和port的日志服务
static int connect_logcat_service(const char * host, int port)
{
	struct addrinfo *res, *ainfo, hints;
	char service[6];
	int sock = -1;
	int err = 0;

	if (!host)
		host = "127.0.0.1";
	if (port == 0)
		port = LOGCAT_SERVICE_PORT;
	log_debug("%s: connecting to tcp/%s:%d...\n", tag, host, port);

	snprintf(service, 6, "%u", port);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(host, service, &hints, &res)) != 0) {
		log_err("%s: getaddrinfo() returns error: %d\n", tag, err);
		return -1;
	}
	ainfo = res;

	while (ainfo) {
		if ((sock = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol)) >= 0) {
			err = connect(sock, ainfo->ai_addr, ainfo->ai_addrlen);
			if (err == 0)
				break;
			else
				close(sock);
		} else {
			log_err("%s: socket failed, error = %d (%s)\n", tag, errno, strerror(errno));
		}
		ainfo = ainfo->ai_next;
	}

	if (res)
		freeaddrinfo(res);

	if (err == -1) {
		if (sock >= 0)
			close(sock);
		return -1;
	}

#if 0
	{
	struct timeval recv_timeout;
	recv_timeout.tv_sec = 3; // seconds
	recv_timeout.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&recv_timeout, sizeof(recv_timeout));
	}
#endif

	log_info("%s: logcat service connected.\n", tag);

	return sock;
}

// 禁用日志前缀和时间戳
static void logcat_set_verbose(void)
{
	extern unsigned int log_verbose_level;

	log_verbose_level &= (~_LOG_LEVEL_PREFIX);
	log_verbose_level &= (~_LOG_LEVEL_TIMESTAMP);

	log_to_kmsg_allowed = 0;
	has_log_file_service = 0;
	has_log_logcat_service = 0;
}

// 从日志字符串中解析出日志级别
static char * get_loglevel_from_prefix(char * s, unsigned int * level)
{
	unsigned int lvl = *level;
	char * old_s = s;

	if (!s || *s != '<')
		return s;

	s++;
	switch (*s) {
		case 'I': lvl = _LOG_LEVEL_INFO;     break;
		case 'X': lvl = _LOG_LEVEL_EMERG;    break;
		case 'E': lvl = _LOG_LEVEL_ERR;      break;
		case 'W': lvl = _LOG_LEVEL_WARN;     break;
		case 'V': lvl = _LOG_LEVEL_VERBOSE;  break;
		case 'D':
			switch (*(s+1)) {
				default:  lvl = _LOG_LEVEL_DEBUG;      s++; break;
				case '>': lvl = _LOG_LEVEL_DEBUG;           break;
				case '1': lvl = _LOG_LEVEL_DEBUG_V1;   s++; break;
				case '2': lvl = _LOG_LEVEL_DEBUG_V2;   s++; break;
				case 'V': lvl = _LOG_LEVEL_DEBUG_VV;   s++; break;
			}
			break;
		case 'S':
			switch (*(s+1)) {
				default:  lvl = _LOG_LEVEL_DEBUG;      s++; break;
				case '>': lvl = _LOG_LEVEL_DEBUG;           break;
				case 'D': lvl = _LOG_LEVEL_DEVICE;     s++; break;
				case 'S': lvl = _LOG_LEVEL_SYSTEM;     s++; break;
				// case 'B': lvl = _LOG_LEVEL_BT;         s++; break;
				// case 'W': lvl = _LOG_LEVEL_WIFI;       s++; break;
				// case 'L': lvl = _LOG_LEVEL_MODEM;      s++; break;
				// case 'E': lvl = _LOG_LEVEL_ESIM;       s++; break;
				// case 'U': lvl = _LOG_LEVEL_WEBUI;      s++; break;
				// case 'O': lvl = _LOG_LEVEL_FOTA;       s++; break;
			}
			break;
		default:
			return old_s;
	}

	s++;
	if (*s != '>')
		return old_s;
	s++;
	*level = lvl;

	return s;
}

// 解析日志
static void logcat_parse_log(const char * buf, int bytes)
{
	char *nl, *s = cached_log;

	if (bytes <= 0)
		return;
	if (bytes + cached_log_bytes >= (int)sizeof(cached_log) - 1)
		bytes = sizeof(cached_log) - cached_log_bytes - 1;
	if (bytes < 0)
		bytes = 0;

	// 把接收到的日志数据存储到缓存中
	memcpy(cached_log + cached_log_bytes, buf, bytes);
	cached_log_bytes += bytes;
	cached_log[cached_log_bytes] = '\0';

	// 逐行处理日志，并输出到终端
	while ((nl = strchr(s, '\n')) != NULL) {
		*nl = '\0';
		s = get_loglevel_from_prefix(s, &loglevel);
		do_log_common(loglevel, FC, LN, "%s\n", s);
		s = nl + 1;
	}

	memmove(cached_log, s, cached_log_bytes);
	cached_log_bytes -= (s - cached_log);

	if (cached_log_bytes >= (int)sizeof(cached_log) - 1) {
		log_err("%s: log buffer corrupted, clear...\n", tag);
		cached_log_bytes = 0;
	}
}

// 启用logcat mode
void set_logcat_mode()
{
	logcat_mode_en = 1;
	logcat_set_verbose();
}

// 设置logcat mode的主机
void set_logcat_mode_host(const char * host)
{
#if CONFIG_LOGCAT_ONLY_LOCAL
	printf("Warning: logcat only supports UNIX domain, not INET any more\n");
#endif
	logcat_server_host = host;
}

// 设置logcat mode的端口
void set_logcat_mode_port(int port)
{
#if CONFIG_LOGCAT_ONLY_LOCAL
	printf("Warning: logcat only supports UNIX domain, not INET any more\n");
#endif
	if (port < 0)
		port = 0;
	logcat_server_port = port;
}

int logcat_mode()
{
	return logcat_mode_en;
}

// 从指定的file中读取日志并解析
static void do_logcat_file(const char * filename)
{
	char line_buf[1024 * 30];  // Refer to do_log_common() in log.c
	FILE * fp;

	if (!filename)
		return;

	fp = fopen(filename, "r");
	if (!fp) {
		log_err("%s: File '%s' not exist.", tag, filename);
		return;
	}

	while (fgets(line_buf, sizeof(line_buf), fp)) {
		int t = strlen(line_buf);
		if (t > 0)
			logcat_parse_log(line_buf, t);
	}

	fclose(fp);
}

// 用socket连接日志服务，并读取日志数据
static void do_logcat_client(void)
{
	char buf[3072], * pbuf;
	int sock, bytes;

#if CONFIG_LOGCAT_ONLY_LOCAL
	sock = service_unix_try_to_connect(log_service_name, sizeof(log_service_name));
#else
	sock = connect_logcat_service(logcat_server_host, logcat_server_port);
#endif

	if (sock < 0) {
		log_info("%s: logcat service not ready, waiting...\n", tag);
		sleep(1);
		return;
	}
	int read_max_times = 50;
	while (read_max_times) {
		bytes = read(sock, buf, sizeof(buf));
		if (bytes < 0) {
			log_err("%s: read socket error: %d (%s), try to reconnect...\n", tag, errno, strerror(errno));
			break;
		}
		else if (bytes == 0) {
			read_max_times--;
			msleep(100);
			if (read_max_times <= 0) {
				log_err("%s: read socket timeout times: %d, try to reconnect...\n", tag, 50-read_max_times);
				break;
			}
		}
		else {
			read_max_times = 50;
		}

		pbuf = buf;
		while (bytes > 0) {
			// Convert the '\0' to "<nul>" string, or else the log will be hung by '\0'
			char nul_str[] = "<nul>";
			int t = strlen(pbuf);;
			if (t > bytes)
				t = bytes;
			if (t > 0)
				logcat_parse_log(pbuf, t);
			if (bytes > t) {
				logcat_parse_log(nul_str, strlen(nul_str));
				t++;
			}

			bytes -= t;
			pbuf += t;
		}
	}

	sock_close(sock);
}

void logcat_main(void)
{
	logcat_set_verbose();

	if (logcat_clear_logs) {
		log_logcat_clear();
		return;
	}

	if (logcat_cat_log_file) {
		do_logcat_file(logcat_cat_log_file);
		return;
	}

	// 未清除日志或者未指定日志文件的情况下，每隔100ms连接日志服务并处理日志数据
	while (1) {
		do_logcat_client();
		msleep(100);
	}
}

void set_log_service_name(const char * me)
{
	sprintf(log_service_name + 1, LOG_SERVICE_NAME_FMT, me);
	log_debug("log_service_name: %s\n", log_service_name + 1);
}

void set_log_dir(char * dir_path)
{
	if(dir_path == NULL){
		log_err("intput para can't be null\n");
		return;
	}
	log_dir = dir_path;
	log_debug("%s: set log dir: %s\n", tag, log_dir);
}

/**
 * return: 0 as a service or run in block as a client until exit
*/
int log_run()
{
	setup_signals();

	if (logcat_mode()){
		log_info("====log client====\n");
		logcat_main();
		exit(EXIT_SUCCESS);
	}

	log_info("log server!\n");
	start_log_service();
	usleep(10000);
	return 0;
}

int log_init(int argc, char *argv[], int (*p_parse_fun)(int, char**))
{
	int ret = 0;
	ret = parse_args(argc, argv);
	if (ret < 0){
		log_err("parse args in log parse failed\n");
		return -1;
	}
	if (ret==0 && p_parse_fun) {
		ret = p_parse_fun(argc, argv);
		if (ret < 0){
			log_debug_vv("stop from user parse func\n");
			return ret;
		}
	}
	return log_run();
}
