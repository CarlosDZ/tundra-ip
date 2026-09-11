#include "iface.h"
#include "netlink.h"

#include <linux/rtnetlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void iface_table_init(struct iface_table *t) {
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}

int iface_table_add(struct iface_table *t, int index, const char *name) {
	if (t->count == t->capacity) {
		int new_cap = (t->capacity == 0) ? 8 : t->capacity * 2;
		struct iface *tmp = realloc(t->items, new_cap * sizeof(struct iface));
		if (tmp == NULL)
			return -1;
		t->items = tmp;
		t->capacity = new_cap;
	}

	t->items[t->count].index = index;
	snprintf(t->items[t->count].name, sizeof(t->items[t->count].name), "%s",
	         name);
	t->count++;
	return 0;
}

const char *iface_table_lookup(struct iface_table *t, int index) {
	for (int i = 0; i < t->count; i++)
		if (t->items[i].index == index)
			return t->items[i].name;
	return NULL;
}

void iface_table_free(struct iface_table *t) {
	free(t->items);
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}

static void collect_iface(struct nlmsghdr *nlh, void *ctx) {
	if (nlh->nlmsg_type != RTM_NEWLINK)
		return;

	struct iface_table *t = ctx;

	struct ifinfomsg *ifm = NLMSG_DATA(nlh);
	struct rtattr *rta = IFLA_RTA(ifm);
	int rta_len = IFLA_PAYLOAD(nlh);

	while (RTA_OK(rta, rta_len)) {
		if (rta->rta_type == IFLA_IFNAME) {
			iface_table_add(t, ifm->ifi_index, RTA_DATA(rta));
			return;
		}
		rta = RTA_NEXT(rta, rta_len);
	}
}

int iface_table_load(struct iface_table *t) {
	int fd = netlink_open();
	if (fd < 0)
		return -1;

	if (netlink_send_dump_req(fd, RTM_GETLINK, AF_UNSPEC) < 0) {
		close(fd);
		return -1;
	}

	int ret = netlink_recv_dump(fd, collect_iface, t);

	close(fd);
	return ret;
}
