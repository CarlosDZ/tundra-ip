#include "netlink.h"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int netlink_open(void) {
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("socket opening error");
		return -1;
	}
	return fd;
}

int netlink_send_dump_req(int fd, int type, int family) {
	struct {
		struct nlmsghdr nlh;
		struct rtgenmsg gen;
	} req;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = type;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.gen.rtgen_family = family;

	if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
		perror("dump req send error");
		return -1;
	}
	return 0;
};

ssize_t netlink_recv_msg(int fd, char *buff, size_t bufflen) {
	ssize_t len = recv(fd, buff, bufflen, 0);
	if (len < 0) {
		perror("message recv error");
		return -1;
	}
	return len;
}
