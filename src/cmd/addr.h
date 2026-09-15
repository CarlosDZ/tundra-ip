#ifndef TUNDRA_ADDR_H
#define TUNDRA_ADDR_H

int addr_show(int verbose);
int addr_add(const char *ip, int prefixlen, const char *ifname);

#endif
