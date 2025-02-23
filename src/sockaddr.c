/*
 * Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
 * Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 * Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 * Copyright (c) 1996-2000 Wichert Akkerman <wichert@cistron.nl>
 * Copyright (c) 2005-2016 Dmitry V. Levin <ldv@strace.io>
 * Copyright (c) 2016-2021 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "defs.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "netlink.h"
#include <linux/ax25.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/mctp.h>
#include <linux/x25.h>

#include "xlat/addrfams.h"
#include "xlat/arp_hardware_types.h"
#include "xlat/ethernet_protocols.h"
#include "xlat/af_packet_types.h"

#include "xlat/bdaddr_types.h"
#include "xlat/bluetooth_l2_cid.h"
#include "xlat/bluetooth_l2_psm.h"
#include "xlat/hci_channels.h"

#include "xlat/mctp_addrs.h"
#include "xlat/mctp_nets.h"

#define SIZEOF_SA_FAMILY sizeof_field(struct sockaddr, sa_family)

#define IPX_NODE_LEN	6
struct sockaddr_ipx {
	uint16_t sipx_family;
	uint16_t sipx_port;
	uint32_t sipx_network;
	unsigned char sipx_node[IPX_NODE_LEN];
	uint8_t sipx_type;
	unsigned char sipx_zero;
};

static void
print_sockaddr_data_un(struct tcb *tcp, const void *const buf, const int addrlen)
{
	const struct sockaddr_un *const sa_un = buf;
	const int un_len = addrlen > (int) sizeof(*sa_un)
			   ? (int) sizeof(*sa_un) : addrlen;
	const int path_len = un_len - SIZEOF_SA_FAMILY;

	tprints_field_name("sun_path");
	if (sa_un->sun_path[0]) {
		print_quoted_string(sa_un->sun_path, path_len + 1,
				    QUOTE_0_TERMINATED);
	} else {
		tprints("@");
		print_quoted_string(sa_un->sun_path + 1, path_len - 1, 0);
	}
}

bool
print_inet_addr(const int af,
		const void *const addr,
		const unsigned int len,
		const char *const var_name)
{
	char buf[INET6_ADDRSTRLEN];

	switch (af) {
	case AF_INET:
		if (inet_ntop(af, addr, buf, sizeof(buf))) {
			if (var_name)
				tprints_field_name(var_name);

			if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_ABBREV)
				print_quoted_string((const char*) addr,
						    len, QUOTE_FORCE_HEX);

			if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW)
				return true;

			if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE)
				tprint_comment_begin();

			tprints_arg_begin("inet_addr");
			print_quoted_string(buf, sizeof(buf), QUOTE_0_TERMINATED);
			tprint_arg_end();

			if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE)
				tprint_comment_end();

			return true;
		}
		break;
	case AF_INET6:
		if (inet_ntop(af, addr, buf, sizeof(buf))) {
			if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_ABBREV) {
				if (var_name)
					tprints_field_name(var_name);
				print_quoted_string(addr, len, QUOTE_FORCE_HEX);
			}

			if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW)
				return true;

			if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE)
				tprint_comment_begin();

			tprints_arg_begin("inet_pton");
			print_xlat_ex(AF_INET6, "AF_INET6", XLAT_STYLE_ABBREV);

			tprint_arg_next();
			print_quoted_string(buf, sizeof(buf), QUOTE_0_TERMINATED);

			if (var_name &&
			    (xlat_verbose(xlat_verbosity) == XLAT_STYLE_ABBREV)) {
				tprint_arg_next();
				tprintf("&%s", var_name);
			}
			tprint_arg_end();

			if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE)
				tprint_comment_end();

			return true;
		}
		break;
	}

	if (var_name)
		tprints_field_name(var_name);
	print_quoted_string(addr, len, QUOTE_FORCE_HEX);
	return false;
}

bool
decode_inet_addr(struct tcb *const tcp,
		 const kernel_ulong_t addr,
		 const unsigned int len,
		 const int family,
		 const char *const var_name)
{
	union {
		struct in_addr  a4;
		struct in6_addr a6;
	} addrbuf;
	size_t size = 0;

	switch (family) {
	case AF_INET:
		size = sizeof(addrbuf.a4);
		break;
	case AF_INET6:
		size = sizeof(addrbuf.a6);
		break;
	}

	if (!size || len < size) {
		if (var_name)
			tprints_field_name(var_name);
		printstr_ex(tcp, addr, len, QUOTE_FORCE_HEX);
		return false;
	}

	if (umoven(tcp, addr, size, &addrbuf) < 0) {
		if (var_name)
			tprints_field_name(var_name);
		printaddr(addr);
		return false;
	}

	return print_inet_addr(family, &addrbuf, size, var_name);
}

static void
print_sockaddr_data_in(struct tcb *tcp, const void *const buf,
		       const int addrlen)
{
	const struct sockaddr_in *const sa_in = buf;

	PRINT_FIELD_NET_PORT(*sa_in, sin_port);
	tprint_struct_next();
	PRINT_FIELD_INET_ADDR(*sa_in, sin_addr, AF_INET);
}

#define SIN6_MIN_LEN offsetof(struct sockaddr_in6, sin6_scope_id)

static void
print_sockaddr_data_in6(struct tcb *tcp, const void *const buf,
			const int addrlen)
{
	const struct sockaddr_in6 *const sa_in6 = buf;

	PRINT_FIELD_NET_PORT(*sa_in6, sin6_port);
	tprint_struct_next();
	tprints_field_name("sin6_flowinfo");
	if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_ABBREV)
		print_quoted_string((const char*) &sa_in6->sin6_flowinfo,
				    sizeof(sa_in6->sin6_flowinfo),
				    QUOTE_FORCE_HEX);

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE)
		tprint_comment_begin();

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE ||
	    xlat_verbose(xlat_verbosity) == XLAT_STYLE_ABBREV) {
		tprints_arg_begin("htonl");
		PRINT_VAL_U(ntohl(sa_in6->sin6_flowinfo));
		tprint_arg_end();
	}

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE)
		tprint_comment_end();

	tprint_struct_next();
	PRINT_FIELD_INET_ADDR(*sa_in6, sin6_addr, AF_INET6);
	if (addrlen <= (int) SIN6_MIN_LEN)
		return;

#if defined IN6_IS_ADDR_LINKLOCAL && defined IN6_IS_ADDR_MC_LINKLOCAL
	if (IN6_IS_ADDR_LINKLOCAL(&sa_in6->sin6_addr)
	    || IN6_IS_ADDR_MC_LINKLOCAL(&sa_in6->sin6_addr)) {
		tprint_struct_next();
		PRINT_FIELD_IFINDEX(*sa_in6, sin6_scope_id);
	} else
#endif
	{
		tprint_struct_next();
		PRINT_FIELD_U(*sa_in6, sin6_scope_id);
	}
}

/**
 * Check that we can print an AX.25 address in its native form, otherwise it
 * makes sense to print it in raw also (or in raw only).
 */
static enum xlat_style
check_ax25_address(const ax25_address *addr)
{
	enum xlat_style ret = XLAT_STYLE_DEFAULT;
	bool space_seen = false;
	bool char_seen = false;

	for (size_t i = 0; i < ARRAY_SIZE(addr->ax25_call) - 1; i++) {
		unsigned char c = addr->ax25_call[i];

		/* The lowest bit should be zero */
		if (c & 1)
			ret = XLAT_STYLE_VERBOSE;

		c >>= 1;

		if (c == ' ')
			space_seen = true;
		else
			char_seen = true;

		/* Sane address contains only numbers and uppercase letters */
		if ((c < '0' || c > '9') && (c < 'A' || c > 'Z') && c != ' ')
			ret = XLAT_STYLE_VERBOSE;
		if (c != ' ' && space_seen)
			ret = XLAT_STYLE_VERBOSE;

		/* non-printable chars */
		if (c < ' ' || c > 0x7e
		    /* characters used for printing comments */
		    || c == '*' || c == '/')
			return XLAT_STYLE_RAW;
	}

	if (addr->ax25_call[ARRAY_SIZE(addr->ax25_call) - 1] & ~0x1e)
		ret = XLAT_STYLE_VERBOSE;

	if (!char_seen && addr->ax25_call[ARRAY_SIZE(addr->ax25_call) - 1])
		ret = XLAT_STYLE_VERBOSE;

	return ret;
}

/** Convert a (presumably) valid AX.25 to a string */
static const char *
ax25_addr2str(const ax25_address *addr)
{
	static char buf[ARRAY_SIZE(addr->ax25_call) + sizeof("-15")];
	char *p = buf;
	size_t end;

	for (end = ARRAY_SIZE(addr->ax25_call) - 1; end; end--)
		if ((addr->ax25_call[end - 1] >> 1) != ' ')
			break;

	for (size_t i = 0; i < end; i++)
		*p++ = ((unsigned char) addr->ax25_call[i]) >> 1;

	*p++ = '-';

	unsigned char ssid = (addr->ax25_call[ARRAY_SIZE(addr->ax25_call) - 1]
			      >> 1) & 0xf;

	if (ssid > 9) {
		*p++ = '1';
		ssid -= 10;
	}

	*p++ = ssid + '0';
	*p = '\0';

	if (buf[0] == '-' && buf[1] == '0')
		return "*";

	return buf;
}

static void
print_ax25_addr_raw(const ax25_address *addr)
{
	tprint_struct_begin();
	PRINT_FIELD_HEX_ARRAY(*addr, ax25_call);
	tprint_struct_end();
}

void
print_ax25_addr(const void /* ax25_address */ *addr_void)
{
	const ax25_address *addr = addr_void;
	enum xlat_style xs = check_ax25_address(addr);

	if (xs == XLAT_STYLE_DEFAULT)
		xs = xlat_verbose(xlat_verbosity);

	if (xs != XLAT_STYLE_ABBREV)
		print_ax25_addr_raw(addr);

	if (xs == XLAT_STYLE_RAW)
		return;

	const char *addr_str = ax25_addr2str(addr);

	(xs == XLAT_STYLE_VERBOSE ? tprints_comment : tprints)(addr_str);
}

static void
print_sockaddr_data_ax25(struct tcb *tcp, const void *const buf,
			 const int addrlen)
{
	const struct full_sockaddr_ax25 *const sax25 = buf;
	size_t addrlen_us = MAX(addrlen, 0);
	bool full = sax25->fsa_ax25.sax25_ndigis ||
	(addrlen_us > sizeof(struct sockaddr_ax25));

	if (full) {
		tprints_field_name("fsa_ax25");
		tprint_struct_begin();
	}

	PRINT_FIELD_OBJ_PTR(sax25->fsa_ax25, sax25_call, print_ax25_addr);
	tprint_struct_next();
	PRINT_FIELD_D(sax25->fsa_ax25, sax25_ndigis);

	if (!full)
		return;

	tprint_struct_end();

	size_t has_digis = MIN((addrlen_us - sizeof(sax25->fsa_ax25))
			       / sizeof(sax25->fsa_digipeater[0]),
			       ARRAY_SIZE(sax25->fsa_digipeater));
	size_t want_digis = MIN(
		(unsigned int) MAX(sax25->fsa_ax25.sax25_ndigis, 0),
		ARRAY_SIZE(sax25->fsa_digipeater));
	size_t digis = MIN(has_digis, want_digis);

	if (want_digis == 0)
		goto digis_end;

	tprint_struct_next();
	tprints_field_name("fsa_digipeater");
	tprint_array_begin();
	for (size_t i = 0; i < digis; i++) {
		if (i)
			tprint_array_next();
		print_ax25_addr(sax25->fsa_digipeater + i);
	}

	if (want_digis > has_digis) {
		if (digis)
			tprint_array_next();
		tprint_unavailable();
	}

	tprint_array_end();

digis_end:
	if (addrlen_us > (has_digis * sizeof(sax25->fsa_digipeater[0])
		       + sizeof(sax25->fsa_ax25))) {
		tprints(", ");
		tprint_more_data_follows();
	}
}

static void
print_sockaddr_data_ipx(struct tcb *tcp, const void *const buf,
			const int addrlen)
{
	const struct sockaddr_ipx *const sa_ipx = buf;

	PRINT_FIELD_NET_PORT(*sa_ipx, sipx_port);
	tprint_struct_next();
	tprints_field_name("sipx_network");
	tprints_arg_begin("htonl");
	PRINT_VAL_X(ntohl(sa_ipx->sipx_network));
	tprint_arg_end();
	tprint_struct_next();
	PRINT_FIELD_VAL_ARRAY(*sa_ipx, sipx_node, PRINT_VAL_0X);
	tprint_struct_next();
	PRINT_FIELD_0X(*sa_ipx, sipx_type);
	if (sa_ipx->sipx_zero) {
		tprint_struct_next();
		PRINT_FIELD_X(*sa_ipx, sipx_zero);
	}
}

void
print_x25_addr(const void /* struct x25_address */ *addr_void)
{
	const struct x25_address *addr = addr_void;

	tprint_struct_begin();
	PRINT_FIELD_CSTRING(*addr, x25_addr);
	tprint_struct_end();
}

static void
print_sockaddr_data_x25(struct tcb *tcp, const void *const buf,
			const int addrlen)
{
	const struct sockaddr_x25 *const sa_x25 = buf;

	PRINT_FIELD_OBJ_PTR(*sa_x25, sx25_addr, print_x25_addr);
}

static void
print_sockaddr_data_nl(struct tcb *tcp, const void *const buf, const int addrlen)
{
	const struct sockaddr_nl *const sa_nl = buf;

	PRINT_FIELD_TGID(*sa_nl, nl_pid, tcp);
	tprint_struct_next();
	PRINT_FIELD_0X(*sa_nl, nl_groups);
}

static void
print_sll_protocol(const struct sockaddr_ll *const sa_ll)
{
	int x_style = xlat_verbose(xlat_verbosity);

	tprints_field_name("sll_protocol");
	if (x_style != XLAT_STYLE_ABBREV)
		print_quoted_string((const char *) &sa_ll->sll_protocol,
				    sizeof(sa_ll->sll_protocol),
				    QUOTE_FORCE_HEX);

	if (x_style == XLAT_STYLE_RAW)
		return;

	if (x_style == XLAT_STYLE_VERBOSE)
		tprint_comment_begin();

	tprints_arg_begin("htons");
	printxval_ex(ethernet_protocols, ntohs(sa_ll->sll_protocol),
		     "ETH_P_???", XLAT_STYLE_ABBREV);
	tprint_arg_end();

	if (x_style == XLAT_STYLE_VERBOSE)
		tprint_comment_end();
}

static void
print_sockaddr_data_ll(struct tcb *tcp, const void *const buf,
		       const int addrlen)
{
	const struct sockaddr_ll *const sa_ll = buf;

	print_sll_protocol(sa_ll);
	tprint_struct_next();
	PRINT_FIELD_IFINDEX(*sa_ll, sll_ifindex);
	tprint_struct_next();
	PRINT_FIELD_XVAL(*sa_ll, sll_hatype, arp_hardware_types, "ARPHRD_???");
	tprint_struct_next();
	PRINT_FIELD_XVAL(*sa_ll, sll_pkttype, af_packet_types, "PACKET_???");
	tprint_struct_next();
	PRINT_FIELD_U(*sa_ll, sll_halen);
	if (sa_ll->sll_halen) {
		const unsigned int oob_halen =
			addrlen - offsetof(struct sockaddr_ll, sll_addr);

		tprint_struct_next();
		tprints_field_name("sll_addr");
		tprint_array_begin();
		for (unsigned int i = 0; i < sa_ll->sll_halen; ++i) {
			if (i)
				tprint_array_next();
			if (i >= oob_halen) {
				tprint_more_data_follows();
				break;
			}
			tprintf("%#02x", sa_ll->sll_addr[i]);
		}
		tprint_array_end();
	}
}

static void
print_sockaddr_data_raw(const void *const buf, const int addrlen)
{
	const char *const data = buf + SIZEOF_SA_FAMILY;
	const int datalen = addrlen - SIZEOF_SA_FAMILY;

	tprints_field_name("sa_data");
	print_quoted_string(data, datalen, 0);
}

static uint16_t
btohs(uint16_t val)
{
	return is_bigendian ? (val << 8) | (val >> 8) : val;
}

static void
print_bluetooth_l2_psm(uint16_t psm)
{
	const uint16_t psm_he = btohs(psm);
	const char *psm_name = xlookup(bluetooth_l2_psm, psm_he);
	const bool psm_str = psm_name || (psm_he >= L2CAP_PSM_LE_DYN_START
					  && psm_he <= L2CAP_PSM_LE_DYN_END)
				      || (psm_he >= L2CAP_PSM_DYN_START);

	tprints_arg_begin("htobs");

	if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_ABBREV || !psm_str)
		PRINT_VAL_X(psm_he);

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW)
		goto print_bluetooth_l2_psm_end;

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE || !psm_str)
		tprint_comment_begin();

	if (psm_name) {
		tprints(psm_name);
	} else if (psm_he >= L2CAP_PSM_LE_DYN_START
	    && psm_he <= L2CAP_PSM_LE_DYN_END) {
		print_xlat(L2CAP_PSM_LE_DYN_START);
		tprints(" + ");
		PRINT_VAL_U(psm_he - L2CAP_PSM_LE_DYN_START);
	} else if (psm_he >= L2CAP_PSM_DYN_START) {
		print_xlat(L2CAP_PSM_DYN_START);
		tprints(" + ");
		PRINT_VAL_U(psm_he - L2CAP_PSM_DYN_START);
	} else {
		tprints("L2CAP_PSM_???");
	}

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE || !psm_str)
		tprint_comment_end();

print_bluetooth_l2_psm_end:
	tprint_arg_end();
}

static void
print_bluetooth_l2_cid(uint16_t cid)
{
	const uint16_t cid_he = btohs(cid);
	const char *cid_name = xlookup(bluetooth_l2_cid, cid_he);
	const bool cid_str = cid_name || (cid_he >= L2CAP_CID_DYN_START);

	tprints_arg_begin("htobs");

	if (xlat_verbose(xlat_verbosity) != XLAT_STYLE_ABBREV || !cid_str)
		PRINT_VAL_X(cid_he);

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW)
		goto print_bluetooth_l2_cid_end;

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE || !cid_str)
		tprint_comment_begin();

	if (cid_name) {
		tprints(cid_name);
	} else if (cid_he >= L2CAP_CID_DYN_START) {
		print_xlat(L2CAP_CID_DYN_START);
		tprints(" + ");
		PRINT_VAL_U(cid_he - L2CAP_CID_DYN_START);
	} else {
		tprints("L2CAP_CID_???");
	}

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_VERBOSE || !cid_str)
		tprint_comment_end();

print_bluetooth_l2_cid_end:
	tprint_arg_end();
}

static void
print_sockaddr_data_bt(struct tcb *tcp, const void *const buf,
		       const int addrlen)
{
	struct sockaddr_hci {
		/* sa_family_t */ uint16_t	hci_family;
		uint16_t			hci_dev;
		uint16_t			hci_channel;
	};

	struct bdaddr {
		uint8_t				b[6];
	} ATTRIBUTE_PACKED;

	struct sockaddr_sco {
		/* sa_family_t */ uint16_t	sco_family;
		struct bdaddr			sco_bdaddr;
	};

	struct sockaddr_rc {
		/* sa_family_t */ uint16_t	rc_family;
		struct bdaddr			rc_bdaddr;
		uint8_t				rc_channel;
	};

	struct sockaddr_l2 {
		/* sa_family_t */ uint16_t	l2_family;
		/* little endian */ uint16_t	l2_psm;
		struct bdaddr			l2_bdaddr;
		/* little endian */ uint16_t	l2_cid;
		uint8_t				l2_bdaddr_type;
	};

	switch (addrlen) {
	case offsetofend(struct sockaddr_hci, hci_dev):
	case sizeof(struct sockaddr_hci): {
		const struct sockaddr_hci *const hci = buf;
		tprints_field_name("hci_dev");
		tprints_arg_begin("htobs");
		PRINT_VAL_U(btohs(hci->hci_dev));
		tprint_arg_end();

		/*
		 * hci_channel field has been introduced
		 * Linux commit in v2.6.38-rc1~476^2~14^2~3^2~43^2~9.
		 */
		if (addrlen == sizeof(struct sockaddr_hci)) {
			tprint_struct_next();
			PRINT_FIELD_XVAL(*hci, hci_channel, hci_channels,
					 "HCI_CHANNEL_???");
		}

		break;
	}
	case sizeof(struct sockaddr_sco): {
		const struct sockaddr_sco *const sco = buf;
		print_mac_addr("sco_bdaddr=", sco->sco_bdaddr.b,
			       sizeof(sco->sco_bdaddr.b));
		break;
	}
	case sizeof(struct sockaddr_rc): {
		const struct sockaddr_rc *const rc = buf;
		print_mac_addr("rc_bdaddr=", rc->rc_bdaddr.b,
			       sizeof(rc->rc_bdaddr.b));
		tprint_struct_next();
		PRINT_FIELD_U(*rc, rc_channel);
		break;
	}
	case offsetof(struct sockaddr_l2, l2_bdaddr_type):
	case sizeof(struct sockaddr_l2): {
		const struct sockaddr_l2 *const l2 = buf;
		PRINT_FIELD_OBJ_VAL(*l2, l2_psm, print_bluetooth_l2_psm);
		print_mac_addr(", l2_bdaddr=", l2->l2_bdaddr.b,
			       sizeof(l2->l2_bdaddr.b));
		tprint_struct_next();
		PRINT_FIELD_OBJ_VAL(*l2, l2_cid, print_bluetooth_l2_cid);

		if (addrlen == sizeof(struct sockaddr_l2)) {
			tprint_struct_next();
			PRINT_FIELD_XVAL(*l2, l2_bdaddr_type,
					 bdaddr_types, "BDADDR_???");
		}

		break;
	}
	default:
		print_sockaddr_data_raw(buf, addrlen);
		break;
	}
}

static void
print_sockaddr_data_mctp(struct tcb *tcp, const void *const buf,
			 const int addrlen)
{
	const struct sockaddr_mctp *const sa_mctp = buf;

	if (sa_mctp->__smctp_pad0) {
		PRINT_FIELD_X(*sa_mctp, __smctp_pad0);
		tprint_struct_next();
	}
	PRINT_FIELD_XVAL(*sa_mctp, smctp_network, mctp_nets, NULL);
	tprint_struct_next();
	tprints_field_name("smctp_addr");
	tprint_struct_begin();
	PRINT_FIELD_XVAL(sa_mctp->smctp_addr, s_addr, mctp_addrs, NULL);
	tprint_struct_end();
	tprint_struct_next();
	PRINT_FIELD_X(*sa_mctp, smctp_type);
	tprint_struct_next();
	PRINT_FIELD_X(*sa_mctp, smctp_tag);
	if (sa_mctp->__smctp_pad1) {
		tprint_struct_next();
		PRINT_FIELD_X(*sa_mctp, __smctp_pad1);
	}

}

typedef void (* const sockaddr_printer)(struct tcb *tcp, const void *const, const int);

static const struct {
	const sockaddr_printer printer;
	const int min_len;
} sa_printers[] = {
	[AF_UNIX] = { print_sockaddr_data_un, SIZEOF_SA_FAMILY + 1 },
	[AF_INET] = { print_sockaddr_data_in, sizeof(struct sockaddr_in) },
	[AF_AX25] = { print_sockaddr_data_ax25, sizeof(struct sockaddr_ax25) },
	[AF_IPX] = { print_sockaddr_data_ipx, sizeof(struct sockaddr_ipx) },
	[AF_X25] = { print_sockaddr_data_x25, sizeof(struct sockaddr_x25) },
	[AF_INET6] = { print_sockaddr_data_in6, SIN6_MIN_LEN },
	[AF_NETLINK] = { print_sockaddr_data_nl, SIZEOF_SA_FAMILY + 1 },
	[AF_PACKET] = { print_sockaddr_data_ll, sizeof(struct sockaddr_ll) },
	[AF_BLUETOOTH] = { print_sockaddr_data_bt, SIZEOF_SA_FAMILY + 1 },
	[AF_MCTP] = { print_sockaddr_data_mctp, sizeof(struct sockaddr_mctp) },
};

void
print_sockaddr(struct tcb *tcp, const void *const buf, const int addrlen)
{
	const struct sockaddr *const sa = buf;

	tprint_struct_begin();
	PRINT_FIELD_XVAL(*sa, sa_family, addrfams, "AF_???");

	if (addrlen > (int) SIZEOF_SA_FAMILY) {
		tprint_struct_next();

		if (sa->sa_family < ARRAY_SIZE(sa_printers)
		    && sa_printers[sa->sa_family].printer
		    && addrlen >= sa_printers[sa->sa_family].min_len) {
			sa_printers[sa->sa_family].printer(tcp, buf, addrlen);
		} else {
			print_sockaddr_data_raw(buf, addrlen);
		}
	}

	tprint_struct_end();
}

int
decode_sockaddr(struct tcb *const tcp, const kernel_ulong_t addr, int addrlen)
{
	if (addrlen < 2) {
		printaddr(addr);
		return -1;
	}

	union {
		struct sockaddr sa;
		struct sockaddr_storage storage;
		char pad[sizeof(struct sockaddr_storage) + 1];
	} addrbuf;

	if ((unsigned) addrlen > sizeof(addrbuf.storage))
		addrlen = sizeof(addrbuf.storage);

	if (umoven_or_printaddr(tcp, addr, addrlen, addrbuf.pad))
		return -1;

	memset(&addrbuf.pad[addrlen], 0, sizeof(addrbuf.pad) - addrlen);

	print_sockaddr(tcp, &addrbuf, addrlen);

	return addrbuf.sa.sa_family;
}
