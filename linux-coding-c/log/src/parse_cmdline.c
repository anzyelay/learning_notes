#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "parse_cmdline.h"

#define tag  "Parse Args"
// static const char * me = "fx-log";
static const char * me_copyright = "JBD-CAR-1. 2024.";
// static const char * name_for_logcat = "fx-log";

static const char * conf_file = "/etc/tbox-cmdline.conf";

static int do_save_config = 0;

extern int logcat_clear_logs;
extern char * logcat_cat_log_file;

const char * firmware_get_version(void)
{
	return "1.0.0";
}

const char * log_get_version(void)
{
	return firmware_get_version();
}

char output_buf[1024];
const char * log_get_help_info(void)
{
	sprintf(output_buf,
			"\t[--verbose=log-level]\n"
			"\t[--logcat]\n"
			"\t[--save-config] [--config-file=conf-file-path]\n"
			"\n"
			"\t[--log-write-file=0/1] [--logcat-service=0/1]\n"
			"\t[--log-no-color] [--log-no-timestamp] [--log-prefix]\n"
			"\t[--log-error-only]\n"
			"\t[--log-verbose] [--log-no-verbose]\n"
			"\t[--log-debug] [--log-debug1] [--log-debug2] [--log-debugv]\n"
			"\t[--log-all]\n"
			"\n"
			"\t[--log-dir=<log-file-dir>]\n"
			"\n"
			"\t[--logcat-file=<log-file>]\n"
			"\t[--logcat-clear | -c ]\n"
			"\n"
			"Default Values:\n"
			"  Verbose:               0x%x\n"
			"  Log File:              %s\n"
			"  Logcat Service:        %s\n"
			"  Config file:           %s\n"
			"\n"
			"Notes:\n"
			"  Option '--save-config' Will exit the program after the configs saved!\n"
			"%s",       // '\n'
			CONFIG_DEFAULT_LOG_LEVEL,
			has_log_file_service ? "Yes" : "No",
			has_log_logcat_service ? "Yes" : "No",
			conf_file,
			"\n"
		);
	return output_buf;
}

static void mifi_main_usage(const char * me, const char * me_copyright, const char * err, const char * err_param)
{
	FILE * output_file = stdout;

	fprintf(output_file, "\n"
			"%s, %s\n", firmware_get_version(), me_copyright);

	if (err) {
		fprintf(output_file, "\nErr: %s%s\n", err, err_param ? err_param : "");
		return;
	}

	fprintf(output_file, "\n"
					"Usage: %s [options]\noptions are:\n%s",
					me,
					log_get_help_info());
}

void usage(const char * err, const char * err_param)
{
	if (logcat_mode()) {
		logcat_usage(me, me_copyright, err, err_param);
		return;
	}

	mifi_main_usage(me, me_copyright, err, err_param);
}

/*
 * Return Value:
 * 1 = Ok, 0 = Not found, -1 = Error Happened
 */
static int parse_arg_common(char * arg)
{
	int log_no_color = 0, log_no_timestamp = 0, log_prefix = 0;
	int log_error_only = 0, log_all = 0;
	int log_verbose = 0, log_no_verbose = 0;
	int log_debug = 0, log_debug1 = 0, log_debug2 = 0, log_debugv = 0;

	if (0) {}
	parse_arg_xint("--verbose=",                 log_verbose_level,       0,       0xffffff)
	parse_arg_string("--config-file=",           conf_file,               1)
	// parse_arg_int("--int-example=",           int_options,             min,     max)
	parse_arg_xint("--log-write-file=",          has_log_file_service,    0,       1)
	parse_arg_xint("--logcat-service=",          has_log_logcat_service,  0,       1)
	parse_arg_set("--log-no-color",              log_no_color,            1)
	parse_arg_set("--log-no-timestamp",          log_no_timestamp,        1)
	parse_arg_set("--log-prefix",                log_prefix,              1)
	parse_arg_set("--log-error-only",            log_error_only,          1)
	parse_arg_set("--log-verbose",               log_verbose,             1)
	parse_arg_set("--log-no-verbose",            log_no_verbose,          1)
	parse_arg_set("--log-debug",                 log_debug,               1)
	parse_arg_set("--log-debug1",                log_debug1,              1)
	parse_arg_set("--log-debug2",                log_debug2,              1)
	parse_arg_set("--log-debugv",                log_debugv,              1)
	parse_arg_set("--log-all",                   log_all,                 1)
	parse_arg_string_cpy("--log-dir=",           default_log_dir,         1)
	parse_arg_string("--logcat-file=",           logcat_cat_log_file,     1)
	parse_arg_set("--logcat-clear",              logcat_clear_logs,       1)
	parse_arg_set("-c",                          logcat_clear_logs,       1)
	else { return 0; /* Not found */ }

	if (log_no_color)
		log_verbose_level &= (~_LOG_LEVEL_COLORS);
	if (log_no_timestamp)
		log_verbose_level &= (~_LOG_LEVEL_TIMESTAMP);
	if (log_prefix)
		log_verbose_level |= _LOG_LEVEL_PREFIX;
	if (log_verbose)
		log_verbose_level |= _LOG_LEVEL_VERBOSE;
	if (log_no_verbose)
		log_verbose_level &= (~_LOG_LEVEL_VERBOSE);
	if (log_debug)
		log_verbose_level |= _LOG_LEVEL_DEBUG;
	if (log_debug1)
		log_verbose_level |= _LOG_LEVEL_DEBUG_V1;
	if (log_debug2)
		log_verbose_level |= _LOG_LEVEL_DEBUG_V2;
	if (log_debugv)
		log_verbose_level |= _LOG_LEVEL_DEBUG_VV;
	if (log_error_only)
		log_verbose_level = (_LOG_LEVEL_EMERG | _LOG_LEVEL_ERR | _LOG_LEVEL_TIMESTAMP | _LOG_LEVEL_COLORS);
	if (log_all)
		log_verbose_level = CONFIG_LOG_ALL_LEVEL;

	return 1;
}
/*
 * Return Value:
 * 1 = Ok, 0 = Not found, -1 = Error Happened
 */
static int parse_args_once(int argc, char **argv)
{
	int logcat_host = 0, logcat_port = 0;
	int ret;
	int unfound = 0;

	// if (!strcmp(me, name_for_logcat))
	// 	set_logcat_mode();

	for (;;) {
		argv++;
		argc--;
		if (argc <= 0)
			break;

		log_debug_vv("%s: %s\n", tag, argv[0]);
		ret = parse_arg_common(argv[0]);
		if (ret > 0) {
			continue;
		} else if (ret < 0) {
			return -1;
		} else if (!strcmp(argv[0], "--logcat")) {
			set_logcat_mode();
		} else if (!strcmp(argv[0], "--save-config")) {
			do_save_config = 1;
		} else {
			// logcat mode, set the host and port
			if (logcat_mode() && argv[0][0] != '-') {
				if (!logcat_host) {
					logcat_host = 1;
					set_logcat_mode_host(argv[0]);
				} else if (!logcat_port) {
					logcat_port = 1;
					set_logcat_mode_port(atoi(argv[0]));
				}
				continue;
			}
			else {
				unfound = 1;
			}
			// usage("Invalid option: ", argv[0]);
			// return -1;
		}
	}

	return unfound == 1 ? 0 : 1;
}

static int read_conf_file(const char* filename, char* buf, int bytes)
{
	int fd, ret;

	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		log_verbose("Config file: open file failed: %s, err = %d\n", filename, errno);
		return fd;
	}

	ret = read(fd, buf, bytes);
	if(ret < 0) {
		log_verbose("Config file: read file failed: %s, err = %d\n", filename, errno);
		return ret;
	}

	close(fd);

	return 0;
}

static int write_conf_file(const char * filename, const char * data)
{
	int fd, ret;

	fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if (fd < 0) {
		log_err("Config file: open file failed: %s, err = %d(%s)\n", filename, errno, strerror(errno));
		return fd;
	}

	ret = write(fd, data, strlen(data));
	if (ret < 0) {
		log_err("Config file: write file failed: %s, err = %d, data =\n%s\n", filename, errno, data);
		return ret;
	}

	close(fd);

	return 0;
}

int read_from_conf_file(void)
{
	char buf[1024], *head, *next = buf;
	int ret;

	log_debug("Config file: %s\n", conf_file);

	memset(buf, 0, sizeof(buf));
	ret = read_conf_file(conf_file, buf, sizeof(buf) - 1);
	if (ret < 0)
		return ret;

	while (1) {
		// Find the head
		head = next;
		while (*head) { // Skip the spaces
			char c = *head;
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
				head++;
			else
				break;
		}
		if (!*head)
			return 0;

		// Find the tail
		next = head + 1;
		while (*next) { // Find the spaces
			char c = *next;
			if (c == '\n' || c == '\r')
				break;
			next++;
		}
		if (*next) {
			*next = 0; // Give the head a tail '\0';
			next++;    // Point the next line
		}

		// skip the line begin with #
		if (*head == '#') {
			continue;
		}

		// Now, we got the line, parse it!
		log_debug("Config data: %s\n", head);
		ret = parse_arg_common(head);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int write_to_conf_file(void)
{
	char buf[1024];
	int ret;

	memset(buf, 0, sizeof(buf));
	sprintf(buf,
		"--verbose=0x%x\n"
		"--log-write-file=%d\n"
		"--logcat-service=%d\n"
		"--log-dir=%s\n"
		"%s", // "\n"
		log_verbose_level,
		!!has_log_file_service,
		!!has_log_logcat_service,
		default_log_dir,
		"\n");

	ret = write_conf_file(conf_file, buf);
	if (ret < 0)
		return ret;

	return 0;
}
/*
 * Return Value:
 * 1 = Ok, 0 = Not found, -1 = Error Happened
 */
int parse_args(int argc, char **argv)
{
	int ret;

	// Save this program name
	set_me(argv[0]);

	// Set the log service name
	set_log_service_name(me);

	// Get the new config file path
	ret = parse_args_once(argc, argv);
	if (ret <= 0)
		return ret;

	// If this is the 'logcat', we return here
	if (logcat_mode())
		return 1;

	// If --save-config set, save the configs to file, and then exit!
	if (do_save_config) {
		ret = write_to_conf_file();
		if (ret < 0) {
			log_err("\n");
			log_err("Save config failed: %s\n\n", conf_file);
		} else {
			log_info("\n");
			log_info("Save config OK: %s\n\n", conf_file);
		}
		exit(0);
	}

	// Get the configs from file
	if(read_from_conf_file()) {
		log_info("read config file failed: %s\n", conf_file);
		write_to_conf_file();
	}

	// Set the log dir path
	if (strlen(default_log_dir) > 0)
		set_log_dir(default_log_dir);

	// Overwrite the configs with command line
	ret = parse_args_once(argc, argv);

	return ret;
}
