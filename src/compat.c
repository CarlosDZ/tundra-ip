#include "compat.h"

#include <string.h>

static const char *norm_obj(const char *o) {
	if (strcmp(o, "address") == 0)
		return "addr";
	return o;
}

int compat_translate(int argc, char *argv[], char *out[], int max) {
	int n = 0;

	if (n < max)
		out[n++] = argv[0];

	if (argc < 2) {
		for (int i = 1; i < argc && n < max; i++)
			out[n++] = argv[i];
		return n;
	}

	const char *obj = norm_obj(argv[1]);

	if (strcmp(obj, "link") == 0 && argc >= 3 && strcmp(argv[2], "set") == 0) {

		int idx = 3;
		if (idx < argc && strcmp(argv[idx], "dev") == 0)
			idx++;

		if (idx < argc) {
			const char *ifname = argv[idx];

			if (idx + 1 < argc && strcmp(argv[idx + 1], "up") == 0) {
				if (n + 3 > max)
					return -1;
				out[n++] = "link";
				out[n++] = "up";
				out[n++] = (char *)ifname;
				return n;
			}
			if (idx + 1 < argc && strcmp(argv[idx + 1], "down") == 0) {
				if (n + 3 > max)
					return -1;
				out[n++] = "link";
				out[n++] = "down";
				out[n++] = (char *)ifname;
				return n;
			}
			if (idx + 2 < argc && strcmp(argv[idx + 1], "address") == 0) {
				if (n + 6 > max)
					return -1;
				out[n++] = "link";
				out[n++] = "set";
				out[n++] = "mac";
				out[n++] = (char *)argv[idx + 2];
				out[n++] = "on";
				out[n++] = (char *)ifname;
				return n;
			}
		}
	}

	if (n < max)
		out[n++] = (char *)obj;
	for (int i = 2; i < argc && n < max; i++) {
		if (strcmp(argv[i], "dev") == 0)
			out[n++] = "on";
		else
			out[n++] = argv[i];
	}

	return n;
}
