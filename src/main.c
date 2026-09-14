#include "color.h"
#include "parser.h"

int main(int argc, char *argv[]) {
	color_init();
	return parse_and_dispatch(argc, argv);
}
