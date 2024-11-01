/*
 * APIs or Macros for Parsing command line arguments - Shujun, 2023
 */
#ifndef _PARSE_CMDLINE_H
#define _PARSE_CMDLINE_H

#ifdef __cplusplus
extern "C" {
#endif

#define parse_arg_int(cmd, var, min, max) \
	else if (!strncmp(arg, (cmd), strlen((cmd)))) { \
		var = atoi(arg + strlen((cmd))); \
		if (var < (min)) { \
			usage(cmd " is too small", NULL); \
			return -1; \
		} \
		if (var > (max)) { \
			usage(cmd " is too large", NULL); \
			return -1; \
		} \
	}

#define parse_arg_xint(cmd, var, min, max) \
	else if (!strncmp(arg, (cmd), strlen((cmd)))) { \
		char *__arg = arg + strlen((cmd)); \
		if (__arg[0] == '0' && (__arg[1] == 'x' || __arg[1] == 'X')) \
			__arg += 2; \
		if (sscanf(__arg, "%x", &var) != 1) { \
			usage(cmd " needs a hex number", NULL); \
			return -1; \
		} \
		if (var < (min)) { \
			usage(cmd " is too small", NULL); \
			return -1; \
		} \
		if (var > (max)) { \
			usage(cmd " is too large", NULL); \
			return -1; \
		} \
	}

#define parse_arg_string(cmd, var, check_null) \
	else if (!strncmp(arg, (cmd), strlen((cmd)))) { \
		var = arg + strlen((cmd)); \
		if (check_null && strlen(var) == 0) { \
			usage(cmd " should NOT be empty!", NULL); \
			return -1; \
		} \
	}

#define parse_arg_string_cpy(cmd, var, check_null) \
	else if (!strncmp(arg, (cmd), strlen((cmd)))) { \
		char *tmp = arg + strlen((cmd)); \
		memset(var,0,sizeof(var));\
		memcpy(var,tmp,strlen(tmp));\
		if (check_null && strlen(tmp) == 0) { \
			usage(cmd " should NOT be empty!", NULL); \
			return -1; \
		} \
	}

#define parse_arg_set(cmd, var, val) \
	else if (!strcmp(arg, (cmd))) { \
		var = val; \
	}

void usage(const char * err, const char * err_param);
int parse_args(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* _PARSE_CMDLINE_H */
