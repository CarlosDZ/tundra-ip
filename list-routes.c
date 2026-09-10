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
		struct rtmsg rtm;
	} req;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = RTM_GETROUTE;
	req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.rtm.rtm_family = AF_UNSPEC;

	if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
		perror("send error");
		close(fd);
		return 1;
	}

	char buf[16384];
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

		if (nlh->nlmsg_type == RTM_NEWROUTE) {
			struct rtmsg *rtm = NLMSG_DATA(nlh);

			if (rtm->rtm_family != AF_INET) {
				nlh = NLMSG_NEXT(nlh, len);
				continue;
			}

			struct rtattr *rta = RTM_RTA(rtm);
			int rta_len = RTM_PAYLOAD(nlh);

			char dst[INET_ADDRSTRLEN] = "default";
			char gw[INET_ADDRSTRLEN] = "";
			int oif = -1;

			while (RTA_OK(rta, rta_len)) {
				if (rta->rta_type == RTA_DST)
					inet_ntop(AF_INET, RTA_DATA(rta), dst, sizeof(dst));
				else if (rta->rta_type == RTA_GATEWAY)
					inet_ntop(AF_INET, RTA_DATA(rta), gw, sizeof(gw));
				else if (rta->rta_type == RTA_OIF)
					oif = *(int *)RTA_DATA(rta);

				rta = RTA_NEXT(rta, rta_len);
			}

			printf("  %s/%d", dst, rtm->rtm_dst_len);
			if (gw[0] != '\0')
				printf(" via %s", gw);
			if (oif != -1)
				printf(" dev-index %d", oif);
			printf("\n");
		}

		nlh = NLMSG_NEXT(nlh, len);
	}

	close(fd);
	return 0;
}
