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
int g_timezone_offset = 0;		// default utc time

int log_readout_once_time = 0;	// 默认的客户端读日志超时无数据会一直循环读取，此参数给来配置只读取超时几次后退出，不用一直等待
int log_no_real_time = 0;			    // 不显示系统时间戳
int logcat_clear_logs = 0;				// 清除logcat日志缓存标志
char * logcat_cat_log_file = NULL;     // cat logs from a log file, instead of the logcat service
char log_service_name[128] = {0};

extern pthread_t log_file_service_threadid;

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
	unsigned int lvl = 0;
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

static int get_utc_time_from_prefix(const char *s, struct tm *utc_tm, int *ms)
{
	if (!s || !utc_tm || !ms || *s != '[')
		return -1;

	int mm, dd, hh, min, ss;
	int ret = sscanf(s, "[%02d-%02d %02d:%02d:%02d.%03d]", &mm, &dd, &hh, &min, &ss, ms);
	if (ret != 6)
		return -1;

	time_t now = time(NULL);
	struct tm curr_utc = *gmtime(&now);
	utc_tm->tm_year = curr_utc.tm_year;
	utc_tm->tm_mon = mm - 1;
	utc_tm->tm_mday = dd;
	utc_tm->tm_hour = hh;
	utc_tm->tm_min = min;
	utc_tm->tm_sec = ss;
	utc_tm->tm_isdst = 0;

	*ms = (*ms < 0 || *ms > 999) ? 0 : *ms;
	return 0;
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

static void logcat_parse_log_with_tz(const char *buf, int bytes)
{
	char *nl, *s = cached_log;
	char time_buf[64] = {0};
	char log_buf[sizeof(cached_log)] = {0};

	if (bytes <= 0)
		return;
	if (bytes + cached_log_bytes >= (int)sizeof(cached_log) - 1)
		bytes = sizeof(cached_log) - cached_log_bytes - 1;
	if (bytes < 0)
		bytes = 0;

	memcpy(cached_log + cached_log_bytes, buf, bytes);
	cached_log_bytes += bytes;
	cached_log[cached_log_bytes] = '\0';

	while ((nl = strchr(s, '\n')) != NULL) {
		*nl = '\0';
		s = get_loglevel_from_prefix(s, &loglevel);
		char *log_content = s;

		struct tm utc_tm = {0};
		int ms = 0;
		if (get_utc_time_from_prefix(log_content, &utc_tm, &ms) == 0) {
			typedef struct {
				time_t sec;
				int ms;
			} time_ms_t;
			time_ms_t utc_timems = {0};
			utc_timems.sec = mktime(&utc_tm);
			utc_timems.ms = (ms >= 0 && ms <= 999) ? ms : 0;
			utc_timems.sec += (g_timezone_offset * 3600);

			struct tm tz_tm = *gmtime(&utc_timems.sec);
			snprintf(time_buf, sizeof(time_buf), "[%02d-%02d %02d:%02d:%02d.%03d]",
					 tz_tm.tm_mon + 1, tz_tm.tm_mday, tz_tm.tm_hour,
					 tz_tm.tm_min, tz_tm.tm_sec, utc_timems.ms);

			// Extract pure log content (skip original timestamps)
			char *pure_log = strstr(log_content, "] ");
			pure_log = pure_log ? (pure_log + 2) : log_content;
			snprintf(log_buf, sizeof(log_buf), "%s %s", time_buf, pure_log);
			do_log_common(loglevel, FC, LN, "%s\n", log_buf);
			s = nl + 1;
			continue;
		}

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
			logcat_parse_log_with_tz(line_buf, t);
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

	if (logcat_clear_logs) {
		fd_set read_fds;
		struct timeval timeout;

		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		while (select(sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
			if (FD_ISSET(sock, &read_fds)) {
				bytes = read(sock, buf, sizeof(buf) - 1);
				buf[sizeof(buf) - 1] = '\0';
				if (bytes <= 0) {
					break;
				}
			}
		}
		log_info("%s: %s logcat buffer cleared\n", tag, me);
	}

	int read_max_times = (log_readout_once_time > 0) ? log_readout_once_time : -1;
	int min_period = 100000;
	int cnt = 0;

	while (!log_get_exit_flag()) {
		fd_set read_fds;
		struct timeval timeout = {0, min_period};   //100ms

		cnt++;
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds);

		int ret = select(sock + 1, &read_fds, NULL, NULL, &timeout);
		if (ret < 0) {
			log_err("%s: read socket error: %d (%s), try to reconnect...\n", tag, errno, strerror(errno));
			break;
		}
		else if (ret == 0) {
			if (cnt % ((1000000 / min_period)) == 0)
				log_debug("%s: read socket timeout: %ds, try to reconnect...\n", tag, (cnt * (min_period / 1000)) / 1000);
			if (read_max_times > 0 && cnt >= read_max_times) {
				log_warn("%s: read out all buffer for %d ms, exit logcat.\n", tag, read_max_times * (min_period / 1000));
				log_set_exit_flag();
			}
			continue;
		}
		else {
			bytes = read(sock, buf, sizeof(buf) - 1);
			if (bytes <= 0) {
				if (cnt >= 10) {
					log_emerg("%s: socket connection closed by remote host normally\n", tag);
					break;
				}
				msleep(10);
				continue;
			}
		}

		cnt = 0;
		buf[sizeof(buf) - 1] = '\0';
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

// 用socket接收日志服务，并读取日志数据
static int do_logcat_logall(void)
{
	char buf[3072], * pbuf;
	int sock, bytes;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		log_warn("%s: logcat-logall service can't create...\n", tag);
		sleep(1);
		return -1;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOGCAT_LOGALL_SERVICE_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_err("%s: logcat-logall service bind failed, errno = %d\n", tag, errno);
        close(sock);
        return -1;
    }

	int read_max_times = 50;
	while (read_max_times) {
		bytes = recv(sock, buf, sizeof(buf) - 1, 0);
		buf[sizeof(buf) - 1] = '\0';
		if (bytes < 0) {
			if (errno == EINTR)
				continue;
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

	close(sock);
	return 0;
}

void logall_main(void)
{
	log_verbose_level &= (~_LOG_LEVEL_PREFIX);
	log_verbose_level &= (~_LOG_LEVEL_TIMESTAMP);

	log_to_kmsg_allowed = 0;
	has_log_file_service = 1;
	has_log_logcat_service = 1;


	while (!log_get_exit_flag()) {
		do_logcat_logall();
		msleep(100);
	}
}

void logcat_main(void)
{
	logcat_set_verbose();

	// if (logcat_clear_logs) {
	// 	log_logcat_clear();
	// 	return;
	// }

	if (logcat_cat_log_file) {
		do_logcat_file(logcat_cat_log_file);
		return;
	}

	while (!log_get_exit_flag()) {
		do_logcat_client();
		msleep(100);
	}
}

void set_log_service_name(const char * me)
{
	snprintf(log_service_name + 1, 127, LOG_SERVICE_NAME_FMT, me);
	log_debug("log_service_name: %s\n", log_service_name + 1);
}

void set_log_dir(char * dir_path)
{
	if(dir_path == NULL){
		log_warn("set log dir path can't be null\n");
		return;
	}
	strncpy(log_dir, dir_path, sizeof(log_dir));
}

const char * get_log_dir()
{
	return log_dir;
}

void log_safely_quit(const char * reason)
{
	const char * quit_tag = "Log Safely Quit";
	// log_emerg("%s: %s: reason: %s!\n", me, quit_tag, reason ? reason : "<null>");

	if (logcat_mode()) {
		log_set_exit_flag();
		log_info("%s: %s: logcat exiting...\n\n", me, quit_tag);
		exit(0);
	}

	// Store logs to file timely before safely quit the program
	log_file_sync();

	// log_info("%s: %s: Stop the Threads...\n", me, quit_tag);
	log_set_exit_flag();
	// log_info("%s: %s: exiting...\n\n", me, quit_tag);
	pthread_join(log_file_service_threadid, NULL);
}

void log_quit()
{
	log_safely_quit("User");
}
