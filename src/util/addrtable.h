#ifndef TUNDRA_ADDRTABLE_H
#define TUNDRA_ADDRTABLE_H

#include <netinet/in.h>

struct addr_entry {
	int ifindex;
	char ip[INET6_ADDRSTRLEN];
	int prefixlen;
	int scope;
	int flags;
	char broadcast[INET6_ADDRSTRLEN];
	unsigned int valid_lft;
	unsigned int preferred_lft;
};

struct addr_table {
	struct addr_entry *items;
	int count;
	int capacity;
};

void addr_table_init(struct addr_table *t);
int addr_table_add(struct addr_table *t, const struct addr_entry *e);
void addr_table_free(struct addr_table *t);

#endif
