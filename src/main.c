#include "color.h"
#include "compat.h"
#include "parser.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
	color_init();

	char *targv[64];
	int targc = compat_translate(argc, argv, targv, 64);
	if (targc < 0) {
		fprintf(stderr, "too many arguments\n");
		return 1;
	}

	return parse_and_dispatch(targc, targv);
}
