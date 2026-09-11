#ifndef TUNDRA_IFACE_H
#define TUNDRA_IFACE_H

struct iface {
	int index;
	char name[16];
};

struct iface_table {
	struct iface *items;
	int count;
	int capacity;
};

void iface_table_init(struct iface_table *t);
int iface_table_add(struct iface_table *t, int index, const char *name);
const char *iface_table_lookup(struct iface_table *t, int index);
void iface_table_free(struct iface_table *t);
int iface_table_load(struct iface_table *t);

#endif
