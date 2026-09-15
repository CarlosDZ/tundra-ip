#ifndef TUNDRA_ROUTE_H
#define TUNDRA_ROUTE_H

int route_show(int verbose, int local);
int route_add(const char *dst, int dst_len, const char *gw, const char *ifname,
              int has_metric, unsigned int metric);
int route_del(const char *dst, int dst_len, const char *ifname, int table);
int route_flush(const char *ifname, int all);

#endif
