#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <errno.h>
/**
 * 在应用程序中可以通过调用如下代码来判断连接是否断开
 */
int is_tcp_connected(int g_fd)
{
	int len;
	struct tcp_info info;

	getsockopt(g_fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	if ((info.tcpi_state == TCP_ESTABLISHED)) {
		printf("socket connected\n");
		return 0;
	}
	else {
		printf("===========socket disconnected");
		return -1;
	}
}
/**
 * 使用阻塞的socket, 可以设置读写超时,但是这个不会影响connect
struct timeval tv_timeout;
tv_timeout.tv_sec = 60;
tv_timeout.tv_usec = 0;
if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void *) &tv_timeout, sizeof(struct timeval)) < 0) {
    perror("setsockopt");
}
tv_timeout.tv_sec = 60;
tv_timeout.tv_usec = 0;
if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv_timeout, sizeof(struct timeval)) < 0) {
    perror("setsockopt");
}

man connect
EINPROGRESS
The socket is non-blocking and the connection cannot be com-
pleted immediately. It is possible to select(2) or poll(2) for
completion by selecting the socket for writing. After select(2)
indicates writability, use getsockopt(2) to read the SO_ERROR
option at level SOL_SOCKET to determine whether connect() com-
pleted successfully (SO_ERROR is zero) or unsuccessfully
(SO_ERROR is one of the usual error codes listed here, explain-
ing the reason for the failure).
首先设置socket fd为非阻塞, connect判断返回值, 如果返回0, 说明connect成功,
如果返回值等于-1并且错误的errno为EINPROGRESS时调用select或者poll判断socket fd的可写状态,
通过select或者poll的超时设置来判断是否超时.
*/
void tcp_connect_noblock(char *domain, int port)
{
    //文件标记
    int server_fd;
    //服务器的ip为本地，端口号
    struct sockaddr_in server_addr;

    //创建套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        printf("[%s;%d]:F Create socket fail: %s.\n",\
             __FUNCTION__, __LINE__, strerror(errno));
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(domain);
    server_addr.sin_port        = htons(port);

    if (0) {// 与下面一种对比设计no block
        //获取客户端文件标记
        int fl_flag = 0;
        fl_flag = fcntl(server_fd, F_GETFL, 0);
        //将客户端文件标记设置为非阻塞的方式
        fcntl(server_fd, F_SETFL, fl_flag | O_NONBLOCK);
    }

	int opt = 1;
	// set non-blocking
	if (ioctl(server_fd, FIONBIO, &opt) < 0) {
		close(server_fd);
		perror("ioctl");
		exit(0);
	}

	if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {

		if (errno == EINPROGRESS) {
			int error;
			int len = sizeof(int);
			tv_timeout.tv_sec = 60;
			tv_timeout.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET(server_fd, &set);
			if (select(server_fd + 1, NULL, &set, NULL, &tv_timeout) > 0) {
				getsockopt(server_fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
				if (error != 0) {
					close(server_fd);
					exit(0);
				}
			}
			else {	// timeout or select error
				close(server_fd);
				exit(0);
			}
		}
		else {
			close(server_fd);
			perror("connect");
			exit(0);
		}
	}

	opt = 0;
	// set blocking
	if (ioctl(server_fd, FIONBIO, &opt) < 0) {
		close(server_fd);
		perror("ioctl");
		exit(0);
	}
}
