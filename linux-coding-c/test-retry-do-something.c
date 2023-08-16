#include <stdio.h>
#include <errno.h>
/* rc is return from sendto and friends.
   Return 1 if we should retry.
   Set errno to zero if we succeeded. 
   Eg:   while(retry_send(sendto(.*))); 
*/
int retry_send(ssize_t rc)
{
	static int retries = 0;
	struct timespec waiter;

	if (rc != -1) {
		retries = 0;
		errno = 0;
		return 0;
	}

	/* Linux kernels can return EAGAIN in perpetuity when calling
	   sendmsg() and the relevant interface has gone. Here we loop
	   retrying in EAGAIN for 1 second max, to avoid this hanging
	   dnsmasq. */

	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		waiter.tv_sec = 0;
		waiter.tv_nsec = 10000;
		nanosleep(&waiter, NULL);
		if (retries++ < 1000)
			return 1;
	}

	retries = 0;

	if (errno == EINTR)
		return 1;

	return 0;
}
