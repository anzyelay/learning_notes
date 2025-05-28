/*
 * Logs and Log levels - Shujun, 2023
 */
#ifndef _LOG_H
#define _LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "log_out.h"

#define CONFIG_LOGCAT_ONLY_LOCAL  (1)     // logcat enabled only inside UNIX domain, not INET
#if CONFIG_LOGCAT_ONLY_LOCAL
#define LOG_SERVICE_NAME_FMT          "/dev/%s_log_interface"
#endif

#define LOGCAT_LOGALL_SERVICE_PORT  8259       // logall服务端口

extern unsigned int log_verbose_level;
extern int has_log_file_service;
extern int has_log_logcat_service;
extern int has_log_logall_service;
extern int log_to_kmsg_allowed;
extern const char * me;
extern char * log_dir;
extern char default_log_dir[64];
extern char log_service_name[128];

void log_set_option_color(int color_en);

const char * log_get_extra_prefix(unsigned int level);
void log_file_inject(unsigned int request_level, const char * log, int log_bytes);
void log_logcat_inject(unsigned int request_level, const char * log, int log_bytes);
void log_logcat_clear(void);
void start_log_service(void);
void start_log_file_service(void);
void start_log_logcat_service(void);

#define logcat_usage(me, me_copyright, err, err_param) mifi_main_usage(me, me_copyright, err, err_param)
int logcat_mode();
void logcat_main(void);
void set_logcat_mode();
void set_logcat_mode_host(const char * host);
void set_logcat_mode_port(int port);
void set_me(char * name);
void set_log_service_name(const char * me);

#ifdef __cplusplus
}
#endif

#endif /* _LOG_H */
