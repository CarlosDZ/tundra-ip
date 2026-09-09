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
		struct ifinfomsg ifm;
	} req;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = RTM_GETLINK;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.ifm.ifi_family = AF_UNSPEC;

	if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
		perror("req send error");
		close(fd);
		return 1;
	}

	char buff[8192];
	ssize_t len;
	len = recv(fd, buff, sizeof(buff), 0);
	if (len < 0) {
		perror("recv error");
		close(fd);
		return 1;
	}

	struct nlmsghdr *nlh;
	nlh = (struct nlmsghdr *)buff;

	int count = 0;
	while (NLMSG_OK(nlh, len)) {
		if (nlh->nlmsg_type == NLMSG_DONE)
			break;
		if (nlh->nlmsg_type == RTM_NEWLINK) {
			struct ifinfomsg *ifm = NLMSG_DATA(nlh);
			struct rtattr *rta = IFLA_RTA(ifm);
			int rta_len = IFLA_PAYLOAD(nlh);

			while (RTA_OK(rta, rta_len)) {
				if (rta->rta_type == IFLA_IFNAME) {
					char *name = RTA_DATA(rta);
					printf("  Interface %d: %s\n", ifm->ifi_index, name);
				}
				rta = RTA_NEXT(rta, rta_len);
			}

			count++;
		}

		nlh = NLMSG_NEXT(nlh, len);
	}

	printf("tundra-ip: %d interfaces found\n", count);
}
