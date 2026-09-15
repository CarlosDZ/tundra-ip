#include "addr.h"
#include "../net/netlink.h"
#include "../util/addrtable.h"
#include "../util/rtnames.h"
#include "color.h"
#include "iface.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
	if (addr_table_load(&addrs) < 0) {
		addr_table_free(&addrs);
		iface_table_free(&ifaces);
		return -1;
	}

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
			if (addrs.items[j].ifindex != idx)
				continue;

			printf("\t%s/%d %s", addrs.items[j].ip, addrs.items[j].prefixlen,
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
				print_lft("preferred_lifetime", addrs.items[j].preferred_lft);
				printf("\n");
			}
		}
	}

	addr_table_free(&addrs);
	iface_table_free(&ifaces);
	return 0;
}

int addr_add(const char *ip, int prefixlen, const char *ifname) {
	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}
	int ifindex = -1;
	for (int i = 0; i < ifaces.count; i++)
		if (strcmp(ifaces.items[i].name, ifname) == 0) {
			ifindex = ifaces.items[i].index;
			break;
		}
	iface_table_free(&ifaces);

	if (ifindex == -1) {
		fprintf(stderr, "interface not found: %s\n", ifname);
		return -1;
	}

	struct in_addr addr;
	if (inet_pton(AF_INET, ip, &addr) != 1) {
		fprintf(stderr, "invalid address: %s\n", ip);
		return -1;
	}

	char buf[256];
	memset(buf, 0, sizeof(buf));

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	nlh->nlmsg_type = RTM_NEWADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL;

	struct ifaddrmsg *ifa = NLMSG_DATA(nlh);
	ifa->ifa_family = AF_INET;
	ifa->ifa_prefixlen = prefixlen;
	ifa->ifa_index = ifindex;

	netlink_add_attr(nlh, sizeof(buf), IFA_LOCAL, &addr, sizeof(addr));

	return netlink_send_change(nlh);
}
