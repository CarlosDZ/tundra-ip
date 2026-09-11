#include "link.h"
#include "netlink.h"

#include <linux/rtnetlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static void print_link(struct nlmsghdr *nlh) {
	if (nlh->nlmsg_type != RTM_NEWLINK)
		return;

	struct ifinfomsg *ifm = NLMSG_DATA(nlh);
	struct rtattr *rta = IFLA_RTA(ifm);
	int rta_len = IFLA_PAYLOAD(nlh);

	while (RTA_OK(rta, rta_len)) {
		if (rta->rta_type == IFLA_IFNAME) {
			char *name = RTA_DATA(rta);
			printf("%d: %s\n", ifm->ifi_index, name);
		}
		rta = RTA_NEXT(rta, rta_len);
	}
}

int link_show(void) {
	int fd = netlink_open();
	if (fd < 0)
		return -1;

	if (netlink_send_dump_req(fd, RTM_GETLINK, AF_UNSPEC) < 0) {
		close(fd);
		return -1;
	}

	int ret = netlink_recv_dump(fd, print_link);

	close(fd);
	return ret;
}
