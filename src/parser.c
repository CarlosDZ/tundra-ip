#include "parser.h"

#include "cmd/commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_link_show(int flags, char **args, int nargs) {
	(void)args;
	(void)nargs;
	return link_show(flags & FLAG_VERBOSE);
}

static int cmd_addr_show(int flags, char **args, int nargs) {
	(void)args;
	(void)nargs;
	return addr_show(flags & FLAG_VERBOSE);
}

static int cmd_route_show(int flags, char **args, int nargs) {
	(void)args;
	(void)nargs;
	return route_show(flags & FLAG_VERBOSE, flags & FLAG_LOCAL);
}
static int cmd_status(int flags, char **args, int nargs) {
	(void)args;
	(void)nargs;
	return status_show(flags & FLAG_VERBOSE, flags & FLAG_LOCAL);
}

static int parse_addr_args(char **args, int nargs, char *ip_out, size_t ip_size,
                           int *prefix_out, const char **if_out) {
	if (nargs != 3 || strcmp(args[1], "on") != 0)
		return -1;
	snprintf(ip_out, ip_size, "%s", args[0]);
	char *slash = strchr(ip_out, '/');
	if (!slash)
		return -1;
	*slash = '\0';
	*prefix_out = atoi(slash + 1);
	*if_out = args[2];
	return 0;
}

static int cmd_addr_add(int flags, char **args, int nargs) {
	(void)flags;
	char ip[64];
	int prefix;
	const char *ifname;
	if (parse_addr_args(args, nargs, ip, sizeof(ip), &prefix, &ifname) < 0) {
		fprintf(stderr,
		        "usage: tundra-ip addr add <IP>/<prefix> on <interface>\n");
		return 1;
	}
	return addr_add(ip, prefix, ifname) < 0 ? 1 : 0;
}

static int cmd_addr_del(int flags, char **args, int nargs) {
	(void)flags;
	char ip[64];
	int prefix;
	const char *ifname;
	if (parse_addr_args(args, nargs, ip, sizeof(ip), &prefix, &ifname) < 0) {
		fprintf(stderr,
		        "usage: tundra-ip addr add <IP>/<prefix> on <interface>\n");
		return 1;
	}
	return addr_del(ip, prefix, ifname) < 0 ? 1 : 0;
}

struct flag_def {
	const char *name;
	int bit;
};

static const struct flag_def known_flags[] = {
    {"--verbose", FLAG_VERBOSE},
    {"--local", FLAG_LOCAL},
};

struct command {
	const char *obj;
	const char *action;
	int allowed_flags;
	int (*handler)(int flags, char **args, int nargs);
};

static const struct command commands[] = {
    {"link", "show", FLAG_VERBOSE, cmd_link_show},
    {"addr", "show", FLAG_VERBOSE, cmd_addr_show},
    {"route", "show", FLAG_VERBOSE | FLAG_LOCAL, cmd_route_show},
    {"status", "", FLAG_VERBOSE | FLAG_LOCAL, cmd_status},
    {"addr", "add", 0, cmd_addr_add},
    {"addr", "del", 0, cmd_addr_del},
};

static void print_flag_names(int mask) {
	int first = 1;
	for (size_t f = 0; f < sizeof(known_flags) / sizeof(known_flags[0]); f++) {
		if (mask & known_flags[f].bit) {
			fprintf(stderr, "%s%s", first ? "" : ", ", known_flags[f].name);
			first = 0;
		}
	}
}

int parse_and_dispatch(int argc, char *argv[]) {
	int flags = 0;
	char *pos[argc];
	int npos = 0;

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--", 2) == 0) {
			int found = 0;
			for (size_t f = 0; f < sizeof(known_flags) / sizeof(known_flags[0]);
			     f++) {
				if (strcmp(argv[i], known_flags[f].name) == 0) {
					flags |= known_flags[f].bit;
					found = 1;
					break;
				}
			}
			if (!found) {
				fprintf(stderr, "unknown option %s. Known options: ", argv[i]);
				print_flag_names(~0);
				fprintf(stderr, "\n");
				return 1;
			}
		} else if (argv[i][0] == '-' && argv[i][1] != '\0') {
			fprintf(stderr, "unknown option %s. Known options: ", argv[i]);
			print_flag_names(~0);
			fprintf(stderr, "\n");
			return 1;
		} else {
			pos[npos++] = argv[i];
		}
	}

	if (npos < 1) {
		fprintf(stderr, "usage: tundra-ip <command> [options]\n");
		return 1;
	}

	for (size_t c = 0; c < sizeof(commands) / sizeof(commands[0]); c++) {
		int obj_match = strcmp(pos[0], commands[c].obj) == 0;
		int action_match;
		int arg_offset;

		if (commands[c].action[0] == '\0') {
			action_match = 1;
			arg_offset = 1; /* objeto en pos[0], args desde pos[1] */
		} else {
			action_match =
			    (npos >= 2 && strcmp(pos[1], commands[c].action) == 0);
			arg_offset = 2; /* objeto+accion, args desde pos[2] */
		}

		if (obj_match && action_match) {
			int bad = flags & ~commands[c].allowed_flags;
			if (bad) {
				fprintf(stderr, "invalid option ");
				print_flag_names(bad);
				fprintf(stderr,
				        " for '%s%s%s'. Available options: ", commands[c].obj,
				        commands[c].action[0] ? " " : "", commands[c].action);
				if (commands[c].allowed_flags)
					print_flag_names(commands[c].allowed_flags);
				else
					fprintf(stderr, "none");
				fprintf(stderr, "\n");
				return 1;
			}
			char **cmd_args = pos + arg_offset;
			int cmd_nargs = npos - arg_offset;
			return commands[c].handler(flags, cmd_args, cmd_nargs) < 0 ? 1 : 0;
		}
	}

	fprintf(stderr, "unknown command: %s %s\n", pos[0], pos[1]);
	return 1;
}
