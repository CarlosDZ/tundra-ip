#ifndef TUNDRA_LINK_H
#define TUNDRA_LINK_H

int link_show(int verbose);
int link_up(const char *ifname);
int link_down(const char *ifname);
int link_set_mac(const char *ifname, const char *macstr);

#endif
