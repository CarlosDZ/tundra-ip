#include "parser.h"

#include "addr.h"
#include "link.h"
#include "route.h"

#include <stdio.h>
#include <string.h>

static int cmd_link_show(int flags) { return link_show(flags & FLAG_VERBOSE); }

static int cmd_addr_show(int flags) { return addr_show(flags & FLAG_VERBOSE); }

static int cmd_route_show(int flags) {
	return route_show(flags & FLAG_VERBOSE, flags & FLAG_LOCAL);
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
	int (*handler)(int flags);
};

static const struct command commands[] = {
    {"link", "show", FLAG_VERBOSE, cmd_link_show},
    {"addr", "show", FLAG_VERBOSE, cmd_addr_show},
    {"route", "show", FLAG_VERBOSE | FLAG_LOCAL, cmd_route_show},
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

	if (npos < 2) {
		fprintf(stderr, "usage: tundra-ip <object> <action> [options]\n");
		return 1;
	}

	for (size_t c = 0; c < sizeof(commands) / sizeof(commands[0]); c++) {
		if (strcmp(pos[0], commands[c].obj) == 0 &&
		    strcmp(pos[1], commands[c].action) == 0) {
			int bad = flags & ~commands[c].allowed_flags;
			if (bad) {
				fprintf(stderr, "invalid option ");
				print_flag_names(bad);
				fprintf(stderr,
				        " for '%s %s'. Available options: ", commands[c].obj,
				        commands[c].action);
				if (commands[c].allowed_flags)
					print_flag_names(commands[c].allowed_flags);
				else
					fprintf(stderr, "none");
				fprintf(stderr, "\n");
				return 1;
			}
			return commands[c].handler(flags) < 0 ? 1 : 0;
		}
	}

	fprintf(stderr, "unknown command: %s %s\n", pos[0], pos[1]);
	return 1;
}
