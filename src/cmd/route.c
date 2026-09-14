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

static int table_shown(int table, int verbose, int local) {
	if (table == RT_TABLE_MAIN)
		return 1;
	if (table == RT_TABLE_LOCAL)
		return local;
	return verbose;
}

static int family_shown(int family, int verbose) {
	if (family == AF_INET)
		return 1;
	if (family == AF_INET6)
		return verbose;
	return 0;
}

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
	if (!family_shown(r->family, verbose))
		return 0;
	if (!table_shown(r->table, verbose, local))
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
		if (r->oif == -1 && table_shown(r->table, verbose, local) &&
		    family_shown(r->family, verbose) &&
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
		if (r->oif == -1 && table_shown(r->table, verbose, local) &&
		    family_shown(r->family, verbose) &&
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
		if (!table_shown(r->table, verbose, local))
			continue;
		if (!family_shown(r->family, verbose))
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
