#include "addrtable.h"
#include "netlink.h"

#include <arpa/inet.h>
#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <sys/socket.h>

static void collect_addr(struct nlmsghdr *nlh, void *ctx) {
	if (nlh->nlmsg_type != RTM_NEWADDR)
		return;

	struct addr_table *at = ctx;
	struct ifaddrmsg *ifa = NLMSG_DATA(nlh);
	struct rtattr *rta = IFA_RTA(ifa);
	int rta_len = IFA_PAYLOAD(nlh);

	struct addr_entry e;
	e.ifindex = ifa->ifa_index;
	e.prefixlen = ifa->ifa_prefixlen;
	e.scope = ifa->ifa_scope;
	e.flags = ifa->ifa_flags;
	e.ip[0] = '\0';
	e.broadcast[0] = '\0';
	e.valid_lft = 0;
	e.preferred_lft = 0;
	int have_ip = 0;

	while (RTA_OK(rta, rta_len)) {
		if (rta->rta_type == IFA_LOCAL) {
			inet_ntop(ifa->ifa_family, RTA_DATA(rta), e.ip, sizeof(e.ip));
			have_ip = 1;
		} else if (rta->rta_type == IFA_FLAGS) {
			e.flags = *(unsigned int *)RTA_DATA(rta);
		} else if (rta->rta_type == IFA_BROADCAST) {
			inet_ntop(ifa->ifa_family, RTA_DATA(rta), e.broadcast,
			          sizeof(e.broadcast));
		} else if (rta->rta_type == IFA_CACHEINFO) {
			struct ifa_cacheinfo *ci = RTA_DATA(rta);
			e.valid_lft = ci->ifa_valid;
			e.preferred_lft = ci->ifa_prefered;
		}
		rta = RTA_NEXT(rta, rta_len);
	}

	if (have_ip)
		addr_table_add(at, &e);
}

int addr_table_load(struct addr_table *t) {
	return netlink_dump(RTM_GETADDR, AF_UNSPEC, collect_addr, t);
}

void addr_table_init(struct addr_table *t) {
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}

int addr_table_add(struct addr_table *t, const struct addr_entry *e) {
	if (t->count == t->capacity) {
		int new_cap = (t->capacity == 0) ? 8 : t->capacity * 2;
		struct addr_entry *tmp =
		    realloc(t->items, new_cap * sizeof(struct addr_entry));
		if (tmp == NULL)
			return -1;
		t->items = tmp;
		t->capacity = new_cap;
	}
	t->items[t->count++] = *e;
	return 0;
}

void addr_table_free(struct addr_table *t) {
	free(t->items);
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}
