#include "help.h"

#include <stdio.h>
#include <string.h>

struct action_help {
	const char *usage;
};

struct object_help {
	const char *name;
	const char *desc;
	const struct action_help *actions;
	int n_actions;
};

static const struct action_help addr_actions[] = {
    {"addr show [--verbose]"},
    {"addr add <IP>/<prefix> on <interface>"},
    {"addr del <IP>/<prefix> on <interface>"},
    {"addr flush on <interface>"},
};

static const struct action_help route_actions[] = {
    {"route show [--verbose] [--local]"},
    {"route add <net>/<prefix> [via <gw>] on <interface> [metric <n>]"},
    {"route del <net>/<prefix> [on <interface>]"},
    {"route flush on <interface> [--all]"},
};

static const struct action_help link_actions[] = {
    {"link show [--verbose]"},
    {"link up <interface>"},
    {"link down <interface>"},
    {"link set mac <mac> on <interface>"},
};

static const struct action_help status_actions[] = {
    {"status [--verbose] [--local]"},
};

static const struct object_help help_tree[] = {
    {"status", "unified per-interface network view", status_actions, 1},
    {"addr", "manage addresses", addr_actions, 4},
    {"route", "manage routes", route_actions, 4},
    {"link", "manage interfaces", link_actions, 4},
};

static void help_level0(void) {
	printf("tundra-ip - network configuration tool\n\n");
	printf("Commands:\n");
	for (size_t i = 0; i < sizeof(help_tree) / sizeof(help_tree[0]); i++)
		printf("  %-10s %s\n", help_tree[i].name, help_tree[i].desc);
	printf("\nRun 'tundra-ip <command> --help' for command details.\n");
	printf("\nOptions:\n");
	printf("  --version   print version\n");
	printf("  --help      print help\n");
	printf("\nWrite commands require root.\n");
}

static void help_object(const struct object_help *o) {
	printf("tundra-ip %s - %s\n\n", o->name, o->desc);
	printf("Usage:\n");
	for (int j = 0; j < o->n_actions; j++)
		printf("  tundra-ip %s\n", o->actions[j].usage);
}

void help_show(const char *obj) {
	if (obj != NULL) {
		for (size_t i = 0; i < sizeof(help_tree) / sizeof(help_tree[0]); i++)
			if (strcmp(help_tree[i].name, obj) == 0) {
				help_object(&help_tree[i]);
				return;
			}
	}
	help_level0();
}
