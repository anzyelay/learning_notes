/*
 * Log File service -- Shujun. 2023
 *
 * Write system logs to files, which is usful for debug
 * Log location: /foxlog/log/
 *
 * 日志文件服务，能够在多线程环境下安全地记录和管理系统日志
 * 主要功能包括：
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
#include <libgen.h>

#include "common.h"
#include "log.h"
#include "pipe.h"

#define tag            "Log File"

unsigned int log_file_verbose_level = CONFIG_LOG_FILE_LEVEL;// 日志文件打印级别
long log_file_write_period = 3600 * 24;            // Write to log file every 24 hours or ERR / Emergency log issued
static long log_file_max_number = 5;				// Maximum number of log files
static size_t log_file_max_size = (1024 * 1024 * 2);// Maximum size of each log file (2MB)
#define LOG_FILE_SN_FILENAME  "%s/%s.log.sn"
#define LOG_FILE_FILENAME_FMT "%s/%s.%d.log"

#define LOG_FILE_DIR "%s/log"
char default_log_dir[64];		// 日志文件默认存放的目录
char * log_dir = NULL;

const char * me = NULL;			// program name

#define LOG_PIPE_SIZE  (1024 * 1024)
static int log_file_ready = 0;						// Log file service is ready
static long log_file_timestamp_start = 0;
static volatile int log_file_need_write = 0;
static pipe_t saved_log_pipe;

#define EARLY_LOG_SIZE (1024 * 128)
static char cached_early_log_buf[EARLY_LOG_SIZE];	// Cached early log buffer
static int  cached_early_log_bytes = 0;

static void may_write_to_file(unsigned int request_level, const char * extra_prefix, const char * log, int log_bytes);
static void write_to_log_file_pipe(const char * extra_prefix, const char * log, int log_bytes);

static pthread_mutex_t log_file_service_mutex;
static pthread_mutexattr_t log_file_service_mutex_attr;

#define log_file_service_mutex_init() do { \
	pthread_mutexattr_settype(&log_file_service_mutex_attr, PTHREAD_MUTEX_RECURSIVE); \
	pthread_mutex_init(&log_file_service_mutex, &log_file_service_mutex_attr); } while (0)
#define log_file_service_mutex_destroy() do { pthread_mutex_destroy(&log_file_service_mutex); } while (0)
#define log_file_service_mutex_lock() do { pthread_mutex_lock(&log_file_service_mutex); } while (0)
#define log_file_service_mutex_unlock() do { pthread_mutex_unlock(&log_file_service_mutex); } while(0)

void set_me(char * name)
{
	me = strrchr(name, '/');
	if (me)
		me++; // Skip the '/'
	else
		me = name;
	log_debug("%s: set program name: %s\n", tag, me);
}

int set_default_log_dir(char * name)
{
	char tmp[64];

	if(!realpath(name, tmp)){
		log_err("%s: failed to resolve log directory, errno = %d (%s)\n", tag, errno, strerror(errno));
		return -1;
	}
	memset(default_log_dir, 0, sizeof(default_log_dir));
	sprintf(default_log_dir, LOG_FILE_DIR, dirname(tmp));
	log_debug("%s: set default log dir: %s\n", tag, default_log_dir);
	return 0;
}

// 创建日志目录，如果目录已经存在则不进行操作
static int try_create_log_dir(void)
{
	int ret;

	if(log_dir != NULL && strlen(log_dir) > 0){
		ret = mkdir(log_dir, 0777);
		if (ret < 0 && errno != EEXIST) {
			log_err("%s: mkdir(%s) failed, errno = %d (%s)\n", tag, log_dir, errno, strerror(errno));
			return -1;
		}
		log_debug("%s: log directory is ready: %s\n", tag, log_dir);
	}
	return 0;
}

// 初始化日志文件服务
static void log_file_service_init(void)
{
	int ret;

	set_single_called(return);

	// Initial the mutex
	log_file_service_mutex_init();

	// Initial the Timestamp
	log_file_timestamp_start = get_time_ms();

	// Initial log file pipe
	pipe_init(&saved_log_pipe);
	ret = pipe_open(&saved_log_pipe, LOG_PIPE_SIZE, 0);
	if (ret != PIPE_ERR_OK) {
		log_err("%s: Out of memory for log service init\n", tag);
		goto try_get_early_log;
	}
	pipe_set_loop_overwrite(&saved_log_pipe, 1);

	// create the log directory
	ret = try_create_log_dir();
	if (ret < 0)
		goto try_get_early_log;
	log_file_ready = 1;

try_get_early_log:
	if (log_file_ready)
		pipe_write(&saved_log_pipe, cached_early_log_buf, cached_early_log_bytes);
	cached_early_log_bytes = -1;  // Stop cached early log buffer
}

static void __log_file_inject(unsigned int request_level, const char * log, int log_bytes)
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

	if (log_file_ready)
		may_write_to_file(request_level, extra_prefix, log, log_bytes);
}

// 注入日志，线程安全
void log_file_inject(unsigned int request_level, const char * log, int log_bytes)
{
	log_file_service_init();
	log_file_service_mutex_lock();
	__log_file_inject(request_level, log, log_bytes);
	log_file_service_mutex_unlock();
}

/*
 * The log file's write may delay... except below cases:
 * 1) The log level is high or critical, including EMERT, ERR, INFO
 * 2) Over 30 seconds from last write
 * 3) The log size is big enough (more than half of the pipe size)
 */
static void may_write_to_file(unsigned int request_level, const char * extra_prefix, const char * log, int log_bytes)
{
	long timestamp, seconds;
	long buf_size, log_size;

	if (!(request_level & log_file_verbose_level))
		return;

	write_to_log_file_pipe(extra_prefix, log, log_bytes);

	if (request_level & (_LOG_LEVEL_EMERG | _LOG_LEVEL_ERR))
		log_file_need_write = 1;

	timestamp = get_time_ms() - log_file_timestamp_start;
	if (timestamp < 0)
		timestamp = 0;
	seconds = timestamp / 1000;
	if (seconds > log_file_write_period)
		log_file_need_write = 1;

	buf_size = pipe_get_size(&saved_log_pipe);
	log_size = pipe_get_bytes(&saved_log_pipe);
	if (buf_size > 0 && log_size > 0 && log_size >= (buf_size / 2))
		log_file_need_write = 1;
}

/*
 * Only called by log_file_service_thread()
 * 生成适当的日志文件名，并在必要时切换到下一个日志文件
 */
static void get_log_filename(char * filename)
{
	char log_file_sn_filename[256];
	char sn_buf[128];
	int fd, file_sn = 0;
	size_t filesize;

	memset(sn_buf, 0, sizeof(sn_buf));
	sprintf(log_file_sn_filename, LOG_FILE_SN_FILENAME, log_dir, me);
	fd = open(log_file_sn_filename, O_RDONLY);
	if (fd >= 0) {
		if (read(fd, sn_buf, sizeof(sn_buf) - 1) > 0)
			file_sn = atoi(sn_buf);
		close(fd);
	}
	if (file_sn < 0 || file_sn >= log_file_max_number)
		file_sn = 0;
	sprintf(filename, LOG_FILE_FILENAME_FMT, log_dir, me, file_sn);

	// If the file is too large, use the next one!
	filesize = get_file_size(filename, 1);
	if ((signed long)filesize >= (signed long)log_file_max_size) {
		log_debug_v2("%s: current log file is too big: %s, %ld > %ld bytes\n", tag, filename, filesize, log_file_max_size);
		file_sn++;
		if (file_sn < 0 || file_sn >= log_file_max_number)
			file_sn = 0;
		sprintf(sn_buf, "%d\n", file_sn);
		fd = open(log_file_sn_filename, O_WRONLY | O_CREAT, 0666);
		if (fd >= 0) {
			write(fd, sn_buf, strlen(sn_buf));
			close(fd);
		}

		sprintf(filename, LOG_FILE_FILENAME_FMT, log_dir, me, file_sn);
		truncate(filename, 0);
		log_debug_v2("%s: switch to next log file: %s\n", tag, filename);
	}
}

/*
 * Only called by log_file_service_thread()
 * 将管道中的数据写入到指定日志文件中
 */
static void write_to_log_file(void)
{
	char filename[256], buf[4096];
	int fd, ret, bytes;

	// Try to create the log directory in case of removed by FOTA service
	ret = try_create_log_dir();
	if (ret < 0) {
		log_debug_v2("%s: create log file failed: %s, error = %d (%s)\n", tag, filename, errno, strerror(errno));
		return;
	}

	memset(filename, 0, sizeof(filename));
	get_log_filename(filename);
	// printf("now filename: %s\n", filename);
	fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
	if (fd < 0) {
		log_debug_v2("%s: create log file failed: %s, error = %d (%s)\n", tag, filename, errno, strerror(errno));
		return;
	}

	while (1) {
		bytes = pipe_read(&saved_log_pipe, buf, sizeof(buf), 0, 0);
		log_debug_v2("%s: write file, %d bytes\n", tag, bytes);
		if (bytes > 0) {
			ret = write(fd, buf, bytes);
			if (ret < 0)
				log_debug_v2("%s: write log file failed: %s, error = %d (%s)\n", tag, filename, errno, strerror(errno));
		}
		if (bytes < (int)sizeof(buf))
			break;
	}
	close(fd);
}

// 将日志写入管道
static void write_to_log_file_pipe(const char * extra_prefix, const char * log, int log_bytes)
{
	pipe_write(&saved_log_pipe, extra_prefix, strlen(extra_prefix));
	pipe_write(&saved_log_pipe, log, log_bytes);
}

// 日志文件服务线程
static void * log_file_service_thread(void *param)
{
	pthread_detach(pthread_self()); // Need NO pthread_join()

	// 定期检查是否需要写入日志到文件中
	while (1) {
		if (log_file_need_write) {
			log_file_need_write = 0;
			write_to_log_file();
			log_file_timestamp_start = get_time_ms();
			sleep(1);
		}
		sleep(1);
	}

	return NULL;
}

static pthread_t log_file_service_threadid;

// 启动日志文件服务，包括初始化并创建服务线程
void start_log_file_service(void)
{
	int ret;
	set_single_called(return);

	log_file_service_init();
	ret = pthread_create(&log_file_service_threadid, NULL, log_file_service_thread, NULL);
	if (ret != 0)
		log_err("%s: create log file service thread failed: %d\n", tag, ret);
}
