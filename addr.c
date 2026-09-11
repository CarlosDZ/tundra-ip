#include "addr.h"
#include "netlink.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static void print_addr(struct nlmsghdr *nlh) {
	if (nlh->nlmsg_type != RTM_NEWADDR)
		return;

	struct ifaddrmsg *ifa = NLMSG_DATA(nlh);
	struct rtattr *rta = IFA_RTA(ifa);
	int rta_len = IFA_PAYLOAD(nlh);

	while (RTA_OK(rta, rta_len)) {
		if (rta->rta_type == IFA_LOCAL) {
			char addr[INET6_ADDRSTRLEN];
			inet_ntop(ifa->ifa_family, RTA_DATA(rta), addr, sizeof(addr));
			printf("%d: %s/%d\n", ifa->ifa_index, addr, ifa->ifa_prefixlen);
		}
		rta = RTA_NEXT(rta, rta_len);
	}
}

int addr_show(void) {
	int fd = netlink_open();
	if (fd < 0)
		return -1;

	if (netlink_send_dump_req(fd, RTM_GETADDR, AF_UNSPEC) < 0) {
		close(fd);
		return -1;
	}

	int ret = netlink_recv_dump(fd, print_addr);

	close(fd);
	return ret;
}
