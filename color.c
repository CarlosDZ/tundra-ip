#include "color.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *c_reset = "";
const char *c_red = "";
const char *c_green = "";
const char *c_yellow = "";
const char *c_cyan = "";

void color_init(void) {
	if (getenv("NO_COLOR") != NULL)
		return;
	if (!isatty(STDOUT_FILENO))
		return;

	c_reset = "\033[0m";
	c_red = "\033[31m";
	c_green = "\033[32m";
	c_yellow = "\033[33m";
	c_cyan = "\033[36m";
}

void color_print_field(const char *text, const char *color, int width) {
	printf("%s%s%s", color, text, c_reset);

	int pad = width - (int)strlen(text);
	for (int i = 0; i < pad; i++)
		putchar(' ');
}
