#include "rtnames.h"

#include <linux/rtnetlink.h>

const char *scope_name(int scope) {
	switch (scope) {
	case RT_SCOPE_UNIVERSE:
		return "global";
	case RT_SCOPE_HOST:
		return "host";
	case RT_SCOPE_LINK:
		return "link";
	case RT_SCOPE_SITE:
		return "site";
	case RT_SCOPE_NOWHERE:
		return "nowhere";
	default:
		return "other";
	}
}

const char *proto_name(int proto) {
	switch (proto) {
	case RTPROT_KERNEL:
		return "kernel";
	case RTPROT_BOOT:
		return "boot";
	case RTPROT_STATIC:
		return "static";
	case RTPROT_DHCP:
		return "dhcp";
	case RTPROT_RA:
		return "ra";
	case RTPROT_REDIRECT:
		return "redirect";
	default:
		return "other";
	}
}
