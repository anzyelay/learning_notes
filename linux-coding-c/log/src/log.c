#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"
#include "log.h"

unsigned int log_verbose_level = CONFIG_DEFAULT_LOG_LEVEL;
static long timestamp_start = 0;

int has_log_file_service = 1;
int has_log_logcat_service = 1;
int log_to_kmsg_allowed = 1;
int has_log_logall_service = 0;

enum __LOG_COLORS {
	__LOG_ATTR_RESET,
	__LOG_BLACK,
	__LOG_RED,
	__LOG_GREEN,
	__LOG_YELLOW,
	__LOG_BLUE,
	__LOG_MAGENTA,
	__LOG_CYAN,
	__LOG_WHITE,
	__LOG_BOLD,
	__LOG_DIM,
	__LOG_UNDERLINE,
	__LOG_FLASH,
	__LOG_REVERSE,
	__LOG_HIDE,
	__LOG_CLR_SCREEN,
};

static int __log_set_attr(char * buf, const char * attr)
{
	if (!attr)
		return 0;
	return sprintf(buf, "\033[%sm", attr);
}

static int log_attr_set(char * buf, int attr)
{
	const char * mode;

	switch (attr) {
		default: return 0;
		case __LOG_ATTR_RESET:  mode = "0"; break;
		case __LOG_BOLD:        mode = "1"; break;
		case __LOG_DIM:         mode = "2"; break;
		case __LOG_UNDERLINE:   mode = "4"; break;
		case __LOG_FLASH:       mode = "5"; break;
		case __LOG_REVERSE:     mode = "7"; break;
		case __LOG_HIDE:        mode = "8"; break;
		case __LOG_CLR_SCREEN:  return sprintf(buf, "\033[2J");
	}

	return __log_set_attr(buf, mode);
}

static int log_attr_reset(char * buf)
{
	return log_attr_set(buf, __LOG_ATTR_RESET);
}

static int log_attr_set_fg_color(char * buf, int color)
{
	const char * mode;

	switch (color) {
		default: return 0;
		case __LOG_BLACK:    mode = "30"; break;
		case __LOG_RED:      mode = "31"; break;
		case __LOG_GREEN:    mode = "32"; break;
		case __LOG_YELLOW:   mode = "33"; break;
		case __LOG_BLUE:     mode = "34"; break;
		case __LOG_MAGENTA:  mode = "35"; break;
		case __LOG_CYAN:     mode = "36"; break;
		case __LOG_WHITE:    mode = "37"; break;
	}

	return __log_set_attr(buf, mode);
}

static int log_attr_set_bg_color(char * buf, int color)
{
	const char * mode;

	switch (color) {
		default: return 0;
		case __LOG_BLACK:    mode = "40"; break;
		case __LOG_RED:      mode = "41"; break;
		case __LOG_GREEN:    mode = "42"; break;
		case __LOG_YELLOW:   mode = "43"; break;
		case __LOG_BLUE:     mode = "44"; break;
		case __LOG_MAGENTA:  mode = "45"; break;
		case __LOG_CYAN:     mode = "46"; break;
		case __LOG_WHITE:    mode = "47"; break;
	}

	return __log_set_attr(buf, mode);
}

static int log_attr_clr_screen(char * buf)
{
	return log_attr_set(buf, __LOG_CLR_SCREEN);
}

static int log_color_start(char * buf, int level)
{
	int ret = 0;
	int color_debug = __LOG_BLUE;
	int color_debug_v = __LOG_BLUE;

	if (!(log_verbose_level & _LOG_LEVEL_COLORS))
		return 0;

	if (level & _LOG_LEVEL_EMERG) {
		ret += log_attr_set_fg_color(buf + ret, __LOG_MAGENTA);
		ret += log_attr_set(buf + ret, __LOG_REVERSE);
	} else if (level & _LOG_LEVEL_ERR) {
		ret += log_attr_set_fg_color(buf + ret, __LOG_RED);
		ret += log_attr_set(buf + ret, __LOG_REVERSE);
	} else if (level & _LOG_LEVEL_WARN) {
		ret += log_attr_set_fg_color(buf + ret, __LOG_YELLOW);
	} else if (level & _LOG_LEVEL_INFO) {
		ret += log_attr_reset(buf + ret);
	} else if (level & _LOG_LEVEL_VERBOSE) {
		ret += log_attr_set_fg_color(buf + ret, __LOG_BLUE);
	} else if (level & _LOG_LEVEL_DEBUG) {
		ret += log_attr_set_fg_color(buf + ret, color_debug);
	} else if (level & _LOG_LEVEL_DEBUG_V1) {
		ret += log_attr_set_fg_color(buf + ret, color_debug_v);
	} else if (level & _LOG_LEVEL_DEBUG_V2) {
		ret += log_attr_set_fg_color(buf + ret, color_debug_v);
	} else if (level & _LOG_LEVEL_DEBUG_VV) {
		ret += log_attr_set_fg_color(buf + ret, color_debug_v);
	} else if (level & _LOG_LEVEL_DEVICE) {
		ret += log_attr_set_fg_color(buf + ret, __LOG_GREEN);
	} else if (level & _LOG_LEVEL_SYSTEM) {
		ret += log_attr_set_fg_color(buf + ret, __LOG_GREEN);
	// } else if (level & _LOG_LEVEL_BT) {
	// 	ret += log_attr_set_fg_color(buf + ret, __LOG_CYAN);
	// } else if (level & _LOG_LEVEL_WIFI) {
	// 	ret += log_attr_set_fg_color(buf + ret, __LOG_CYAN);
	// } else if (level & _LOG_LEVEL_MODEM) {
	// 	ret += log_attr_set_fg_color(buf + ret, __LOG_YELLOW);
	// } else if (level & _LOG_LEVEL_ESIM) {
	// 	ret += log_attr_set_fg_color(buf + ret, __LOG_YELLOW);
	// } else if (level & _LOG_LEVEL_WEBUI) {
	// 	ret += log_attr_set_fg_color(buf + ret, __LOG_MAGENTA);
	// } else if (level & _LOG_LEVEL_FOTA) {
	// 	ret += log_attr_set_fg_color(buf + ret, __LOG_CYAN);
	// 	ret += log_attr_set(buf + ret, __LOG_REVERSE);
	} else {
		return 0;
	}

	return ret;
}

static int log_color_end(char * buf, int level)
{
	if (!(log_verbose_level & _LOG_LEVEL_COLORS))
		return 0;

	return log_attr_reset(buf);
}

static int log_prefix(char * buf, int level, const char *func, int line)
{
	const char * prefix = "";
	int len = 0;

	if (!(log_verbose_level & _LOG_LEVEL_PREFIX))
		return 0;

	if (level & _LOG_LEVEL_EMERG)
		prefix = "X";
	else if (level & _LOG_LEVEL_ERR)
		prefix = "E";
	else if (level & _LOG_LEVEL_WARN)
		prefix = "W";
	else if (level & _LOG_LEVEL_INFO)
		prefix = "I";
	else if (level & _LOG_LEVEL_VERBOSE)
		prefix = "V";
	else if (level & _LOG_LEVEL_DEBUG)
		prefix = "D";
	else if (level & _LOG_LEVEL_DEBUG_V1)
		prefix = "D: V1";
	else if (level & _LOG_LEVEL_DEBUG_V2)
		prefix = "D: V2";
	else if (level & _LOG_LEVEL_DEBUG_VV)
		prefix = "D: VV";
	else if (level & _LOG_LEVEL_DEVICE)
		prefix = "S: D";
	else if (level & _LOG_LEVEL_SYSTEM)
		prefix = "S: S";
	// else if (level & _LOG_LEVEL_BT)
	// 	prefix = "S: B";
	// else if (level & _LOG_LEVEL_WIFI)
	// 	prefix = "S: W";
	// else if (level & _LOG_LEVEL_MODEM)
	// 	prefix = "S: L";
	// else if (level & _LOG_LEVEL_ESIM)
	// 	prefix = "S: E";
	// else if (level & _LOG_LEVEL_WEBUI)
	// 	prefix = "S: U";
	// else if (level & _LOG_LEVEL_FOTA)
	// 	prefix = "S: O";
	else
		return 0;

	len = sprintf(buf, "[%s: %s: %d] ", prefix, func, line);

	return len;
}

const char * log_get_extra_prefix(unsigned int level)
{
	const char * prefix = "I";

	if (level & _LOG_LEVEL_EMERG)
		prefix = "X";
	else if (level & _LOG_LEVEL_ERR)
		prefix = "E";
	else if (level & _LOG_LEVEL_WARN)
		prefix = "W";
	else if (level & _LOG_LEVEL_INFO)
		prefix = "I";
	else if (level & _LOG_LEVEL_VERBOSE)
		prefix = "V";
	else if (level & _LOG_LEVEL_DEBUG)
		prefix = "D";
	else if (level & _LOG_LEVEL_DEBUG_V1)
		prefix = "D1";
	else if (level & _LOG_LEVEL_DEBUG_V2)
		prefix = "D2";
	else if (level & _LOG_LEVEL_DEBUG_VV)
		prefix = "DV";
	else if (level & _LOG_LEVEL_DEVICE)
		prefix = "SD";
	else if (level & _LOG_LEVEL_SYSTEM)
		prefix = "SS";
	// else if (level & _LOG_LEVEL_BT)
	// 	prefix = "SB";
	// else if (level & _LOG_LEVEL_WIFI)
	// 	prefix = "SW";
	// else if (level & _LOG_LEVEL_MODEM)
	// 	prefix = "SL";
	// else if (level & _LOG_LEVEL_ESIM)
	// 	prefix = "SE";
	// else if (level & _LOG_LEVEL_WEBUI)
	// 	prefix = "SU";
	// else if (level & _LOG_LEVEL_FOTA)
	// 	prefix = "SO";

	return prefix;
}

static int log_timestamp(char * buf, int level)
{
	long timestamp;
	long seconds, ms;
	int len;

	if (!(log_verbose_level & _LOG_LEVEL_TIMESTAMP))
		return 0;

	timestamp = get_time_ms() - timestamp_start;
	if (timestamp < 0)
		timestamp = 0;
	seconds = timestamp / 1000;
	ms = timestamp % 1000;

	len = sprintf(buf, "[%6ld.%03ld] ", seconds, ms);

	return len;
}

void log_set_option_color(int color_en)
{
	if (color_en)
		log_verbose_level |= _LOG_LEVEL_COLORS;
	else
		log_verbose_level &= ~_LOG_LEVEL_COLORS;
}

int do_log_common(unsigned int request_level, const char *func, int line, const char * log_fmt, ...)
{
	FILE * stream;
	va_list args;
	char buf[1024 * 32];
	int ret = 0, prefix_i = 0;
	int buf_start_of_no_color = 0, buf_end_of_no_color = 0;

	// Initial the Timestamp
	if (timestamp_start <= 0)
		timestamp_start = get_timestamp_start();

	// If has any log, enable the Error level
	if (log_verbose_level > 0)
		log_verbose_level |= _LOG_LEVEL_ERR | _LOG_LEVEL_EMERG;

	// 确保没有残留数据
	memset(buf, 0, sizeof(buf));
	prefix_i = 0;

	prefix_i += log_color_start(buf + prefix_i, request_level);
	buf_start_of_no_color = prefix_i;

	prefix_i += log_timestamp(buf + prefix_i, request_level);
	prefix_i += log_prefix(buf + prefix_i, request_level, func, line);

	va_start(args, log_fmt);
	prefix_i += vsnprintf(buf + prefix_i, sizeof(buf) - prefix_i - 1, log_fmt, args);
	va_end(args);

	buf_end_of_no_color = prefix_i;
	prefix_i += log_color_end(buf + prefix_i, request_level);

	// Print to kernel message for Error and Emergency log
	if (log_to_kmsg_allowed && (request_level & (_LOG_LEVEL_ERR | _LOG_LEVEL_EMERG))) {
		write_file_quiet("/dev/kmsg", buf, strlen(buf), 1, 0);
	}

	if (prefix_i >= sizeof(buf) - 1) {
		// memory overflow
		fprintf(stderr, "Fatal Error: memory overflow in do_log_common(), %d >= %d\n", prefix_i, (int)sizeof(buf));
		return -1;
	}

	// 若request_level包含在log_verbose_level中，意味着当前日志级别是被允许的，输出到控制台
	if (request_level & log_verbose_level) {
		stream = stdout;
		ret = fputs(buf, stream);
		// 刷新输出缓冲区,确保日志信息不会被延迟输出
		fflush(stream);
	}
	if (has_log_logcat_service)
		log_logcat_inject(request_level, (const char *)buf + buf_start_of_no_color, buf_end_of_no_color - buf_start_of_no_color);
	if (has_log_file_service)
		log_file_inject(request_level, (const char *)buf + buf_start_of_no_color, buf_end_of_no_color - buf_start_of_no_color);

	if (0) {
		static long log_bytes = 0;
		long k, m;
		log_bytes += prefix_i;
		k = log_bytes / 1024;
		m = k / 1024;
		if (request_level & (_LOG_LEVEL_ERR | _LOG_LEVEL_EMERG)) {
			printf(">>>>>> Log total bytes: %ld bytes, %ld KB, %ld MB\n", log_bytes, k, m);
			fflush(stdout);
		}
	}

	return ret;
}

void start_log_service(void)
{
	if (has_log_logcat_service)
		start_log_logcat_service();
	if (has_log_file_service)
		start_log_file_service();
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

int logall_run(int argc, char *argv[])
{
       int ret = 0;

       setup_signals();

       ret = parse_args(argc, argv);
       if (ret < 0) {
               log_err("parse args in log parse failed\n");
               return -1;
       }

       if (ret == 0) {
               usage(NULL, NULL);
               return -1;
       }

       if (logcat_mode()) {
               if (me != NULL) {
                       log_info("====%s log client====\n", me);
               }
               logcat_main();
               exit(EXIT_SUCCESS);
       }

       if (me != NULL) {
               log_info("%s log server!\n", me);
       }

       has_log_logall_service = 1;

       start_log_service();

       return logall_main();
}
