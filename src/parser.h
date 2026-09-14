#ifndef TUNDRA_PARSER_H
#define TUNDRA_PARSER_H

#define FLAG_VERBOSE (1 << 0)
#define FLAG_LOCAL (1 << 1)

int parse_and_dispatch(int argc, char *argv[]);

#endif
