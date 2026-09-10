#include "netlink.h"

#include <stdio.h>
#include <unistd.h>

int main(void) {
	int fd = netlink_open();
	if (fd < 0)
		return 1;

	printf("tundra-ip: netlink ready, fd=%d\n", fd);

	close(fd);
	return 0;
}
