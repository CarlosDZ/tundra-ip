#include "status.h"

#include "addrtable.h"
#include "color.h"
#include "iface.h"
#include "routetable.h"
#include "rtnames.h"

#include <linux/if.h>
#include <linux/if_addr.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <stdio.h>

static const char *state_str(int operstate, const char **color) {
	switch (operstate) {
	case IF_OPER_UP:
		*color = c_green;
		return "UP";
	case IF_OPER_DOWN:
		*color = c_red;
		return "DOWN";
	case IF_OPER_DORMANT:
		*color = c_yellow;
		return "DORMANT";
	default:
		*color = c_cyan;
		return "UNKNOWN";
	}
}

static const char *type_str(unsigned short type) {
	switch (type) {
	case ARPHRD_ETHER:
		return "ethernet";
	case ARPHRD_LOOPBACK:
		return "loopback";
	case ARPHRD_NONE:
		return "none";
	default:
		return "other";
	}
}

int status_show(int verbose, int local) {
	(void)verbose;
	(void)local;

	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}

	struct addr_table addrs;
	addr_table_init(&addrs);
	if (addr_table_load(&addrs) < 0) {
		addr_table_free(&addrs);
		iface_table_free(&ifaces);
		return -1;
	}

	struct route_table routes;
	route_table_init(&routes);
	if (route_table_load(&routes) < 0) {
		route_table_free(&routes);
		addr_table_free(&addrs);
		iface_table_free(&ifaces);
		return -1;
	}

	for (int i = 0; i < ifaces.count; i++) {
		struct iface *itf = &ifaces.items[i];

		const char *scolor;
		const char *sstr = state_str(itf->operstate, &scolor);

		color_print_field(itf->name, "", 12);
		color_print_field(sstr, scolor, 8);
		color_print_field(type_str(itf->type), "", 10);

		char macbuf[18];
		if (itf->has_mac)
			snprintf(macbuf, sizeof(macbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
			         itf->mac[0], itf->mac[1], itf->mac[2], itf->mac[3],
			         itf->mac[4], itf->mac[5]);
		else
			snprintf(macbuf, sizeof(macbuf), "MACLESS");
		printf("%s\n", macbuf);

		int first_addr = 1;
		for (int j = 0; j < addrs.count; j++) {
			if (addrs.items[j].ifindex != itf->index)
				continue;
			if (first_addr) {
				printf("\taddresses:  ");
				first_addr = 0;
			} else {
				printf("\t            ");
			}
			printf("%s/%d %s", addrs.items[j].ip, addrs.items[j].prefixlen,
			       scope_name(addrs.items[j].scope));
			if (!(addrs.items[j].flags & IFA_F_PERMANENT))
				printf(" %sdynamic%s", c_yellow, c_reset);
			printf("\n");
		}

		int first_route = 1;
		for (int j = 0; j < routes.count; j++) {
			struct route_entry *r = &routes.items[j];
			if (r->oif != itf->index)
				continue;
			if (r->table != RT_TABLE_MAIN || r->family != AF_INET)
				continue;
			if (first_route) {
				printf("\troutes:     ");
				first_route = 0;
			} else {
				printf("\t            ");
			}
			if (r->has_metric)
				printf("%s[%u]%s  ", c_cyan, r->metric, c_reset);
			else
				printf("%s[-]%s     ", c_cyan, c_reset);
			if (r->dst[0] == '\0')
				printf("%sdefault%s", c_green, c_reset);
			else
				printf("%s/%d", r->dst, r->dst_len);
			if (r->gw[0] != '\0')
				printf(" via %s", r->gw);
			printf("\n");
		}
	}

	route_table_free(&routes);
	addr_table_free(&addrs);
	iface_table_free(&ifaces);
	return 0;
}
