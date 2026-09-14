#include "addr.h"
#include "../util/addrtable.h"
#include "../util/rtnames.h"
#include "color.h"
#include "iface.h"
#include "netlink.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static void collect_addr(struct nlmsghdr *nlh, void *ctx) {
	if (nlh->nlmsg_type != RTM_NEWADDR)
		return;

	struct addr_table *at = ctx;
	struct ifaddrmsg *ifa = NLMSG_DATA(nlh);
	struct rtattr *rta = IFA_RTA(ifa);
	int rta_len = IFA_PAYLOAD(nlh);

	struct addr_entry e;
	e.ifindex = ifa->ifa_index;
	e.prefixlen = ifa->ifa_prefixlen;
	e.scope = ifa->ifa_scope;
	e.flags = ifa->ifa_flags;
	e.ip[0] = '\0';
	int have_ip = 0;
	e.broadcast[0] = '\0';
	e.valid_lft = 0;
	e.preferred_lft = 0;

	while (RTA_OK(rta, rta_len)) {
		if (rta->rta_type == IFA_LOCAL) {
			inet_ntop(ifa->ifa_family, RTA_DATA(rta), e.ip, sizeof(e.ip));
			have_ip = 1;
		} else if (rta->rta_type == IFA_FLAGS) {
			e.flags = *(unsigned int *)RTA_DATA(rta);
		} else if (rta->rta_type == IFA_BROADCAST) {
			inet_ntop(ifa->ifa_family, RTA_DATA(rta), e.broadcast,
			          sizeof(e.broadcast));
		} else if (rta->rta_type == IFA_CACHEINFO) {
			struct ifa_cacheinfo *ci = RTA_DATA(rta);
			e.valid_lft = ci->ifa_valid;
			e.preferred_lft = ci->ifa_prefered;
		}
		rta = RTA_NEXT(rta, rta_len);
	}

	if (have_ip)
		addr_table_add(at, &e);
}

static void print_addr_flags(unsigned int flags) {
	printf("\t\t<");
	int first = 1;

	struct {
		unsigned int bit;
		const char *name;
	} table[] = {
	    {IFA_F_SECONDARY, "SECONDARY"},         {IFA_F_PERMANENT, "PERMANENT"},
	    {IFA_F_DEPRECATED, "DEPRECATED"},       {IFA_F_TENTATIVE, "TENTATIVE"},
	    {IFA_F_DADFAILED, "DADFAILED"},         {IFA_F_TEMPORARY, "TEMPORARY"},
	    {IFA_F_NOPREFIXROUTE, "NOPREFIXROUTE"},
	};

	for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
		if (flags & table[i].bit) {
			printf("%s%s", first ? "" : ",", table[i].name);
			first = 0;
		}
	}
	printf(">\n");
}

static void print_lft(const char *label, unsigned int lft) {
	if (lft == 0xFFFFFFFF) {
		printf("[%s forever]", label);
		return;
	}

	unsigned int days = lft / 86400;
	unsigned int hours = (lft % 86400) / 3600;
	unsigned int mins = (lft % 3600) / 60;
	unsigned int secs = lft % 60;

	if (days > 0)
		printf("[%s %ud %02u:%02u:%02u]", label, days, hours, mins, secs);
	else
		printf("[%s %02u:%02u:%02u]", label, hours, mins, secs);
}

int addr_show(int verbose) {
	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}

	struct addr_table addrs;
	addr_table_init(&addrs);

	int fd = netlink_open();
	if (fd < 0) {
		addr_table_free(&addrs);
		iface_table_free(&ifaces);
		return -1;
	}

	if (netlink_send_dump_req(fd, RTM_GETADDR, AF_UNSPEC) < 0) {
		close(fd);
		addr_table_free(&addrs);
		iface_table_free(&ifaces);
		return -1;
	}

	int ret = netlink_recv_dump(fd, collect_addr, &addrs);
	close(fd);

	for (int i = 0; i < ifaces.count; i++) {
		int idx = ifaces.items[i].index;

		int has = 0;
		for (int j = 0; j < addrs.count; j++) {
			if (addrs.items[j].ifindex == idx) {
				has = 1;
				break;
			}
		}
		if (!has)
			continue;

		printf("%s:\n", ifaces.items[i].name);

		for (int j = 0; j < addrs.count; j++) {
			if (addrs.items[j].ifindex == idx) {
				printf("\t%s/%d %s", addrs.items[j].ip,
				       addrs.items[j].prefixlen,
				       scope_name(addrs.items[j].scope));
				if (!(addrs.items[j].flags & IFA_F_PERMANENT))
					printf(" %sdynamic%s", c_yellow, c_reset);
				printf("\n");

				if (verbose) {
					print_addr_flags(addrs.items[j].flags);
					printf("\t\t");
					if (addrs.items[j].broadcast[0] != '\0')
						printf("[broadcast %s]  ", addrs.items[j].broadcast);
					else
						printf("[no broadcast]  ");

					print_lft("valid_lifetime", addrs.items[j].valid_lft);
					printf("  ");
					print_lft("preferred_lifetime",
					          addrs.items[j].preferred_lft);
					printf("\n");
				}
			}
		}
	}

	addr_table_free(&addrs);
	iface_table_free(&ifaces);
	return ret;
}
