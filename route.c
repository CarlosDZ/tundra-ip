#include "route.h"
#include "netlink.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static void print_route(struct nlmsghdr *nlh, void *ctx) {
	(void)ctx;

	if (nlh->nlmsg_type != RTM_NEWROUTE)
		return;

	struct rtmsg *rtm = NLMSG_DATA(nlh);

	if (rtm->rtm_family != AF_INET)
		return;

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

	printf("%s/%d", dst, rtm->rtm_dst_len);
	if (gw[0] != '\0')
		printf(" via %s", gw);
	if (oif != -1)
		printf(" oif %d", oif);
	printf("\n");
}

int route_show(void) {
	int fd = netlink_open();
	if (fd < 0)
		return -1;

	if (netlink_send_dump_req(fd, RTM_GETROUTE, AF_UNSPEC) < 0) {
		close(fd);
		return -1;
	}

	int ret = netlink_recv_dump(fd, print_route, NULL);

	close(fd);
	return ret;
}
