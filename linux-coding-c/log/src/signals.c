/*
 * Signals process - Shujun, 2023
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>          // SIGxxxx types
#include <sys/stat.h>
#include <pthread.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>

#include "common.h"

#define tag "Signal"

static void quit_by_user(int signo)
{
	printf("%s: Quit by Signal: %d\n", tag, signo);
	safely_quit("Signal", 0);
}

static void stop_by_user(int signo)
{
	if (logcat_mode())
		return;

	if (signo == SIGTSTP) {
		log_emerg("%s: Stopped by SIGTSTP, WDT stopped!\n", tag);
		// wdt_stop();
		raise(SIGSTOP);   // Let the SIGSTOP to suspend myself
	} else if (signo == SIGCONT) {
		log_emerg("%s: Continued by SIGCONT, WDT started!\n", tag);
		// wdt_start();
	} else {
		log_err("%s: Unknown Signal: %d\n", tag, signo);
	}
}

static void pipe_broken(int signo)
{
	log_debug_v1("%s: PIPE broken: %d, Ignored!\n", tag, signo);
}

static void signal_hook(int signo, void (*sigfn)(int signo))
{
	struct sigaction act, oldact;
	int ret;

	act.sa_handler = sigfn;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	ret = sigaction(signo, &act, &oldact);
	if (ret < 0) {
		log_err("%s: sigaction(%d) failed with error %d (%s)\n", tag, signo, errno, strerror(errno));
	}
}

static void handle_exited_subprocess(int signo)
{
	pid_t pid;
	int stat;

	while (1) {
		pid = waitpid(-1, &stat, WNOHANG);
		if (pid <= 0)
			break;
		log_emerg("%s: Child quitted: %d, stat = %d\n", tag, pid, stat);
	}
}

void setup_signals(void)
{
	signal_hook(SIGHUP,  SIG_DFL);              // Session exit... Something bad may happen, Let the WDT fire...

	// Handle quitted child processes
	signal_hook(SIGCHLD, handle_exited_subprocess);

	signal_hook(SIGQUIT, quit_by_user);         // "Ctrl-\"
	signal_hook(SIGINT,  quit_by_user);         // "Ctrl-C"
	signal_hook(SIGTERM, quit_by_user);         // Default signal by 'kill'

	signal_hook(SIGTSTP, stop_by_user);         // "Ctrl-Z"
	signal_hook(SIGCONT, stop_by_user);         // "Ctrl-Z", recalled by 'fg'

	signal_hook(SIGPIPE, pipe_broken);          // pipe broken

	log_debug("%s: signals are ready.\n", tag);
}
