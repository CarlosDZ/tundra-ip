#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
	int fd;
	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("socket error");
		return 1;
	}

	struct {
		struct nlmsghdr nlh;
		struct ifaddrmsg ifa;
	} req;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = RTM_GETADDR;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.ifa.ifa_family = AF_UNSPEC;

	if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
		perror("send error");
		close(fd);
		return 1;
	}

	char buf[8192];
	ssize_t len;
	len = recv(fd, buf, sizeof(buf), 0);
	if (len < 0) {
		perror("recv error");
		close(fd);
		return 1;
	}

	struct nlmsghdr *nlh;
	nlh = (struct nlmsghdr *)buf;

	while (NLMSG_OK(nlh, len)) {
		if (nlh->nlmsg_type == NLMSG_DONE)
			break;

		if (nlh->nlmsg_type == RTM_NEWADDR) {
			struct ifaddrmsg *ifa = NLMSG_DATA(nlh);
			struct rtattr *rta = IFA_RTA(ifa);
			int rta_len = IFA_PAYLOAD(nlh);

			while (RTA_OK(rta, rta_len)) {
				if (rta->rta_type == IFA_LOCAL) {
					char addr[INET6_ADDRSTRLEN];
					inet_ntop(ifa->ifa_family, RTA_DATA(rta), addr,
					          sizeof(addr));
					printf("  interface %d: %s/%d\n", ifa->ifa_index, addr,
					       ifa->ifa_prefixlen);
				}
				rta = RTA_NEXT(rta, rta_len);
			}
		}

		nlh = NLMSG_NEXT(nlh, len);
	}

	close(fd);
	return 0;
}
