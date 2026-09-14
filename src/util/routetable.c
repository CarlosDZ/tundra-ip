#include "routetable.h"

#include <stdlib.h>

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
