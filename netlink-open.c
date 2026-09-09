#include <linux/netlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
	int fd;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("socket");
		return 1;
	}

	printf("tundra-ip: netlink socket open, fd=%d\n", fd);

	close(fd);
	return 0;
}
