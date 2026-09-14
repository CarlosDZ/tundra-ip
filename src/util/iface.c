#include "iface.h"

#include "netlink.h"

#include <linux/rtnetlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void iface_table_init(struct iface_table *t) {
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}

int iface_table_add(struct iface_table *t, const struct iface *e) {
	if (t->count == t->capacity) {
		int new_cap = (t->capacity == 0) ? 8 : t->capacity * 2;
		struct iface *tmp = realloc(t->items, new_cap * sizeof(struct iface));
		if (tmp == NULL)
			return -1;
		t->items = tmp;
		t->capacity = new_cap;
	}
	t->items[t->count++] = *e;
	return 0;
}

const char *iface_table_lookup(struct iface_table *t, int index) {
	for (int i = 0; i < t->count; i++)
		if (t->items[i].index == index)
			return t->items[i].name;
	return NULL;
}

static void collect_iface(struct nlmsghdr *nlh, void *ctx) {
	if (nlh->nlmsg_type != RTM_NEWLINK)
		return;

	struct iface_table *t = ctx;
	struct ifinfomsg *ifm = NLMSG_DATA(nlh);
	struct rtattr *rta = IFLA_RTA(ifm);
	int rta_len = IFLA_PAYLOAD(nlh);

	struct iface e;
	memset(&e, 0, sizeof(e));
	e.index = ifm->ifi_index;
	e.type = ifm->ifi_type;
	e.flags = ifm->ifi_flags;
	e.operstate = -1;
	e.has_mac = 0;
	e.name[0] = '\0';

	while (RTA_OK(rta, rta_len)) {
		switch (rta->rta_type) {
		case IFLA_IFNAME:
			snprintf(e.name, sizeof(e.name), "%s", (char *)RTA_DATA(rta));
			break;
		case IFLA_OPERSTATE:
			e.operstate = *(unsigned char *)RTA_DATA(rta);
			break;
		case IFLA_ADDRESS:
			if (RTA_PAYLOAD(rta) == 6) {
				memcpy(e.mac, RTA_DATA(rta), 6);
				e.has_mac = 1;
			}
			break;
		}
		rta = RTA_NEXT(rta, rta_len);
	}

	iface_table_add(t, &e);
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

void iface_table_free(struct iface_table *t) {
	free(t->items);
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}
