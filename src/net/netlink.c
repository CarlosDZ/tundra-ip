#include "netlink.h"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

int netlink_recv_dump(int fd, void (*callback)(struct nlmsghdr *nlh, void *ctx),
                      void *ctx) {
	char buf[8192];

	for (;;) {
		ssize_t len = recv(fd, buf, sizeof(buf), 0);
		if (len < 0) {
			perror("recv");
			return -1;
		}

		struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
		while (NLMSG_OK(nlh, len)) {
			if (nlh->nlmsg_type == NLMSG_DONE)
				return 0;

			if (nlh->nlmsg_type == NLMSG_ERROR) {
				fprintf(stderr, "netlink: error message\n");
				return -1;
			}

			callback(nlh, ctx);

			nlh = NLMSG_NEXT(nlh, len);
		}
	}
}

int netlink_dump(int type, int family,
                 void (*callback)(struct nlmsghdr *nlh, void *ctx), void *ctx) {
	int fd = netlink_open();
	if (fd < 0)
		return -1;

	if (netlink_send_dump_req(fd, type, family) < 0) {
		close(fd);
		return -1;
	}

	int ret = netlink_recv_dump(fd, callback, ctx);
	close(fd);
	return ret;
}

int netlink_add_attr(struct nlmsghdr *nlh, size_t maxlen, int type,
                     const void *data, int datalen) {
	int len = RTA_LENGTH(datalen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(nlh->nlmsg_len) + RTA_ALIGN(len) > maxlen)
		return -1;

	rta = (struct rtattr *)((char *)nlh + NLMSG_ALIGN(nlh->nlmsg_len));
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, datalen);

	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + RTA_ALIGN(len);
	return 0;
}

int netlink_send_change(struct nlmsghdr *nlh) {
	int fd = netlink_open();
	if (fd < 0)
		return -1;

	if (send(fd, nlh, nlh->nlmsg_len, 0) < 0) {
		perror("send");
		close(fd);
		return -1;
	}

	char buf[4096];
	ssize_t len = recv(fd, buf, sizeof(buf), 0);
	close(fd);

	if (len < 0) {
		perror("recv");
		return -1;
	}

	struct nlmsghdr *resp = (struct nlmsghdr *)buf;
	if (resp->nlmsg_type == NLMSG_ERROR) {
		struct nlmsgerr *err = NLMSG_DATA(resp);
		if (err->error != 0) {
			fprintf(stderr, "netlink: %s\n", strerror(-err->error));
			return -1;
		}
		return 0;
	}

	return 0;
}
