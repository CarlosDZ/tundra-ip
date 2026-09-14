#include "routetable.h"
#include "netlink.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

const char *route_table_name(int table) {
	switch (table) {
	case RT_TABLE_MAIN:
		return "main";
	case RT_TABLE_LOCAL:
		return "local";
	case RT_TABLE_DEFAULT:
		return "default";
	default: {
		static char buf[16];
		snprintf(buf, sizeof(buf), "%d", table);
		return buf;
	}
	}
}

int route_table_shown(int table, int verbose, int local) {
	if (table == RT_TABLE_MAIN)
		return 1;
	if (table == RT_TABLE_LOCAL)
		return local;
	return verbose;
}

int route_family_shown(int family, int verbose) {
	if (family == AF_INET)
		return 1;
	if (family == AF_INET6)
		return verbose;
	return 0;
}

static void collect_route(struct nlmsghdr *nlh, void *ctx) {
	if (nlh->nlmsg_type != RTM_NEWROUTE)
		return;

	struct route_table *rt = ctx;
	struct rtmsg *rtm = NLMSG_DATA(nlh);

	struct route_entry e;
	e.table = rtm->rtm_table;
	e.family = rtm->rtm_family;
	e.dst_len = rtm->rtm_dst_len;
	e.proto = rtm->rtm_protocol;
	e.scope = rtm->rtm_scope;
	e.oif = -1;
	e.dst[0] = '\0';
	e.gw[0] = '\0';
	e.src[0] = '\0';
	e.has_metric = 0;
	e.metric = 0;

	struct rtattr *rta = RTM_RTA(rtm);
	int rta_len = RTM_PAYLOAD(nlh);

	while (RTA_OK(rta, rta_len)) {
		switch (rta->rta_type) {
		case RTA_DST:
			inet_ntop(e.family, RTA_DATA(rta), e.dst, sizeof(e.dst));
			break;
		case RTA_GATEWAY:
			inet_ntop(e.family, RTA_DATA(rta), e.gw, sizeof(e.gw));
			break;
		case RTA_PREFSRC:
			inet_ntop(e.family, RTA_DATA(rta), e.src, sizeof(e.src));
			break;
		case RTA_OIF:
			e.oif = *(int *)RTA_DATA(rta);
			break;
		case RTA_PRIORITY:
			e.metric = *(unsigned int *)RTA_DATA(rta);
			e.has_metric = 1;
			break;
		}
		rta = RTA_NEXT(rta, rta_len);
	}

	route_table_add(rt, &e);
}

int route_table_load(struct route_table *t) {
	return netlink_dump(RTM_GETROUTE, AF_UNSPEC, collect_route, t);
}

void route_table_init(struct route_table *t) {
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}

int route_table_add(struct route_table *t, const struct route_entry *e) {
	if (t->count == t->capacity) {
		int new_cap = (t->capacity == 0) ? 8 : t->capacity * 2;
		struct route_entry *tmp =
		    realloc(t->items, new_cap * sizeof(struct route_entry));
		if (tmp == NULL)
			return -1;
		t->items = tmp;
		t->capacity = new_cap;
	}
	t->items[t->count++] = *e;
	return 0;
}

void route_table_free(struct route_table *t) {
	free(t->items);
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}
