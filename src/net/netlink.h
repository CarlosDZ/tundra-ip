#ifndef TUNDRA_NETLINK_H
#define TUNDRA_NETLINK_H

#include <linux/netlink.h>
#include <stddef.h>
#include <sys/types.h>

int netlink_open(void);
int netlink_send_dump_req(int fd, int type, int family);
ssize_t netlink_recv_msg(int fd, char *buff, size_t bufflen);

int netlink_recv_dump(int fd, void (*callback)(struct nlmsghdr *nlh, void *ctx),
                      void *ctx);

int netlink_dump(int type, int family,
                 void (*callback)(struct nlmsghdr *nlh, void *ctx), void *ctx);

int netlink_send_change(struct nlmsghdr *nlh);
int netlink_add_attr(struct nlmsghdr *nlh, size_t maxlen, int type,
                     const void *data, int datalen);

#endif
