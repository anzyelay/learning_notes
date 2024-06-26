#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

sigjmp_buf env;
void tfgets_handler(int sig)
{
  // signal(SIGALRM, SIG_DFL);
  signal(SIGALRM, SIG_IGN);
  siglongjmp(env, 1);
}

char *tfgets(char *buf, int bufsize, FILE *stream)
{
  static const int TimeLimitSecs = 5;
  signal(SIGALRM, tfgets_handler);
  alarm(TimeLimitSecs);
  int rc = sigsetjmp(env, 1);
  if (rc == 0) {
    char *ret = fgets(buf, bufsize, stream);
    signal(SIGALRM, SIG_IGN);
    return ret;
  }
  else {
    return NULL; // alarm，time out
  }
}
