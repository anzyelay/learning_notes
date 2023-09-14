#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/fcntl.h>

#define ANZYE_NL_MSG_MAX_LEN 16384
#define NLMSG_SEQ 1
#define ANZYE_NL_GETNEIGH_SEQ 334455
#define ANZYE_NL_PID (getpid() & 0x7FFFFFFF)
#define ANZYE_NL_SOCK_PORTID ANZYE_NL_PID + ANZYE_NL_GETNEIGH_SEQ
#define ANZYE_NL_FAILURE ANZYE_NL_FAILURE
#define ANZYE_NL_SUCCESS 0
#define ANZYE_GET_IFINDEX 1
#define ANZYE_GET_IFNAME 2

#define ANZYE_NDA_RTA(r) ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#define ANZYE_IFA_RTA(r) ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#define ANZYE_RTN_RTA(r) ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct rtmsg))))

typedef struct {
	struct nlmsghdr hdr;
	struct rtgenmsg gen;
} nl_req_type;

struct msghdr *anzye_nl_alloc_msg(uint32_t msglen)
{
	unsigned char *buf = NULL;
	struct sockaddr_nl *nladdr = NULL;
	struct iovec *iov = NULL;
	struct msghdr *msgh = NULL;

	/*-------------------------------------------------------------------------*/

	if (ANZYE_NL_MSG_MAX_LEN < msglen) {
		LOG_MSG_ERROR("Netlink message exceeds maximum length", 0, 0, 0);
		return NULL;
	}

	if ((msgh = malloc(sizeof(struct msghdr))) == NULL) {
		LOG_MSG_ERROR("Failed malloc for msghdr", 0, 0, 0);
		return NULL;
	}

	if ((nladdr = malloc(sizeof(struct sockaddr_nl))) == NULL) {
		LOG_MSG_ERROR("Failed malloc for sockaddr", 0, 0, 0);
		free(msgh);
		return NULL;
	}

	if ((iov = malloc(sizeof(struct iovec))) == NULL) {
		LOG_MSG_ERROR("Failed malloc for iovec", 0, 0, 0);
		free(nladdr);
		free(msgh);
		return NULL;
	}

	if ((buf = malloc(msglen)) == NULL) {
		LOG_MSG_ERROR("Failed malloc for buffer to store netlink message", 0, 0, 0);
		free(iov);
		free(nladdr);
		free(msgh);
		return NULL;
	}

	memset(nladdr, 0, sizeof(struct sockaddr_nl));
	nladdr->nl_family = AF_NETLINK;

	memset(msgh, 0x0, sizeof(struct msghdr));
	msgh->msg_name = nladdr;
	msgh->msg_namelen = sizeof(struct sockaddr_nl);
	msgh->msg_iov = iov;
	msgh->msg_iovlen = 1;

	memset(iov, 0x0, sizeof(struct iovec));
	iov->iov_base = buf;
	iov->iov_len = msglen;

	return msgh;
}
/**
* @brief
  calls a system ioctl to get the interface details for the corresponding
  passed interface name
* @return VALUE
  ANZYE_NL_SUCCESS on success
  ANZYE_NL_FAILURE on failure
*/
int anzye_nl_get_interface_details(char *if_name, int *if_index, int interface_attribute)
{
	int fd;
	struct ifreq ifr;

	/*-------------------------------------------------------------------------
	  Open a socket which is required to call an ioctl
	--------------------------------------------------------------------------*/
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("get interface index socket create failed\n");
		return ANZYE_NL_FAILURE;
	}

	memset(&ifr, 0, sizeof(struct ifreq));

	if (interface_attribute == ANZYE_GET_IFINDEX) {
		/*-------------------------------------------------------------------------
			Populate the passed interface name
		  --------------------------------------------------------------------------*/
		strlcpy((char *)ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
		/*-------------------------------------------------------------------------
			Call the ioctl to get the interface index
		   --------------------------------------------------------------------------*/
		if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
			printf("call_ioctl_on_dev; ioctl failed error:%s\n", strerror(errno), 0, 0);

			close(fd);
			return ANZYE_NL_FAILURE;
		}
		*if_index = ifr.ifr_ifindex;
	}
	else {
		/*-------------------------------------------------------------------------
			Populate the passed interface name
		   --------------------------------------------------------------------------*/
		ifr.ifr_ifindex = *if_index;
		/*-------------------------------------------------------------------------
			Call the ioctl to get the interface index
		   --------------------------------------------------------------------------*/
		if (ioctl(fd, SIOCGIFNAME, &ifr) < 0) {
			printf("call_ioctl_on_dev; ioctl failed\
                     error:%s\n",
				   strerror(errno), 0, 0);

			close(fd);
			return ANZYE_NL_FAILURE;
		}
		(void)strlcpy(if_name, ifr.ifr_name, sizeof(ifr.ifr_name));
	}

	close(fd);
	return ANZYE_NL_SUCCESS;
}

/**
 * @brief: sends RTM message to kernel
 * @return: ANZYE_NL_FAILURE: failed; 0:success;
 */
int netlink_query_if(int sk_fd, unsigned int nl_groups)
{
	struct msghdr *nl_msg_hdr = NULL;
	nl_req_type *nl_req = NULL;
	ssize_t sndbytes = 0;
	/*------------------------------------------------------------------------*/
	/*-------------------------------------------------------------------------
	  Send RTM_GETNEIGH to find if RNDIS/ECM interfaces are running
	-------------------------------------------------------------------------*/
	nl_msg_hdr = anzye_nl_alloc_msg(ANZYE_NL_MSG_MAX_LEN);
	if (nl_msg_hdr == NULL) {
		printf("anzye_nl_query_if(): Failed in anzye_nl_alloc_msg\n");
		return ANZYE_NL_FAILURE;
	}
	nl_req = (nl_req_type *)(nl_msg_hdr->msg_iov->iov_base);

	/*-------------------------------------------------------------------------
	  Populate the required parameters in the netlink message
	---------------------------------------------------------------------------*/
	nl_req->hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
	if (nl_groups & RTMGRP_NEIGH) {
		nl_req->hdr.nlmsg_type = RTM_GETNEIGH;
	}
	if (nl_groups & RTMGRP_IPV6_ROUTE) {
		nl_req->hdr.nlmsg_type |= RTM_GETROUTE;
	}
	if (nl_groups & RTMGRP_LINK) {
		nl_req->hdr.nlmsg_type |= RTM_GETLINK;
	}
	/* NLM_F_REQUEST - has to be set for request messages
	   NLM_F_DUMP -  equivalent to NLM_F_ROOT|NLM_F_MATCH */
	nl_req->hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nl_req->hdr.nlmsg_seq = NLMSG_SEQ;
	nl_req->hdr.nlmsg_pid = 0;
	nl_req->gen.rtgen_family = AF_PACKET;

	if (sendmsg(sk_fd, (struct msghdr *)nl_msg_hdr, 0) <= 0) {
		printf("IOCTL Send Failed errno:%d, send GETNEIGH failed\n", errno);
		return ANZYE_NL_FAILURE;
	}

	free(nl_msg_hdr->msg_iov->iov_base);
	free(nl_msg_hdr->msg_iov);
	free(nl_msg_hdr->msg_name);
	free(nl_msg_hdr);

	return ANZYE_NL_SUCCESS;
}
/**
 * @brief: Sends a RTM_NEWNEIGH message for bridge iface.
* @return VALUE
  ANZYE_NL_SUCCESS on success
  ANZYE_NL_FAILURE on failure
*/
int anzye_nl_send_getneigh_event(int fd)
{
	ssize_t sndbytes = 0;
	int ifindex = ANZYE_NL_FAILURE;
	struct {
		struct nlmsghdr n;
		struct ndmsg r;
	} req;

	printf("%s %d \n", __func__, __LINE__);

	if (anzye_nl_get_interface_details("bridge0", &ifindex, ANZYE_GET_IFINDEX) ==
		ANZYE_NL_SUCCESS) {
		printf("Bridge Iface Index = %d\n", ifindex);
	}
	else {
		printf("Bridge Iface retrival error Index = %d\n", ifindex);
		return ANZYE_NL_FAILURE;
	}

	/* Send the GETNEIGH message to the kernel*/
	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT | NLM_F_REQUEST;
	req.n.nlmsg_seq = ANZYE_NL_GETNEIGH_SEQ;
	req.n.nlmsg_pid = ANZYE_NL_SOCK_PORTID;
	req.n.nlmsg_type = RTM_GETNEIGH;
	req.r.ndm_state = NUD_REACHABLE | NUD_STALE | NUD_DELAY;
	req.r.ndm_family = AF_INET6;
	req.r.ndm_ifindex = ifindex;

	printf(" Dump getneigh len=%d , flags =%d, seq=%d, pid=%d, type=%d, family=%d ifindex %d\n",
		   req.n.nlmsg_len, req.n.nlmsg_flags, req.n.nlmsg_seq, req.n.nlmsg_pid, req.n.nlmsg_type,
		   req.r.ndm_family, req.r.ndm_ifindex);
	sndbytes = send(fd, &req, req.n.nlmsg_len, 0);

	if (sndbytes < 0) {
		printf("Send GETNEIGH failed: %s \n", strerror(errno));
		return ANZYE_NL_FAILURE;
	}
	else {
		printf("Send GETNEIGH succeded: %d bytes send \n", sndbytes);
	}

	return ANZYE_NL_SUCCESS;
}

int netlink_open_socket(unsigned int groups)
{
	struct sockaddr_nl addr;
	int nl_skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (nl_skfd < 0) {
		printf("can not open netlink socket\n");
		return nl_skfd;
	}

	memset(&addr, 0, sizeof(struct sockaddr_nl));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = groups;
	addr.nl_pid = ANZYE_NL_PID;

	if (bind(nl_skfd, &addr, sizeof(struct sockaddr_nl)) < 0) {
		printf("socket bind failed\n");
		close(nl_skfd);
		nl_skfd = -1;
	}

	return nl_skfd;
}
/**
 * @brief:
 * - decodes the Netlink Neighbour message
 */
int anzye_nl_decode_rtm_neigh(const char *buffer, unsigned int buflen)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *)buffer; /* NL message header */
	struct rtattr *rtah = NULL;
	struct ndmsg metainfo; /* from header */
	struct				   /* attributes  */
	{
		unsigned int param_mask;
		struct sockaddr_storage local_addr;
		struct sockaddr lladdr_hwaddr;
	} attr_info;
	char dev_name[16];

	/* Extract the header data */
	metainfo = *((struct ndmsg *)NLMSG_DATA(nlh));
	/* Subtracting NL Header to decode RT MSG*/
	buflen -= sizeof(struct nlmsghdr);

	/* Extract the available attributes */
	attr_info.param_mask = 0x0000;

	rtah = ANZYE_NDA_RTA(NLMSG_DATA(nlh));

	while (RTA_OK(rtah, buflen)) {
		switch (rtah->rta_type) {
			case NDA_DST:
				// ip family like: ipv4 ,ipv6
				attr_info.local_addr.ss_family = metainfo.ndm_family;
				// ip address
				memcpy(&attr_info.local_addr.__ss_padding, RTA_DATA(rtah),
					   sizeof(attr_info.local_addr.__ss_padding));
				break;
			case NDA_LLADDR:
				// mac address
				memcpy(attr_info.lladdr_hwaddr.sa_data, RTA_DATA(rtah),
					   sizeof(attr_info.lladdr_hwaddr.sa_data));
				break;
			default:
				break;
		}

		/* Advance to next attribute */
		rtah = RTA_NEXT(rtah, buflen);
	}

	anzye_nl_get_interface_details(dev_name, metainfo.ndm_ifindex, ANZYE_GET_IFNAME);
	printf("the netlink msg is recved from dev: %s\n", dev_name);

	return ANZYE_NL_SUCCESS;
}
int anzye_nl_decode_rtm_addr(const char *buffer, unsigned int buflen,
							 anzye_nl_if_addr_t *if_addr_info)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *)buffer; /* NL message header */
	struct rtattr *rtah = NULL;
	struct ifaddrmsg metainfo; /*from header*/
	struct					   /*attributes*/
	{
		struct in_addr ifa_local_v4;
		struct in6_addr ifa_local_v6;
		struct sockaddr_storage ip6_addr;
		uint8_t prefix_len;
	} attr_info;
	/* Extract the header data */
	metainfo = *((struct ifaddrmsg *)NLMSG_DATA(nlh));

	if (metainfo.ifa_family == AF_INET) {
		printf("RTM_NEWADDR received for IPv4\n");
	}
	else if (metainfo.ifa_family == AF_INET6) {
		printf("Received RTM_NEWADDR for IPv6\n");
	}
	else {
		printf("Received RTM_NEWADDR for unknown family\n");
		return ANZYE_NL_FAILURE;
	}

	/* Subtracting NL Header to decode RT MSG*/
	buflen -= sizeof(struct nlmsghdr);
	rtah = ANZYE_IFA_RTA(NLMSG_DATA(nlh));
	/*Setting ifa_local to zero.*/
	memset(&attr_info, 0, sizeof(attr_info));

	while (RTA_OK(rtah, buflen)) {
		switch (rtah->rta_type) {
			case IFA_LOCAL:
				if (metainfo.ifa_family == AF_INET)
					attr_info.ifa_local_v4 = *((struct in_addr *)RTA_DATA(rtah));
				break;
			case IFA_ADDRESS: /* For Global IPv6 Address */
				if ((metainfo.ifa_family != AF_INET6) || (metainfo.ifa_scope != RT_SCOPE_UNIVERSE))
					break;
				else if (metainfo.ifa_family == AF_INET6)
					memcpy(&attr_info.ifa_local_v6, RTA_DATA(rtah), sizeof(struct in6_addr));
				break;
			default:
				break;
		}
		/* Advance to next attribute */
		rtah = RTA_NEXT(rtah, buflen);
	}
	return ANZYE_NL_SUCCESS;
}
/**
 * @brief:
  - decodes the Netlink Route message
*/
int anzye_nl_decode_rtm_route(const char *buffer, unsigned int buflen)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *)buffer; /* NL message header */
	struct rtattr *rtah = NULL;
	struct rtmsg metainfo;
	struct /* attributes  */
	{
		unsigned int param_mask;
		struct in6_addr dst_addr;
		int ifindex;
	} attr_info;
	/* Extract the header data */
	metainfo = *((struct rtmsg *)NLMSG_DATA(nlh));
	/* Subtracting NL Header to decode RT MSG*/
	buflen -= sizeof(struct nlmsghdr);

	attr_info.param_mask = 0;

	if (metainfo.rtm_protocol != RTPROT_STATIC) {
		/* Ignore the route silently. */
		return ANZYE_NL_SUCCESS;
	}

	rtah = ANZYE_RTN_RTA(NLMSG_DATA(nlh));

	while (RTA_OK(rtah, buflen)) {
		switch (rtah->rta_type) {
			case RTA_DST:
				memcpy(&attr_info.dst_addr, RTA_DATA(rtah), sizeof(attr_info.dst_addr));
				attr_info.param_mask |= ANZYE_NL_ROUTE_INFO_DST_ADDR;
				break;
			case RTA_OIF:
				memcpy(&attr_info.ifindex, RTA_DATA(rtah), sizeof(attr_info.ifindex));
				attr_info.param_mask |= ANZYE_NL_ROUTE_INFO_IFINDEX;
				break;
			default:
				break;
		}

		/* Advance to next attribute */
		rtah = RTA_NEXT(rtah, buflen);
	}

	return ANZYE_NL_SUCCESS;
}
void netlink_decode_nlmsg(unsigned int fd)
{
	struct msghdr msgh;
	int rmsgl;
	// Receive message over the socket
	rmsgl = recvmsg(fd, &msgh, 0);
	if (rmsgl <= 0) {
		printf("recv netlink msg failed\n");
		return;
	}
	// verify that NL address length in the recieved message is expected value
	if (msgh.msg_namelen != sizeof(struct sockaddr_nl)) {
		printf("recved msg with error namelen\n");
		return;
	}
	// verify that message was not truncated. this should not occur
	if (msgh.msg_flags & MSG_TRUNC) {
		printf("recved msg truncated!\n");
		return;
	}

	struct nlmsghdr *nlh = (struct nlmsghdr *)msgh.msg_iov->iov_base;
	unsigned buflen = rmsgl;
	while (NLMSG_OK(nlh, buflen)) {
		switch (nlh->nlmsg_type) {
			case RTM_NEWNEIGH:
				anzye_nl_decode_rtm_neigh((char *)nlh, buflen);
				break;
			case RTM_DELNEIGH:
				anzye_nl_decode_rtm_neigh((char *)nlh, buflen);
				break;
			case RTM_NEWADDR:
				anzye_nl_decode_rtm_addr((char *)nlh, buflen);
				break;
			case RTM_NEWROUTE:
				anzye_nl_decode_rtm_route((char *)nlh, buflen);
				break;
			default:
				break;
		}
		nlh = NLMSG_NEXT(nlh, buflen);
	}
}

int main(void)
{
	fd_set sets;
	int num_max;
	unsigned int groups =
		RTMGRP_NEIGH | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFADDR;
	int nl_fd = netlink_open_socket(groups);
	if (nl_fd < 0) {
		return 0
	}

	/* -------------------------------------------------------------------------
	Query the kernel about the current Neighbors by sending RTM_GETNEIGH.
	This is useful for the interfaces if the USB is plugged in and then powered up
	Its Just a safe guard, since DHCP addresses wouldnt be assigned until DHCP server is bought
	up
	--------------------------------------------------------------------------*/
	netlink_query_if(nl_fd, groups);

	while (TRUE) {
		FD_ZERO(sets);
		FD_SET(nl_fd, &sets);
		if ((select(nl_fd + 1, &sets, NULL, NULL, NULL)) < 0)
			continue;

		if (FD_ISSET(nl_fd, &sets)) {
			netlink_decode_nlmsg(nl_fd);
		}

		FD_CLR(nl_fd, &sets);
	}

	close(nl_fd);
}
