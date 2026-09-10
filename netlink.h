#ifndef TUNDRA_NETLINK_H
#define TUNDRA_NETLINK_H

#include <sys/types.h>

int netlink_open(void);
int netlink_send_dump_req(int fd, int type, int family);
ssize_t netlink_recv_msg(int fd, char *buff, size_t bufflen);

#endif
