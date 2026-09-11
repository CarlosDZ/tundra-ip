#include "addr.h"
#include "color.h"
#include "link.h"
#include "route.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
	color_init();

	int verbose = 0;
	char *args[argc];
	int nargs = 0;

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--", 2) == 0) {
			if (strcmp(argv[i], "--verbose") == 0)
				verbose = 1;
			else {
				fprintf(stderr, "unknown option: %s\n", argv[i]);
				return 1;
			}
		} else if (argv[i][0] == '-' && argv[i][1] != '\0') {
			fprintf(stderr, "unknown option: %s\n", argv[i]);
			return 1;
		} else {
			args[nargs++] = argv[i];
		}
	}

	if (nargs >= 2 && strcmp(args[0], "link") == 0 &&
	    strcmp(args[1], "show") == 0)
		return link_show(verbose) < 0 ? 1 : 0;

	if (nargs >= 2 && strcmp(args[0], "addr") == 0 &&
	    strcmp(args[1], "show") == 0)
		return addr_show() < 0 ? 1 : 0;

	if (nargs >= 2 && strcmp(args[0], "route") == 0 &&
	    strcmp(args[1], "show") == 0)
		return route_show() < 0 ? 1 : 0;

	fprintf(stderr, "usage: tundra-ip {link|addr|route} show [--verbose]\n");
	return 1;
}
