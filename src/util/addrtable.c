#include "addrtable.h"

#include <stdlib.h>

void addr_table_init(struct addr_table *t) {
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}

int addr_table_add(struct addr_table *t, const struct addr_entry *e) {
	if (t->count == t->capacity) {
		int new_cap = (t->capacity == 0) ? 8 : t->capacity * 2;
		struct addr_entry *tmp =
		    realloc(t->items, new_cap * sizeof(struct addr_entry));
		if (tmp == NULL)
			return -1;
		t->items = tmp;
		t->capacity = new_cap;
	}
	t->items[t->count++] = *e;
	return 0;
}

void addr_table_free(struct addr_table *t) {
	free(t->items);
	t->items = NULL;
	t->count = 0;
	t->capacity = 0;
}
