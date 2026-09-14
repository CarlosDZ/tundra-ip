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
#include <string.h>

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

static void print_link_flags_full(unsigned int flags) {
	printf("\tflags:      <");
	int first = 1;
	struct {
		unsigned int bit;
		const char *name;
	} tbl[] = {
	    {IFF_UP, "UP"},
	    {IFF_BROADCAST, "BROADCAST"},
	    {IFF_LOOPBACK, "LOOPBACK"},
	    {IFF_POINTOPOINT, "POINTOPOINT"},
	    {IFF_RUNNING, "RUNNING"},
	    {IFF_NOARP, "NOARP"},
	    {IFF_PROMISC, "PROMISC"},
	    {IFF_MULTICAST, "MULTICAST"},
	    {IFF_LOWER_UP, "LOWER_UP"},
	    {IFF_DORMANT, "DORMANT"},
	};
	for (size_t i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) {
		if (flags & tbl[i].bit) {
			printf("%s%s", first ? "" : ",", tbl[i].name);
			first = 0;
		}
	}
	printf(">\n");
}

static void print_link_details(const struct iface *itf) {
	printf("\tlink:       [mtu %d]  [qdisc %s]  [txqlen %d]  [group %d]",
	       itf->mtu, itf->qdisc, itf->txqlen, itf->group);
	if (itf->has_broadcast)
		printf("  [brd %02x:%02x:%02x:%02x:%02x:%02x]", itf->broadcast[0],
		       itf->broadcast[1], itf->broadcast[2], itf->broadcast[3],
		       itf->broadcast[4], itf->broadcast[5]);
	printf("\n");
}

int status_show(int verbose, int local) {
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
		if (itf->has_mac)
			printf("%02x:%02x:%02x:%02x:%02x:%02x\n", itf->mac[0], itf->mac[1],
			       itf->mac[2], itf->mac[3], itf->mac[4], itf->mac[5]);
		else
			printf("MACLESS\n");

		if (verbose) {
			print_link_flags_full(itf->flags);
			print_link_details(itf);
		}

		int first_addr = 1;
		for (int j = 0; j < addrs.count; j++) {
			struct addr_entry *a = &addrs.items[j];
			if (a->ifindex != itf->index)
				continue;
			printf(first_addr ? "\taddresses:  " : "\t            ");
			first_addr = 0;
			printf("%s/%d %s", a->ip, a->prefixlen, scope_name(a->scope));
			if (!(a->flags & IFA_F_PERMANENT))
				printf(" %sdynamic%s", c_yellow, c_reset);
			printf("\n");
			if (verbose) {
				printf("\t            ");
				if (a->broadcast[0] != '\0')
					printf("[broadcast %s]", a->broadcast);
				else
					printf("[no broadcast]");
				printf("\n");
			}
		}

		int first_route = 1;
		for (int j = 0; j < routes.count; j++) {
			struct route_entry *r = &routes.items[j];
			if (r->oif != itf->index)
				continue;
			if (!route_family_shown(r->family, verbose))
				continue;
			if (!route_table_shown(r->table, verbose, local))
				continue;
			printf(first_route ? "\troutes:     " : "\t            ");
			first_route = 0;
			char metric[16];
			if (r->has_metric)
				snprintf(metric, sizeof(metric), "[%u]", r->metric);
			else
				snprintf(metric, sizeof(metric), "[-]");
			printf("%s%s%s", c_cyan, metric, c_reset);
			for (int k = strlen(metric); k < 7; k++)
				putchar(' ');
			putchar(' ');

			/* parte de ruta en buffer plano para medir */
			char rbuf[128];
			int rn = 0;
			if (r->dst[0] == '\0')
				rn += snprintf(rbuf + rn, sizeof(rbuf) - rn, "default");
			else
				rn += snprintf(rbuf + rn, sizeof(rbuf) - rn, "%s/%d", r->dst,
				               r->dst_len);
			if (r->gw[0] != '\0')
				rn += snprintf(rbuf + rn, sizeof(rbuf) - rn, " via %s", r->gw);

			if (r->dst[0] == '\0')
				printf("%s%s%s%s", c_green, "default", c_reset,
				       rbuf + strlen("default"));
			else
				printf("%s", rbuf);

			if (verbose) {
				int pad = (rn < 38) ? 38 - rn : 1;
				for (int k = 0; k < pad; k++)
					putchar(' ');
				printf("[proto %s] [scope %s]", proto_name(r->proto),
				       scope_name(r->scope));
				if (r->src[0] != '\0')
					printf(" [src %s]", r->src);
			}
			printf("\n");
		}
	}

	route_table_free(&routes);
	addr_table_free(&addrs);
	iface_table_free(&ifaces);
	return 0;
}
