#include "link.h"
#include "color.h"
#include "iface.h"
#include "netlink.h"

#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

struct link_ctx {
	struct iface_table *ifaces;
	int verbose;
};

static const char *link_type_name(unsigned short type) {
	switch (type) {
	case ARPHRD_ETHER:
		return "ethernet";
	case ARPHRD_LOOPBACK:
		return "loopback";
	case ARPHRD_NONE:
		return "none";
	default:
		return "other";
	}
}

static void print_all_flags(unsigned int flags) {
	printf("\t<");
	int first = 1;

	struct {
		unsigned int bit;
		const char *name;
	} table[] = {
	    {IFF_UP, "UP"},
	    {IFF_BROADCAST, "BROADCAST"},
	    {IFF_DEBUG, "DEBUG"},
	    {IFF_LOOPBACK, "LOOPBACK"},
	    {IFF_POINTOPOINT, "POINTOPOINT"},
	    {IFF_RUNNING, "RUNNING"},
	    {IFF_NOARP, "NOARP"},
	    {IFF_PROMISC, "PROMISC"},
	    {IFF_ALLMULTI, "ALLMULTI"},
	    {IFF_MASTER, "MASTER"},
	    {IFF_SLAVE, "SLAVE"},
	    {IFF_MULTICAST, "MULTICAST"},
	    {IFF_DYNAMIC, "DYNAMIC"},
	    {IFF_LOWER_UP, "LOWER_UP"},
	    {IFF_DORMANT, "DORMANT"},
	    {IFF_ECHO, "ECHO"},
	};

	for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
		if (flags & table[i].bit) {
			printf("%s%s", first ? "" : ",", table[i].name);
			first = 0;
		}
	}
	printf(">\n");
}

static void print_link(struct nlmsghdr *nlh, void *ctx) {
	struct link_ctx *lc = ctx;
	struct iface_table *ifaces = lc->ifaces;
	int verbose = lc->verbose;

	int mtu = -1;
	char *qdisc = "";
	int txqlen = -1;
	int group = -1;
	unsigned char *brd = NULL;
	int brd_len = 0;

	if (nlh->nlmsg_type != RTM_NEWLINK)
		return;

	struct ifinfomsg *ifm = NLMSG_DATA(nlh);
	struct rtattr *rta = IFLA_RTA(ifm);
	int rta_len = IFLA_PAYLOAD(nlh);

	char *name = "?";
	unsigned char *mac = NULL;
	int mac_len = 0;
	int operstate = -1;
	int master = -1;

	while (RTA_OK(rta, rta_len)) {
		switch (rta->rta_type) {
		case IFLA_IFNAME:
			name = RTA_DATA(rta);
			break;
		case IFLA_ADDRESS:
			mac = RTA_DATA(rta);
			mac_len = RTA_PAYLOAD(rta);
			break;
		case IFLA_OPERSTATE:
			operstate = *(unsigned char *)RTA_DATA(rta);
			break;
		case IFLA_MASTER:
			master = *(int *)RTA_DATA(rta);
			break;
		case IFLA_MTU:
			mtu = *(int *)RTA_DATA(rta);
			break;
		case IFLA_QDISC:
			qdisc = RTA_DATA(rta);
			break;
		case IFLA_TXQLEN:
			txqlen = *(int *)RTA_DATA(rta);
			break;
		case IFLA_GROUP:
			group = *(int *)RTA_DATA(rta);
			break;
		case IFLA_BROADCAST:
			brd = RTA_DATA(rta);
			brd_len = RTA_PAYLOAD(rta);
			break;
		}

		rta = RTA_NEXT(rta, rta_len);
	}

	const char *state = "UNKNOWN";
	const char *scolor = c_cyan;
	switch (operstate) {
	case IF_OPER_UP:
		state = "UP";
		scolor = c_green;
		break;
	case IF_OPER_DOWN:
		state = "DOWN";
		scolor = c_red;
		break;
	case IF_OPER_DORMANT:
		state = "DORMANT";
		scolor = c_yellow;
		break;
	}

	// id and name
	printf("%d - ", ifm->ifi_index);
	color_print_field(name, "", 14);

	// state
	color_print_field(state, scolor, 8);

	// mac
	char macbuf[18];
	if (mac && mac_len == 6)
		snprintf(macbuf, sizeof(macbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
		         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	else
		snprintf(macbuf, sizeof(macbuf), "MACLESS");
	color_print_field(macbuf, "", 18);

	// type
	color_print_field(link_type_name(ifm->ifi_type), "", 10);

	// flags
	printf("<");
	int first = 1;
	if (ifm->ifi_flags & IFF_BROADCAST) {
		printf("BROADCAST");
		first = 0;
	}
	if (ifm->ifi_flags & IFF_MULTICAST) {
		printf("%sMULTICAST", first ? "" : ", ");
		first = 0;
	}
	if (ifm->ifi_flags & IFF_PROMISC) {
		printf("%s%sPROMISC%s", first ? "" : ", ", c_red, c_reset);
		first = 0;
	}
	printf(">");

	// master
	if (master != -1) {
		const char *mname = iface_table_lookup(ifaces, master);
		if (mname)
			printf(" master %s", mname);
		else
			printf(" master %d", master);
	}

	if (verbose) {
		printf("\n");
		print_all_flags(ifm->ifi_flags);

		printf("\t[mtu %d]  [qdisc %s]  [txqlen %d]  [group %d]\n", mtu, qdisc,
		       txqlen, group);

		if (brd && brd_len == 6)
			printf("\tbroadcast mac %02x:%02x:%02x:%02x:%02x:%02x", brd[0],
			       brd[1], brd[2], brd[3], brd[4], brd[5]);
		else
			printf("\tno broadcast mac");
	}

	printf("\n");
}

int link_show(int verbose) {
	struct iface_table ifaces;
	iface_table_init(&ifaces);
	if (iface_table_load(&ifaces) < 0) {
		iface_table_free(&ifaces);
		return -1;
	}

	int fd = netlink_open();
	if (fd < 0) {
		iface_table_free(&ifaces);
		return -1;
	}

	if (netlink_send_dump_req(fd, RTM_GETLINK, AF_UNSPEC) < 0) {
		close(fd);
		iface_table_free(&ifaces);
		return -1;
	}

	struct link_ctx lc = {.ifaces = &ifaces, .verbose = verbose};
	int ret = netlink_recv_dump(fd, print_link, &lc);

	close(fd);
	iface_table_free(&ifaces);
	return ret;
}
