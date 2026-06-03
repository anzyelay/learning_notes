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
#include <dirent.h>
#include <sys/statvfs.h>

#include "common.h"
#include "log.h"
#include "pipe.h"

#define tag            "Log File"

#define LOG_DIR        "/home/log"
unsigned int log_file_verbose_level = CONFIG_LOG_FILE_LEVEL;// 日志文件打印级别
long log_file_write_period = 30;            		// Write to log file every 30s or ERR / Emergency /Warn log issued
static long log_file_max_number = 5;				// Maximum number of log files
static size_t log_file_max_size = (1024 * 1024 * 2);// Maximum size of each log file (2MB)
#define LOG_FILE_SN_FILENAME  "%s/%s.log.sn"
#define LOG_FILE_FILENAME_FMT "%s/%s.%d.log"
#define LOG_WRITE_PAUSE "/run/log/fii-log-pause"

char log_dir[128] = LOG_DIR;

const char * me = NULL;			// program name

#define LOG_PIPE_SIZE  (1024 * 1024)
static int log_file_ready = 0;						// Log file service is ready
static volatile int log_file_need_write = 0;
static pipe_t saved_log_pipe;

#define EARLY_LOG_SIZE (1024 * 128)
static char cached_early_log_buf[EARLY_LOG_SIZE];	// Cached early log buffer
static int  cached_early_log_bytes = 0;

static int clean_log_files(char *dir);
int check_storage_space(const char *dir, size_t required_space);
static void may_write_to_file(unsigned int request_level, const char * extra_prefix, const char * log, int log_bytes);
static void write_to_log_file_pipe(const char * extra_prefix, const char * log, int log_bytes);

static pthread_mutex_t log_file_service_mutex;
static pthread_mutexattr_t log_file_service_mutex_attr;

#define log_file_service_mutex_init() do { \
	pthread_mutexattr_settype(&log_file_service_mutex_attr, PTHREAD_MUTEX_RECURSIVE); \
	pthread_mutex_init(&log_file_service_mutex, &log_file_service_mutex_attr); } while (0)
#define log_file_service_mutex_destroy() do { pthread_mutex_destroy(&log_file_service_mutex); } while (0)
#define log_file_service_mutex_lock() do { \
    int lock_ret = pthread_mutex_lock(&log_file_service_mutex); \
    if (lock_ret != 0) { \
        log_err("Failed to lock log_file_service_mutex: %s\n", strerror(lock_ret)); \
    } \
} while (0)

#define log_file_service_mutex_unlock() do { \
    int unlock_ret = pthread_mutex_unlock(&log_file_service_mutex); \
    if (unlock_ret != 0) { \
        log_err("Failed to unlock log_file_service_mutex: %s\n", strerror(unlock_ret)); \
    } \
} while (0)

void set_me(char * name)
{
	me = strrchr(name, '/');
	if (me)
		me++; // Skip the '/'
	else
		me = name;
	log_debug("%s: set program name: %s\n", tag, me);
}

// 递归地创建目录
static int create_dir_recursively(const char *dir)
{
    char tmp[512];
    char *p = NULL;

    // 复制路径到临时变量中
    snprintf(tmp, sizeof(tmp), "%s", dir);

    // 遍历每个目录层级
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0'; // 临时终止字符串
            // 创建目录
            if (mkdir(tmp, 0777) < 0) {
                if (errno != EEXIST) {
                    log_err("mkdir(%s) failed, errno = %d (%s)\n", tmp, errno, strerror(errno));
                    return -1;
                }
            }
            *p = '/'; // 还原字符串
        }
    }
    // 创建最后一级目录
    if (mkdir(tmp, 0777) < 0) {
        if (errno != EEXIST) {
            log_err("mkdir(%s) failed, errno = %d (%s)\n", tmp, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

// 创建日志目录，如果目录已经存在则不进行操作
static int try_create_log_dir(void)
{
    if (log_dir != NULL && strlen(log_dir) > 0) {
        if (create_dir_recursively(log_dir) >= 0) {
            return 0;
        }
    }
    return -1;
}

// 初始化日志文件服务
static void log_file_service_init(void)
{
	int ret;

	set_single_called(return);

	// Initial the mutex
	log_file_service_mutex_init();

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
    log_debug("%s: %s: log directory is ready: %s\n", me, tag, log_dir);

try_get_early_log:
	if (log_file_ready)
		pipe_write(&saved_log_pipe, cached_early_log_buf, cached_early_log_bytes);
	cached_early_log_bytes = -1;  // Stop cached early log buffer
}

static void __log_file_inject(unsigned int request_level, const char * log, int log_bytes)
{
	const char * extra_tag = log_get_extra_prefix(request_level);
	char extra_prefix[32];

	snprintf(extra_prefix, 32, "<%s>", extra_tag);

	if (cached_early_log_bytes >= 0) {
		int extra_prefix_len, log_len, len;

		extra_prefix_len = strlen(extra_prefix);
		log_len = log_bytes;
		len = cached_early_log_bytes + extra_prefix_len + log_len;

		if (len < EARLY_LOG_SIZE && log_len >= 0) {
			strncpy(cached_early_log_buf + cached_early_log_bytes, extra_prefix, EARLY_LOG_SIZE-cached_early_log_bytes);
			cached_early_log_bytes += extra_prefix_len;
			strncpy(cached_early_log_buf + cached_early_log_bytes, log, EARLY_LOG_SIZE-cached_early_log_bytes);
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
 * 1) The log level is high or critical, including EMERT, ERR, WARN
 * 2) Over 30 seconds from last write
 * 3) The log size is big enough (more than half of the pipe size)
 */
static void may_write_to_file(unsigned int request_level, const char * extra_prefix, const char * log, int log_bytes)
{
	long buf_size, log_size;

	if (!(request_level & log_file_verbose_level))
		return;

	write_to_log_file_pipe(extra_prefix, log, log_bytes);

	if (request_level & (_LOG_LEVEL_EMERG | _LOG_LEVEL_ERR  | _LOG_LEVEL_WARN))
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
static int get_log_file_fd_with_sn()
{
    char filename[256];
	char log_file_sn_filename[256];
	char sn_buf[128];
	int byte, ret, fd, file_sn = 0;
	size_t filesize;

	memset(filename, 0, sizeof(filename));
	memset(sn_buf, 0, sizeof(sn_buf));
	snprintf(log_file_sn_filename, 256, LOG_FILE_SN_FILENAME, log_dir, me);
	fd = open(log_file_sn_filename, O_RDONLY);
	if (fd >= 0) {
        byte = read(fd, sn_buf, sizeof(sn_buf) - 1);
		if (byte > 0) {
            sn_buf[byte] = '\0';
			file_sn = atoi(sn_buf);
        }
		close(fd);
	}
	if (file_sn < 0 || file_sn >= log_file_max_number)
		file_sn = 0;
	snprintf(filename, 256, LOG_FILE_FILENAME_FMT, log_dir, me, file_sn);

	// If the file is too large, use the next one!
	filesize = get_file_size(filename);
	if ((signed long)filesize >= (signed long)log_file_max_size) {
		log_debug_v2("%s: current log file is too big: %s, %ld > %ld bytes\n", tag, filename, filesize, log_file_max_size);
		file_sn++;
		if (file_sn < 0 || file_sn >= log_file_max_number)
			file_sn = 0;
		snprintf(sn_buf, 128, "%d\n", file_sn);
		fd = open(log_file_sn_filename, O_WRONLY | O_CREAT, 0666);
		if (fd >= 0) {
			ret = write(fd, sn_buf, sizeof(sn_buf));
            if (ret < 0) {
                log_err("%s: write error to fd-%d, %d bytes, errno: %d\n", tag, fd, ret, errno);
            }
			close(fd);
		}

		snprintf(filename, 256, LOG_FILE_FILENAME_FMT, log_dir, me, file_sn);
		truncate(filename, 0);
		log_debug_v2("%s: switch to next log file: %s\n", tag, filename);
	}

    fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
	if (fd < 0) {
		log_err("%s: open log file failed: %s, error = %d (%s)\n", tag, filename, errno, strerror(errno));
		return -1;
	}
    return fd;
}

static int get_log_file_fd_with_date()
{
    char filename[256];
    int fd = 0;

    memset(filename, 0, sizeof(filename));
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    if (lt == NULL) {
        log_err("%s: localtime time info error\n", tag);
        return -1;
    }

    struct tm safe_tm = *lt;
    long cur_ts = (safe_tm.tm_year - 100) * 10000 + (safe_tm.tm_mon + 1) * 100 + safe_tm.tm_mday;
    static long last_ts = 0;

    if (cur_ts != last_ts) {
        last_ts = cur_ts;
        clean_log_files(log_dir);
    }

    snprintf(filename, sizeof(filename), "%s/%ld.log", log_dir, cur_ts);

    fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd < 0) {
        log_debug_v2("%s: create log file failed: %s, error = %d (%s)\n", tag, filename, errno, strerror(errno));
        return -1;
    }
    return fd;
}

/*
 * Only called by log_file_service_thread()
 * 将管道中的数据写入到指定日志文件中
 */
static void write_to_log_file(void)
{
	char buf[4096];
	int fd, ret, bytes;

    if (has_log_logall_service) {
        fd = get_log_file_fd_with_date();
    } else {
        fd = get_log_file_fd_with_sn();
    }

    if (fd < 0) return;

	while (1) {
		bytes = pipe_read(&saved_log_pipe, buf, sizeof(buf), 0, 0);
		log_never("%s: write file, %d bytes\n", tag, bytes);
		if (bytes > 0) {
			ret = write(fd, buf, bytes);
			if (ret < 0)
				log_debug_v2("%s: write log file failed, error = %d (%s)\n", tag, errno, strerror(errno));
		}
		if (bytes < (int)sizeof(buf))
			break;
	}
	close(fd);
	sync();
}

static void _write_to_log_file(const char *buf)
{
    int fd, ret;

    if (has_log_logall_service) {
        fd = get_log_file_fd_with_date();
    } else {
        fd = get_log_file_fd_with_sn();
    }

    if (fd < 0) return;

    ret = write(fd, buf, strlen(buf));
    if (ret < 0){
        log_debug_v2("%s: write log file failed, error = %d (%s)\n", tag, errno, strerror(errno));
    }

	close(fd);
	sync();
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
	long log_file_timestamp_start = 0;
	long storage_check_timestamp_start = 0;
	long timestamp, seconds;
	long storage_timestamp, storage_seconds;
	int pause_flag = 0;

	// pthread_detach(pthread_self()); // Need NO pthread_join()

	log_file_timestamp_start = get_time_ms();
	storage_check_timestamp_start = get_time_ms();

	// 定期检查是否需要写入日志到文件中
	while(!log_get_exit_flag()){
		if (log_file_need_write) {
            if (access(LOG_WRITE_PAUSE, F_OK) == 0) {
                if (!pause_flag) {
                    char buf[128] = {0};
                    snprintf(buf, sizeof(buf) - 1, "pausing writing log files because %s exist\n", LOG_WRITE_PAUSE);
                    _write_to_log_file(buf);
                }
                pause_flag = 1;
            } else {
                pause_flag = 0;
                log_file_need_write = 0;
                write_to_log_file();
			    log_file_timestamp_start = get_time_ms();
            }
		} else {
			timestamp = get_time_ms() - log_file_timestamp_start;
			if (timestamp < 0)
				timestamp = 0;
			seconds = timestamp / 1000.0;
			if (seconds > log_file_write_period) {
				log_file_need_write = 1;
			}
		}

		storage_timestamp = get_time_ms() - storage_check_timestamp_start;
		if (storage_timestamp < 0)
			storage_timestamp = 0;
		storage_seconds = storage_timestamp / 1000.0;
		if (storage_seconds > 60) {
			check_storage_space(log_dir, log_file_max_size);
			storage_check_timestamp_start = get_time_ms();
		}

		msleep(500);
	}

	log_info("%s: log file service thread exit\n", tag);
    if (!pause_flag) {
        write_to_log_file();
    }
	return NULL;
}

pthread_t log_file_service_threadid;

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


void log_file_sync()
{
    log_file_need_write = 1;
}

int filter_log_files(const struct dirent *entry)
{
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        return 0;
    }

    return has_suffix(entry->d_name, ".log") || has_suffix(entry->d_name, ".tgz");
}

void* compress_thread(void* arg)
{
    int ret = -1;
    char *filename = (char*)arg;
    char command[512];
    char delete_path[256];

    snprintf(command, sizeof(command), "tar -czf \"%s/%s.tgz\" -C \"%s\" \"%s\"",
             log_dir, filename, log_dir, filename);

    ret = system(command);
    if (ret == 0) {
        log_info("%s: File %s compressed successfully.\n", tag, filename);
        snprintf(delete_path, sizeof(delete_path), "%s/%s", log_dir, filename);
        remove_file(delete_path);
    } else {
        log_err("%s: Failed to compress file %s, err = %d\n", tag, filename, ret);
    }

    return (void *)0;
}

static time_t get_time_from_file_name(const char *filename)
{
    struct tm file_time_tm = {0};
    char date_part[9];
    const char *dot_pos;

    dot_pos = strchr(filename, '.');
    if (dot_pos == NULL) {
        // log_warn("%s: Invalid filename format (no extension): %s\n", tag, filename);
        return -1;
    }

    size_t date_len = dot_pos - filename;
    if (date_len == 6) {
        int year, month, day;
        if (sscanf(filename, "%2d%2d%2d", &year, &month, &day) != 3) {
            return -1;
        }
        year += 2000;   // short format: YYMMDD, year+=2000
        snprintf(date_part, sizeof(date_part), "%04d%02d%02d", year, month, day);
        date_part[8] = '\0';
    } else {
        // log_warn("%s: Invalid date format in filename: %s\n", tag, filename);
        return -1;
    }

    if (strptime(date_part, "%Y%m%d", &file_time_tm) == NULL) {
        log_err("%s: Failed to parse date from filename %s (date_part: %s)\n", tag, filename, date_part);
        return -1;
    }

    file_time_tm.tm_hour = 0;
    file_time_tm.tm_min = 0;
    file_time_tm.tm_sec = 0;
    file_time_tm.tm_isdst = -1;

    return mktime(&file_time_tm);
}

int handle_log_file(const char *filename)
{
    char full_path[256];
    time_t current_time, file_create_time, one_month_ago;

    file_create_time = get_time_from_file_name(filename);
    if (file_create_time == -1) {
        // log_warn("%s: Failed to convert date from filename %s to time_t\n", tag, filename);
        return -1;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s", log_dir, filename);
    current_time = time(NULL);
    one_month_ago = current_time - 31 * 24 * 60 * 60;

    if (file_create_time < one_month_ago) {
        log_info("%s: Removing old log file: %s\n", tag, filename);
        remove_file(full_path);
    } else if (file_create_time < (current_time - 24 * 60 * 60)) {
        if (has_suffix(filename, ".log")) {
            pthread_t thread_id;
            int ret = pthread_create(&thread_id, NULL, compress_thread, (void*)filename);
            if (ret != 0) {
                log_err("%s: Failed to create compression thread for %s, err = %d\n", tag, filename, ret);
                return -1;
            }
            pthread_detach(thread_id);
        }
    }
    return 0;
}

static int clean_log_files(char *dir)
{
    int n;
    struct dirent **namelist;

    n = scandir(dir, &namelist, filter_log_files, alphasort);
    if (n < 0) {
        log_err("%s: Failed to scan directory %s, err = %d (%s)\n", tag, dir, errno, strerror(errno));
        return 1;
    }
    for (int i = 0; i < n; i++) {
        handle_log_file(namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);

    return 0;
}

int compare_files_by_creation_time(const void *a, const void *b)
{
    struct dirent **entry_a = (struct dirent **)a;
    struct dirent **entry_b = (struct dirent **)b;

    time_t time_a = get_time_from_file_name((*entry_a)->d_name);
    time_t time_b = get_time_from_file_name((*entry_b)->d_name);

    if (time_a == -1 && time_b == -1)
        return 0;
    if (time_a == -1)
        return -1;
    if (time_b == -1)
        return 1;

    return (time_a > time_b) - (time_a < time_b);
}

int check_storage_space(const char *dir, size_t required_space)
{
    struct statvfs fs_stat;
    size_t free_space;
    struct dirent **namelist;
    int n, deleted_count = 0;
    char full_path[512] = {0};

    if (statvfs(dir, &fs_stat) < 0) {
        log_emerg("%s: statvfs(%s) failed, errno = %d (%s)\n", tag, dir, errno, strerror(errno));
        return -1;
    }

    free_space = fs_stat.f_bavail * fs_stat.f_frsize;
    if (free_space >= required_space)
        return 0;

    log_warn("%s: Insufficient storage space: %zu bytes free, %zu bytes required. "
             "Will delete oldest log files.\n", tag, free_space, required_space);
    n = scandir(dir, &namelist, filter_log_files, alphasort);
    if (n < 0) {
        log_err("%s: Failed to scan directory %s, err = %d (%s)\n", tag, dir, errno, strerror(errno));
        return -1;
    }

    qsort(namelist, n, sizeof(struct dirent *), compare_files_by_creation_time);
    for (int i = 0; i < n && free_space < required_space; i++) {
        if (namelist[i] == NULL) {
            continue;
        }

        memset(full_path, 0, sizeof(full_path));
        snprintf(full_path, sizeof(full_path)-1, "%s/%s", dir, namelist[i]->d_name);
        log_info("%s: Deleting oldest log file to free space: %s\n", tag, namelist[i]->d_name);
        remove_file(full_path);
        deleted_count++;

        if (statvfs(dir, &fs_stat) < 0) {
            log_err("%s: Failed to recheck storage space\n", tag);
            break;
        }
        free_space = fs_stat.f_bavail * fs_stat.f_frsize;
    }

    for (int i = 0; i < n; i++) {
        if (namelist[i] != NULL) {
            free(namelist[i]);
        }
    }
    free(namelist);

    if (free_space >= required_space) {
        log_info("%s: Successfully freed space by deleting %d files. "
                 "Now free space: %zu bytes\n", tag, deleted_count, free_space);
        return 0;
    } else {
        log_emerg("%s: Still insufficient space after deleting %d files. "
                  "Free space: %zu bytes, required: %zu bytes\n",
                  tag, deleted_count, free_space, required_space);
        return -1;
    }
}
