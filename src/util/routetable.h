#ifndef TUNDRA_ROUTETABLE_H
#define TUNDRA_ROUTETABLE_H

#include <netinet/in.h>

struct route_entry {
	int table;
	int oif;
	char dst[INET6_ADDRSTRLEN];
	int dst_len;
	char gw[INET6_ADDRSTRLEN];
	int has_metric;
	unsigned int metric;
	int family;
	char src[INET6_ADDRSTRLEN];
	int proto;
	int scope;
};

struct route_table {
	struct route_entry *items;
	int count;
	int capacity;
};

void route_table_init(struct route_table *t);
int route_table_add(struct route_table *t, const struct route_entry *e);
void route_table_free(struct route_table *t);
int route_table_load(struct route_table *t);

#endif
