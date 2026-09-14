#ifndef TUNDRA_IFACE_H
#define TUNDRA_IFACE_H

struct iface {
	int index;
	char name[16];
	int operstate;
	unsigned short type;
	unsigned char mac[6];
	int has_mac;
	unsigned int flags;
};

struct iface_table {
	struct iface *items;
	int count;
	int capacity;
};

void iface_table_init(struct iface_table *t);
int iface_table_add(struct iface_table *t, const struct iface *e);
const char *iface_table_lookup(struct iface_table *t, int index);
int iface_table_load(struct iface_table *t);
void iface_table_free(struct iface_table *t);

#endif
