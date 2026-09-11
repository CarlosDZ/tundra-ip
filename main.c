#include "link.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc >= 3 && strcmp(argv[1], "link") == 0 &&
	    strcmp(argv[2], "show") == 0)
		return link_show() < 0 ? 1 : 0;

	fprintf(stderr, "usage: tundra-ip link show\n");
	return 1;
}
