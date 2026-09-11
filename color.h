#ifndef TUNDRA_COLOR_H
#define TUNDRA_COLOR_H

extern const char *c_reset;
extern const char *c_red;
extern const char *c_green;
extern const char *c_yellow;
extern const char *c_cyan;

void color_init(void);
void color_print_field(const char *text, const char *color, int width);

#endif
