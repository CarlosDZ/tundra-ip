#ifndef TUNDRA_PARSER_H
#define TUNDRA_PARSER_H

// This limits me to 32 flags but if i need more than 32 i might have a problem
#define FLAG_VERBOSE (1 << 0)
#define FLAG_LOCAL (1 << 1)
#define FLAG_ALL (1 << 2)

int parse_and_dispatch(int argc, char *argv[]);

#endif
