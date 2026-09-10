#include "netlink.h"

#include <linux/netlink.h>
#include <stdio.h>
#include <sys/socket.h>

int netlink_open(void) {
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("socket error");
		return -1;
	}
	return fd;
}
