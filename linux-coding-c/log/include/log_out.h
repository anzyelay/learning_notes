#ifndef _LOG_OUT_H
#define _LOG_OUT_H

#define CONFIG_BASIC_LOG_LEVEL    (_LOG_LEVEL_EMERG | \
                                   _LOG_LEVEL_ERR | \
                                   _LOG_LEVEL_WARN | \
                                   _LOG_LEVEL_INFO | \
                                   _LOG_LEVEL_DEVICE | \
                                   _LOG_LEVEL_SYSTEM | \
                                   _LOG_LEVEL_TIMESTAMP | \
                                   _LOG_LEVEL_COLORS)

#define CONFIG_DEFAULT_LOG_LEVEL  ((CONFIG_BASIC_LOG_LEVEL | \
                                   _LOG_LEVEL_VERBOSE | \
                                   0) & ( ~( /* Remove Levels  */ \
                                   0)))

#define CONFIG_LOG_ALL_LEVEL      (CONFIG_DEFAULT_LOG_LEVEL | \
                                   _LOG_LEVEL_DEBUG | \
                                   _LOG_LEVEL_DEBUG_V1 | \
                                   _LOG_LEVEL_DEBUG_V2 | \
                                   _LOG_LEVEL_DEBUG_VV)

#define CONFIG_LOG_FILE_LEVEL     ((CONFIG_BASIC_LOG_LEVEL | \
                                   0) & ( ~( /* Remove Levels  */ \
                                   _LOG_LEVEL_COLORS | \
                                   0)))

#define _LOG_LEVEL_EMERG          (0x1 << 0)
#define _LOG_LEVEL_ERR            (0x1 << 1)
#define _LOG_LEVEL_WARN           (0x1 << 2)
#define _LOG_LEVEL_INFO           (0x1 << 3)
#define _LOG_LEVEL_VERBOSE        (0x1 << 4)
#define _LOG_LEVEL_DEBUG          (0x1 << 5)
#define _LOG_LEVEL_DEBUG_V1       (0x1 << 6)
#define _LOG_LEVEL_DEBUG_V2       (0x1 << 7)
#define _LOG_LEVEL_DEBUG_VV       (0x1 << 8)

#define _LOG_LEVEL_DEVICE         (0x1 << 9)
#define _LOG_LEVEL_SYSTEM         (0x1 << 10)
// #define _LOG_LEVEL_BT             (0x1 << 11)
// #define _LOG_LEVEL_MODEM          (0x1 << 12)
// #define _LOG_LEVEL_ESIM           (0x1 << 13)
// #define _LOG_LEVEL_WIFI           (0x1 << 14)
// #define _LOG_LEVEL_WEBUI          (0x1 << 15)
// #define _LOG_LEVEL_FOTA           (0x1 << 16)

#define _LOG_LEVEL_PREFIX         (0x1 << 20)
#define _LOG_LEVEL_TIMESTAMP      (0x1 << 21)
#define _LOG_LEVEL_COLORS         (0x1 << 22)

// #define log_x_bluetooth(...)      do_log_common(_LOG_LEVEL_BT,       __VA_ARGS__)
// #define log_x_modem(...)          do_log_common(_LOG_LEVEL_MODEM,    __VA_ARGS__)
// #define log_x_wifi(...)           do_log_common(_LOG_LEVEL_WIFI,     __VA_ARGS__)
// #define log_x_webui(...)          do_log_common(_LOG_LEVEL_WEBUI,    __VA_ARGS__)
// #define log_x_esim(...)           do_log_common(_LOG_LEVEL_ESIM,     __VA_ARGS__)
// #define log_x_fota(...)           do_log_common(_LOG_LEVEL_FOTA,     __VA_ARGS__)

#define log_never(...)            // Print Nothing

#define FC __FUNCTION__
#define LN __LINE__

#define log_emerg(...)            do_log_common(_LOG_LEVEL_EMERG,    FC, LN, __VA_ARGS__)
#define log_device(...)           do_log_common(_LOG_LEVEL_DEVICE,   FC, LN, __VA_ARGS__)
#define log_system(...)           do_log_common(_LOG_LEVEL_SYSTEM,   FC, LN, __VA_ARGS__)
#define log_err(...)              do_log_common(_LOG_LEVEL_ERR,      FC, LN, __VA_ARGS__)
#define log_warn(...)             do_log_common(_LOG_LEVEL_WARN,     FC, LN, __VA_ARGS__)
#define log_info(...)             do_log_common(_LOG_LEVEL_INFO,     FC, LN, __VA_ARGS__)
#define log_verbose(...)          do_log_common(_LOG_LEVEL_VERBOSE,  FC, LN, __VA_ARGS__)
#define log_debug(...)            do_log_common(_LOG_LEVEL_DEBUG,    FC, LN, __VA_ARGS__)
#define log_debug_v1(...)         do_log_common(_LOG_LEVEL_DEBUG_V1, FC, LN, __VA_ARGS__)
#define log_debug_v2(...)         do_log_common(_LOG_LEVEL_DEBUG_V2, FC, LN, __VA_ARGS__)
#define log_debug_vv(...)         do_log_common(_LOG_LEVEL_DEBUG_VV, FC, LN, __VA_ARGS__)

const char * log_get_version(void);

const char * log_get_help_info(void);

int do_log_common(unsigned int request_level, const char * func, int line, const char * log_fmt, ...);

void set_log_dir(char * dir_path);

/**
 * log_init: init the log service, you can use log_get_help_info
 * to get the support options argv
 * p_parse_fun: user can add self parse function to handle the args,
 *              or give NULL do nothing.
 *              the return val is: 0 = Ok , and the negtive return val will
 *  			return upper to log_init directly, which stop log run
 * Return Value:
 *  0           - run as a log service thread, use to product logs
 *  -1          - error parse, stop run log init
 *  negtive val - return from p_parse_fun, defined by user
 *  no return   - if there has a '--logcat' option ,it run as a logcat client,
 *                block here and exit the program until the end of life or be killed
*/
int log_init(int argc, char *argv[], int (*p_parse_fun)(int, char**));

#endif /* _LOG_OUT_H */
