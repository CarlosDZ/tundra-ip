#include "route.h"
#include "../util/color.h"
#include "../util/iface.h"
#include "../util/routetable.h"
#include "../util/rtnames.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <netlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *table_name(int table) {
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

static int route_visible(const struct route_entry *r, int idx, int only_table,
                         int verbose, int local) {
	if (r->oif != idx)
		return 0;
	if (!route_family_shown(r->family, verbose))
		return 0;
	if (!route_table_shown(r->table, verbose, local))
		return 0;
	if (only_table != -1 && r->table != only_table)
		return 0;
	return 1;
}

static void print_one_route(const struct route_entry *r, int verbose,
                            const char *indent) {
	char metric[16];
	if (r->has_metric)
		snprintf(metric, sizeof(metric), "[%u]", r->metric);
	else
		snprintf(metric, sizeof(metric), "[-]");

	int metric_len = strlen(metric);
	int metric_width = 7; /* ancho de columna para la metrica */

	printf("%s\t%s%s%s", indent, c_cyan, metric, c_reset);
	for (int i = metric_len; i < metric_width; i++)
		putchar(' ');
	putchar(' ');

	char buf[128];
	int n = 0;
	if (r->dst[0] == '\0')
		n += snprintf(buf + n, sizeof(buf) - n, "default");
	else
		n += snprintf(buf + n, sizeof(buf) - n, "%s/%d", r->dst, r->dst_len);
	if (r->gw[0] != '\0')
		n += snprintf(buf + n, sizeof(buf) - n, " via %s", r->gw);

	if (r->dst[0] == '\0')
		printf("%s%s%s%s", c_green, "default", c_reset,
		       buf + strlen("default"));
	else
		printf("%s", buf);

	if (verbose) {
		int pad = (n < 36) ? 36 - n : 1;
		for (int i = 0; i < pad; i++)
			putchar(' ');
		printf("[proto %s] [scope %s]", proto_name(r->proto),
		       scope_name(r->scope));
		if (r->src[0] != '\0')
			printf(" [src %s]", r->src);
	}

	printf("\n");
}

static void print_no_iface(int only_table, struct route_table *routes,
                           int verbose, int local, const char *indent) {
	int has = 0;
	for (int j = 0; j < routes->count; j++) {
		struct route_entry *r = &routes->items[j];
		if (r->oif == -1 && route_table_shown(r->table, verbose, local) &&
		    route_family_shown(r->family, verbose) &&
		    (only_table == -1 || r->table == only_table)) {
			has = 1;
			break;
		}
	}
	if (!has)
		return;

	printf("%sno interface:\n", indent);

	for (int j = 0; j < routes->count; j++) {
		struct route_entry *r = &routes->items[j];
		if (r->oif == -1 && route_table_shown(r->table, verbose, local) &&
		    route_family_shown(r->family, verbose) &&
		    (only_table == -1 || r->table == only_table))
			print_one_route(r, verbose, indent);
	}
}

static void print_routes_by_iface(int only_table, struct iface_table *ifaces,
                                  struct route_table *routes, int verbose,
                                  int local, const char *indent) {
	for (int i = 0; i < ifaces->count; i++) {
		int idx = ifaces->items[i].index;
		int has = 0;
		for (int j = 0; j < routes->count; j++) {
			if (route_visible(&routes->items[j], idx, only_table, verbose,
			                  local)) {
				has = 1;
				break;
			}
		}
		if (!has)
			continue;

		printf("%s%s:\n", indent, ifaces->items[i].name);

		for (int j = 0; j < routes->count; j++) {
			struct route_entry *r = &routes->items[j];
			if (route_visible(r, idx, only_table, verbose, local))
				print_one_route(r, verbose, indent);
		}
	}
}

int route_show(int verbose, int local) {
	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}

	struct route_table routes;
	route_table_init(&routes);
	if (route_table_load(&routes) < 0) {
		route_table_free(&routes);
		iface_table_free(&ifaces);
		return -1;
	}

	int tables[64];
	int ntables = 0;
	for (int j = 0; j < routes.count; j++) {
		struct route_entry *r = &routes.items[j];
		if (!route_table_shown(r->table, verbose, local))
			continue;
		if (!route_family_shown(r->family, verbose))
			continue;
		int seen = 0;
		for (int t = 0; t < ntables; t++)
			if (tables[t] == r->table) {
				seen = 1;
				break;
			}
		if (!seen && ntables < 64)
			tables[ntables++] = r->table;
	}

	if (ntables <= 1) {
		print_routes_by_iface(-1, &ifaces, &routes, verbose, local, "");
		print_no_iface(-1, &routes, verbose, local, "");
	} else {
		for (int t = 0; t < ntables; t++) {
			printf("%s:\n", table_name(tables[t]));
			print_routes_by_iface(tables[t], &ifaces, &routes, verbose, local,
			                      "\t");
			print_no_iface(tables[t], &routes, verbose, local, "\t");
		}
	}

	route_table_free(&routes);
	iface_table_free(&ifaces);
	return 0;
}

int route_add(const char *dst, int dst_len, const char *gw, const char *ifname,
              int has_metric, unsigned int metric) {
	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}
	int ifindex = -1;
	for (int i = 0; i < ifaces.count; i++)
		if (strcmp(ifaces.items[i].name, ifname) == 0) {
			ifindex = ifaces.items[i].index;
			break;
		}
	iface_table_free(&ifaces);
	if (ifindex == -1) {
		fprintf(stderr, "interface not found: %s\n", ifname);
		return -1;
	}

	struct in_addr dstaddr;
	int is_default = (dst == NULL); /* NULL = default (0.0.0.0/0) */
	if (!is_default) {
		if (inet_pton(AF_INET, dst, &dstaddr) != 1) {
			fprintf(stderr, "invalid address: %s\n", dst);
			return -1;
		}
	}

	struct in_addr gwaddr;
	int has_gw = (gw != NULL);
	if (has_gw) {
		if (inet_pton(AF_INET, gw, &gwaddr) != 1) {
			fprintf(stderr, "invalid gateway: %s\n", gw);
			return -1;
		}
	}

	char buf[256];
	memset(buf, 0, sizeof(buf));

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlh->nlmsg_type = RTM_NEWROUTE;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE;

	struct rtmsg *rtm = NLMSG_DATA(nlh);
	rtm->rtm_family = AF_INET;
	rtm->rtm_dst_len = is_default ? 0 : dst_len;
	rtm->rtm_table = RT_TABLE_MAIN;
	rtm->rtm_protocol = RTPROT_STATIC;
	rtm->rtm_scope = has_gw ? RT_SCOPE_UNIVERSE : RT_SCOPE_LINK;
	rtm->rtm_type = RTN_UNICAST;

	if (!is_default)
		netlink_add_attr(nlh, sizeof(buf), RTA_DST, &dstaddr, sizeof(dstaddr));
	if (has_gw)
		netlink_add_attr(nlh, sizeof(buf), RTA_GATEWAY, &gwaddr,
		                 sizeof(gwaddr));

	int oif = ifindex;
	netlink_add_attr(nlh, sizeof(buf), RTA_OIF, &oif, sizeof(oif));

	if (has_metric)
		netlink_add_attr(nlh, sizeof(buf), RTA_PRIORITY, &metric,
		                 sizeof(metric));

	return netlink_send_change(nlh);
}

int route_del(const char *dst, int dst_len, const char *ifname, int table) {
	int is_default = (dst == NULL);

	struct in_addr dstaddr;
	if (!is_default) {
		if (inet_pton(AF_INET, dst, &dstaddr) != 1) {
			fprintf(stderr, "invalid address: %s\n", dst);
			return -1;
		}
	}

	int ifindex = -1;
	if (ifname != NULL) {
		struct iface_table ifaces;
		iface_table_init(&ifaces);
		if (iface_table_load(&ifaces) < 0) {
			iface_table_free(&ifaces);
			return -1;
		}
		for (int i = 0; i < ifaces.count; i++)
			if (strcmp(ifaces.items[i].name, ifname) == 0) {
				ifindex = ifaces.items[i].index;
				break;
			}
		iface_table_free(&ifaces);
		if (ifindex == -1) {
			fprintf(stderr, "interface not found: %s\n", ifname);
			return -1;
		}
	}

	char buf[256];
	memset(buf, 0, sizeof(buf));

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlh->nlmsg_type = RTM_DELROUTE;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;

	struct rtmsg *rtm = NLMSG_DATA(nlh);
	rtm->rtm_family = AF_INET;
	rtm->rtm_dst_len = is_default ? 0 : dst_len;
	rtm->rtm_table = table;
	rtm->rtm_scope = RT_SCOPE_NOWHERE;

	if (!is_default)
		netlink_add_attr(nlh, sizeof(buf), RTA_DST, &dstaddr, sizeof(dstaddr));
	if (ifindex != -1) {
		int oif = ifindex;
		netlink_add_attr(nlh, sizeof(buf), RTA_OIF, &oif, sizeof(oif));
	}

	return netlink_send_change(nlh);
}

int route_flush(const char *ifname, int all) {
	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}
	int ifindex = -1;
	for (int i = 0; i < ifaces.count; i++)
		if (strcmp(ifaces.items[i].name, ifname) == 0) {
			ifindex = ifaces.items[i].index;
			break;
		}
	iface_table_free(&ifaces);
	if (ifindex == -1) {
		fprintf(stderr, "interface not found: %s\n", ifname);
		return -1;
	}

	struct route_table routes;
	route_table_init(&routes);
	if (route_table_load(&routes) < 0) {
		route_table_free(&routes);
		return -1;
	}

	int ret = 0;
	for (int i = 0; i < routes.count; i++) {
		struct route_entry *r = &routes.items[i];
		if (r->oif != ifindex)
			continue;
		if (r->family != AF_INET)
			continue;
		if (!all && r->table != RT_TABLE_MAIN)
			continue;

		const char *dst = (r->dst[0] == '\0') ? NULL : r->dst;
		if (route_del(dst, r->dst_len, ifname, r->table) < 0)
			ret = -1;
	}

	route_table_free(&routes);
	return ret;
}
