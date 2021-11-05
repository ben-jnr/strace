/*
 * Check decoding of sockaddr fields under xlat styles.
 *
 * Copyright (c) 2015-2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "tests.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/ax25.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
# include <bluetooth/bluetooth.h>
# include <bluetooth/hci.h>
# include <bluetooth/l2cap.h>
# include <bluetooth/rfcomm.h>
# include <bluetooth/sco.h>
#endif

#ifdef HAVE_LINUX_NFC_H
# include <linux/nfc.h>
#else
struct sockaddr_nfc {
	uint16_t sa_family;
	uint32_t dev_idx;
	uint32_t target_idx;
	uint32_t nfc_protocol;
};

#define NFC_LLCP_MAX_SERVICE_NAME 63
struct sockaddr_nfc_llcp {
	uint16_t sa_family;
	uint32_t dev_idx;
	uint32_t target_idx;
	uint32_t nfc_protocol;
	uint8_t dsap;
	uint8_t ssap;
	char service_name[NFC_LLCP_MAX_SERVICE_NAME];
	size_t service_name_len;
};
#endif

#ifdef HAVE_LINUX_VM_SOCKETS_H
# include <linux/vm_sockets.h>
#endif

#ifdef HAVE_STRUCT_SOCKADDR_VM
# ifdef HAVE_STRUCT_SOCKADDR_VM_SVM_FLAGS
#  define SVM_FLAGS		svm_flags
#  define SVM_ZERO		svm_zero
#  define SVM_ZERO_FIRST	svm_zero[0]
# else
#  define SVM_FLAGS		svm_zero[0]
#  define SVM_ZERO		svm_zero + 1
#  define SVM_ZERO_FIRST	svm_zero[1]
# endif
#else
struct sockaddr_vm {
	uint16_t  svm_family;
	uint16_t svm_reserved1;
	uint32_t svm_port;
	uint32_t svm_cid;
	uint8_t svm_flags;
	uint8_t svm_zero[sizeof(struct sockaddr) - 13];
};
# define SVM_FLAGS	svm_flags
# define SVM_ZERO	svm_zero
# define SVM_ZERO_FIRST	svm_zero[0]
#endif

#ifdef HAVE_LINUX_QRTR_H
# include <linux/qrtr.h>
#else
struct sockaddr_qrtr {
	uint16_t sq_family;
	uint32_t sq_node;
	uint32_t sq_port;
};
#endif

#ifdef HAVE_LINUX_IF_XDP_H
# include <linux/if_xdp.h>
#endif

#ifndef HAVE_STRUCT_SOCKADDR_XDP
struct sockaddr_xdp {
	uint16_t sxdp_family;
	uint16_t sxdp_flags;
	uint32_t sxdp_ifindex;
	uint32_t sxdp_queue_id;
	uint32_t sxdp_shared_umem_fd;
};
#endif

#ifdef HAVE_LINUX_MCTP_H
# include <linux/mctp.h>
#else
struct sockaddr_mctp {
	uint16_t smctp_family;
	uint16_t __smctp_pad0;
	uint32_t  smctp_network;
	struct mctp_addr {
		uint8_t s_addr;
	}        smctp_addr;
	uint8_t  smctp_type;
	uint8_t  smctp_tag;
	uint8_t __smctp_pad1;
};

# define MCTP_NET_ANY	0x0
# define MCTP_ADDR_NULL	0x0
# define MCTP_ADDR_ANY	0xff
#endif

#include "xlat.h"
#include "xlat/xdp_sockaddr_flags.h"
#define XLAT_MACROS_ONLY
# include "xlat/addrfams.h"
#undef XLAT_MACROS_ONLY

#ifndef SKIP_IF_PROC_IS_UNAVAILABLE
# define SKIP_IF_PROC_IS_UNAVAILABLE
#endif
#ifndef FD0_PATH
# define FD0_PATH ""
#endif
#ifndef FD7_PATH
# define FD7_PATH ""
#endif

static void
check_ll(void)
{
	struct sockaddr_ll c_ll = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_ALL),
		.sll_ifindex = 0xfacefeed,
		.sll_hatype = ARPHRD_ETHER,
		.sll_pkttype = PACKET_HOST,
		.sll_halen = sizeof(c_ll.sll_addr),
		.sll_addr = "abcdefgh"
	};
	unsigned int len = sizeof(c_ll);
	int rc = connect(-1, (void *) &c_ll, len);
	const char *errstr = sprintrc(rc);

#if XLAT_RAW
	printf("connect(-1, {sa_family=%#x, sll_protocol=", AF_PACKET);
	print_quoted_hex(&c_ll.sll_protocol, sizeof(c_ll.sll_protocol));
	printf(", sll_ifindex=%u, sll_hatype=%#x"
	       ", sll_pkttype=%u, sll_halen=%u, sll_addr="
	       "[%#02x, %#02x, %#02x, %#02x, %#02x, %#02x, %#02x, %#02x]"
	       "}, %u) = %s\n",
	       c_ll.sll_ifindex, ARPHRD_ETHER,
	       PACKET_HOST, c_ll.sll_halen,
	       c_ll.sll_addr[0], c_ll.sll_addr[1],
	       c_ll.sll_addr[2], c_ll.sll_addr[3],
	       c_ll.sll_addr[4], c_ll.sll_addr[5],
	       c_ll.sll_addr[6], c_ll.sll_addr[7],
	       len, errstr);
#elif XLAT_VERBOSE
	printf("connect(-1, {sa_family=%#x /* AF_PACKET */"
	       ", sll_protocol=", AF_PACKET);
	print_quoted_hex(&c_ll.sll_protocol, sizeof(c_ll.sll_protocol));
	printf(" /* htons(ETH_P_ALL) */"
	       ", sll_ifindex=%u, sll_hatype=%#x /* ARPHRD_ETHER */"
	       ", sll_pkttype=%u /* PACKET_HOST */, sll_halen=%u, sll_addr="
	       "[%#02x, %#02x, %#02x, %#02x, %#02x, %#02x, %#02x, %#02x]"
	       "}, %u) = %s\n",
	       c_ll.sll_ifindex, ARPHRD_ETHER,
	       PACKET_HOST, c_ll.sll_halen,
	       c_ll.sll_addr[0], c_ll.sll_addr[1],
	       c_ll.sll_addr[2], c_ll.sll_addr[3],
	       c_ll.sll_addr[4], c_ll.sll_addr[5],
	       c_ll.sll_addr[6], c_ll.sll_addr[7],
	       len, errstr);

#else /* XLAT_ABBREV */
	printf("connect(-1, {sa_family=AF_PACKET"
	       ", sll_protocol=htons(ETH_P_ALL)"
	       ", sll_ifindex=%u, sll_hatype=ARPHRD_ETHER"
	       ", sll_pkttype=PACKET_HOST, sll_halen=%u, sll_addr="
	       "[%#02x, %#02x, %#02x, %#02x, %#02x, %#02x, %#02x, %#02x]"
	       "}, %u) = %s\n",
	       c_ll.sll_ifindex, c_ll.sll_halen,
	       c_ll.sll_addr[0], c_ll.sll_addr[1],
	       c_ll.sll_addr[2], c_ll.sll_addr[3],
	       c_ll.sll_addr[4], c_ll.sll_addr[5],
	       c_ll.sll_addr[6], c_ll.sll_addr[7],
	       len, errstr);
#endif
}

static void
check_in(void)
{
	const unsigned short h_port = 12345;
	static const char h_addr[] = "127.0.0.1";
	struct sockaddr_in in = {
		.sin_family = AF_INET,
		.sin_port = htons(h_port),
		.sin_addr.s_addr = inet_addr(h_addr)
	};
	unsigned int len = sizeof(in);
	int rc = connect(-1, (void *) &in, len);
	const char * errstr = sprintrc(rc);
#if XLAT_RAW
	printf("connect(-1, {sa_family=%#x, sin_port=", AF_INET);
	print_quoted_hex((const void *) &in.sin_port, sizeof(in.sin_port));
	printf(", sin_addr=");
	print_quoted_hex((const void *) &in.sin_addr.s_addr,
			sizeof(in.sin_addr.s_addr));
	printf("}, %u) = %s\n", len, errstr);
#elif XLAT_VERBOSE
	printf("connect(-1, {sa_family=%#x /* AF_INET */, sin_port=", AF_INET);
	print_quoted_hex((const void *) &in.sin_port, sizeof(in.sin_port));
	printf(" /* htons(%hu) */, sin_addr=", h_port);
	print_quoted_hex((const void *) &in.sin_addr.s_addr,
				sizeof(in.sin_addr.s_addr));
	printf(" /* inet_addr(\"%s\") */}, %u) = %s\n",
			h_addr, len, errstr);
#else /* XLAT_ABBREV */
	printf("connect(-1, {sa_family=AF_INET, sin_port=htons(%hu)"
	       ", sin_addr=inet_addr(\"%s\")}, %u) = %s\n",
	       h_port, h_addr, len, errstr);
#endif
}

static void
validate_in6(struct sockaddr_in6 *const in6, const char *const h_addr)
{
	inet_pton(AF_INET6, h_addr, &in6->sin6_addr);

	unsigned int len = sizeof(*in6);
	int rc = connect(-1, (void *) in6, len);
	const char *errstr = sprintrc(rc);
#if XLAT_RAW
	printf("connect(-1, {sa_family=%#x, sin6_port=", AF_INET6);
	print_quoted_hex(&in6->sin6_port, sizeof(in6->sin6_port));
	printf(", sin6_flowinfo=");
	print_quoted_hex(&in6->sin6_flowinfo, sizeof(in6->sin6_flowinfo));
	printf(", sin6_addr=");
	print_quoted_hex(&in6->sin6_addr, sizeof(struct in6_addr));
	printf(", sin6_scope_id=%u}, %u) = %s\n",
	       in6->sin6_scope_id, len, errstr);
#elif XLAT_VERBOSE
	printf("connect(-1, {sa_family=%#x /* AF_INET6 */", AF_INET6);
	printf(", sin6_port=");
	print_quoted_hex(&in6->sin6_port, sizeof(in6->sin6_port));
	printf(" /* htons(%hu) */", ntohs(in6->sin6_port));
	printf(", sin6_flowinfo=");
	print_quoted_hex(&in6->sin6_flowinfo, sizeof(in6->sin6_flowinfo));
	printf(" /* htonl(%u) */", ntohl(in6->sin6_flowinfo));
	printf(", sin6_addr=");
	print_quoted_hex(&in6->sin6_addr, sizeof(struct in6_addr));
	printf(" /* inet_pton(AF_INET6, \"%s\") */", h_addr);
	printf(", sin6_scope_id=%u}, %u) = %s\n",
	       in6->sin6_scope_id, len, errstr);
#else
	printf("connect(-1, {sa_family=AF_INET6, sin6_port=htons(%hu)"
	       ", sin6_flowinfo=htonl(%u)"
	       ", inet_pton(AF_INET6, \"%s\", &sin6_addr)"
	       ", sin6_scope_id=%u}, %u)"
	       " = %s\n",
	       ntohs(in6->sin6_port), ntohl(in6->sin6_flowinfo),
	       h_addr, in6->sin6_scope_id, len, errstr);
#endif
}

static void
check_in6(void)
{
	struct sockaddr_in6 in6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(12345),
		.sin6_flowinfo = htonl(123456890),
		.sin6_scope_id = 0xfacefeed
	};

	validate_in6(&in6, "12:34:56:78:90:ab:cd:ef");
	validate_in6(&in6, "::");
	validate_in6(&in6, "::1");
}

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
static void
check_sco(void)
{
	const struct sockaddr_sco c_sco = {
		.sco_family = AF_BLUETOOTH,
		.sco_bdaddr.b = "abcdef"
	};
	void *sco = tail_memdup(&c_sco, sizeof(c_sco));
	unsigned int len = sizeof(c_sco);
	int ret = connect(-1, sco, len);
	const char *errstr = sprintrc(ret);
# if XLAT_RAW
	printf("connect(-1, {sa_family=%#x, sco_bdaddr=", AF_BLUETOOTH);
	print_quoted_hex((const void *) &c_sco.sco_bdaddr,
			 sizeof(c_sco.sco_bdaddr));
	printf("}, %u) = %s\n", len, errstr);
# elif XLAT_VERBOSE
	printf("connect(-1, {sa_family=%#x /* AF_BLUETOOTH */"
	       ", sco_bdaddr=", AF_BLUETOOTH);
	print_quoted_hex((const void *) &c_sco.sco_bdaddr,
			 sizeof(c_sco.sco_bdaddr));
	printf(" /* %02x:%02x:%02x:%02x:%02x:%02x */"
	       "}, %u) = %s\n",
	       c_sco.sco_bdaddr.b[0], c_sco.sco_bdaddr.b[1],
	       c_sco.sco_bdaddr.b[2], c_sco.sco_bdaddr.b[3],
	       c_sco.sco_bdaddr.b[4], c_sco.sco_bdaddr.b[5],
	       len, errstr);
# else
	printf("connect(-1, {sa_family=AF_BLUETOOTH"
	       ", sco_bdaddr=%02x:%02x:%02x:%02x:%02x:%02x"
	       "}, %u) = %s\n",
	       c_sco.sco_bdaddr.b[0], c_sco.sco_bdaddr.b[1],
	       c_sco.sco_bdaddr.b[2], c_sco.sco_bdaddr.b[3],
	       c_sco.sco_bdaddr.b[4], c_sco.sco_bdaddr.b[5],
	       len, errstr);
# endif
}

static void
check_rc(void)
{
	const struct sockaddr_rc c_rc = {
		.rc_family = AF_BLUETOOTH,
		.rc_bdaddr.b = "abcdef",
		.rc_channel = 42
	};
	void *rc = tail_memdup(&c_rc, sizeof(c_rc));
	unsigned int len = sizeof(c_rc);
	int ret = connect(-1, rc, len);
	const char *errstr = sprintrc(ret);
# if XLAT_RAW
	printf("connect(-1, {sa_family=%#x, rc_bdaddr=", AF_BLUETOOTH);
	print_quoted_hex((const void *) &c_rc.rc_bdaddr,
			 sizeof(c_rc.rc_bdaddr));
	printf(", rc_channel=%u}, %u) = %s\n", c_rc.rc_channel, len, errstr);
# elif XLAT_VERBOSE
	printf("connect(-1, {sa_family=%#x /* AF_BLUETOOTH */"
	       ", rc_bdaddr=", AF_BLUETOOTH);
	print_quoted_hex((const void *) &c_rc.rc_bdaddr,
			 sizeof(c_rc.rc_bdaddr));
	printf(" /* %02x:%02x:%02x:%02x:%02x:%02x */"
	       ", rc_channel=%u}, %u) = %s\n",
	       c_rc.rc_bdaddr.b[0], c_rc.rc_bdaddr.b[1],
	       c_rc.rc_bdaddr.b[2], c_rc.rc_bdaddr.b[3],
	       c_rc.rc_bdaddr.b[4], c_rc.rc_bdaddr.b[5],
	       c_rc.rc_channel, len, errstr);
# else
	printf("connect(-1, {sa_family=AF_BLUETOOTH"
	       ", rc_bdaddr=%02x:%02x:%02x:%02x:%02x:%02x"
	       ", rc_channel=%u}, %u) = %s\n",
	       c_rc.rc_bdaddr.b[0], c_rc.rc_bdaddr.b[1],
	       c_rc.rc_bdaddr.b[2], c_rc.rc_bdaddr.b[3],
	       c_rc.rc_bdaddr.b[4], c_rc.rc_bdaddr.b[5],
	       c_rc.rc_channel, len, errstr);
# endif
}
#endif /* HAVE_BLUETOOTH_BLUETOOTH_H */

static void
check_nfc(void)
{
	const struct {
		struct sockaddr_nfc sa;
		const char *str;
	} nfc_vecs[] = {
		{ { AF_NFC },
		  "{sa_family=" XLAT_KNOWN(0x27, "AF_NFC") ", dev_idx=0"
		  ", target_idx=0, nfc_protocol=0" NRAW(" /* NFC_PROTO_??? */")
		  "}" },
		{ { AF_NFC, .dev_idx = ifindex_lo(), .target_idx = 1,
		    .nfc_protocol = 1 },
		  "{sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
		  ", dev_idx=" XLAT_KNOWN(1, IFINDEX_LO_STR) ", target_idx=0x1"
		  ", nfc_protocol=" XLAT_KNOWN(0x1, "NFC_PROTO_JEWEL") "}" },
		{ { AF_NFC, .dev_idx = 0xcafebeef, .target_idx = 0xdeadface,
		    .nfc_protocol = 0xfeedbabe },
		  "{sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
		  ", dev_idx=3405692655, target_idx=0xdeadface"
		  ", nfc_protocol=0xfeedbabe" NRAW(" /* NFC_PROTO_??? */")
		  "}" },
	};
	const struct {
		struct sockaddr_nfc_llcp sa;
		const char *str;
	} nfc_llcp_vecs[] = {
		{ { AF_NFC },
		  "{sa_family=" XLAT_KNOWN(0x27, "AF_NFC") ", dev_idx=0"
		  ", target_idx=0, nfc_protocol=0" NRAW(" /* NFC_PROTO_??? */")
		  ", dsap=0, ssap=0, service_name=\"\", service_name_len=0}" },
		{ { AF_NFC, .dev_idx = ifindex_lo(), .target_idx = 0x42,
		    .nfc_protocol = 7, .dsap = 1, .ssap = 5,
		    .service_name_len = 1 },
		  "{sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
		  ", dev_idx=" XLAT_KNOWN(1, IFINDEX_LO_STR) ", target_idx=0x42"
		  ", nfc_protocol=" XLAT_KNOWN(0x7, "NFC_PROTO_ISO15693")
		  ", dsap=0x1" NRAW(" /* LLCP_SAP_SDP */")
		  ", ssap=0x5, service_name=\"\\0\", service_name_len=1}" },
		{ { AF_NFC, .dev_idx = 0xcafed1ce, .target_idx = 0x42,
		    .nfc_protocol = 8, .dsap = 42, .ssap = 0xff,
		    .service_name="OH HAI THAR\0EHLO\n\t\v\f'\"\\\177\333",
		    .service_name_len = 42 },
		  "{sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
		  ", dev_idx=3405697486, target_idx=0x42"
		  ", nfc_protocol=0x8" NRAW(" /* NFC_PROTO_??? */")
		  ", dsap=0x2a, ssap=0xff" NRAW(" /* LLCP_SAP_MAX */")
		  ", service_name=\"OH HAI THAR\\0EHLO\\n\\t\\v\\f'\\\""
		  "\\\\\\177\\333\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0"
		  "\\0\\0\\0\\0\\0\", service_name_len=42}" },
	};

	TAIL_ALLOC_OBJECT_CONST_PTR(struct sockaddr_nfc, sa_nfc);
	TAIL_ALLOC_OBJECT_CONST_PTR(struct sockaddr_nfc_llcp, sa_nfc_llcp);
	int rc;

	fill_memory(sa_nfc, sizeof(*sa_nfc));
	sa_nfc->sa_family = AF_NFC;

	rc = connect(-1, (void *) sa_nfc, sizeof(*sa_nfc) + 1);
	printf("connect(-1, %p, %zu) = %s\n",
	       (void *) sa_nfc, sizeof(*sa_nfc) + 1, sprintrc(rc));

	rc = connect(-1, (void *) sa_nfc, sizeof(*sa_nfc) - 1);
	const char *errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
	       ", sa_data=");
	print_quoted_memory((void *) sa_nfc + sizeof(sa_nfc->sa_family),
			    sizeof(*sa_nfc) - sizeof(sa_nfc->sa_family) - 1);
	printf("}, %zu) = %s\n", sizeof(*sa_nfc) - 1, errstr);

	rc = connect(-1, (void *) sa_nfc, sizeof(*sa_nfc));
	errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
	       ", dev_idx=%u, target_idx=%#x, nfc_protocol=%#x"
	       NRAW(" /* NFC_PROTO_??? */") "}, %zu) = %s\n",
	       sa_nfc->dev_idx, sa_nfc->target_idx, sa_nfc->nfc_protocol,
	       sizeof(*sa_nfc), errstr);

	for (size_t i = 0; i < ARRAY_SIZE(nfc_vecs); i++) {
		*sa_nfc = nfc_vecs[i].sa;

		rc = connect(-1, (void *) sa_nfc, sizeof(*sa_nfc));
		printf("connect(-1, %s, %zu) = %s\n",
		       nfc_vecs[i].str, sizeof(*sa_nfc), sprintrc(rc));
	}

	fill_memory(sa_nfc_llcp, sizeof(*sa_nfc_llcp));
	sa_nfc_llcp->sa_family = AF_NFC;

	rc = connect(-1, (void *) sa_nfc_llcp, sizeof(*sa_nfc_llcp) + 1);
	printf("connect(-1, %p, %zu) = %s\n",
	       (void *) sa_nfc_llcp, sizeof(*sa_nfc_llcp) + 1, sprintrc(rc));

	for (size_t i = 0; i < 3; i++) {
		size_t sz = i ? i == 2 ? sizeof(*sa_nfc_llcp) - 1
				       : sizeof(struct sockaddr_nfc) + 1
			      : sizeof(struct sockaddr_nfc);

		rc = connect(-1, (void *) sa_nfc_llcp, sz);
		errstr = sprintrc(rc);
		printf("connect(-1, {sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
		       ", dev_idx=%u, target_idx=%#x, nfc_protocol=%#x"
		       NRAW(" /* NFC_PROTO_??? */") "%s}, %zu) = %s\n",
		       sa_nfc_llcp->dev_idx, sa_nfc_llcp->target_idx,
		       sa_nfc_llcp->nfc_protocol, i ? ", ..." : "", sz, errstr);
	}

	rc = connect(-1, (void *) sa_nfc_llcp, sizeof(*sa_nfc_llcp));
	errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x27, "AF_NFC")
	       ", dev_idx=%u, target_idx=%#x, nfc_protocol=%#x"
	       NRAW(" /* NFC_PROTO_??? */") ", dsap=%#hhx, ssap=%#hhx"
	       ", service_name=",
	       sa_nfc_llcp->dev_idx, sa_nfc_llcp->target_idx,
	       sa_nfc_llcp->nfc_protocol, sa_nfc_llcp->dsap, sa_nfc_llcp->ssap);
	print_quoted_memory(sa_nfc_llcp->service_name,
			    sizeof(sa_nfc_llcp->service_name));
	printf(", service_name_len=%zu}, %zu) = %s\n",
	       sa_nfc_llcp->service_name_len, sizeof(*sa_nfc_llcp), errstr);

	for (size_t i = 0; i < ARRAY_SIZE(nfc_llcp_vecs); i++) {
		*sa_nfc_llcp = nfc_llcp_vecs[i].sa;

		rc = connect(-1, (void *) sa_nfc_llcp, sizeof(*sa_nfc_llcp));
		printf("connect(-1, %s, %zu) = %s\n",
		       nfc_llcp_vecs[i].str, sizeof(*sa_nfc_llcp),
		       sprintrc(rc));
	}
}

static void
check_vsock(void)
{
	static const struct {
		struct sockaddr_vm sa;
		const char *str;
	} vsock_vecs[] = {
		{ { AF_VSOCK },
		  "{sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
		  ", svm_cid=" XLAT_KNOWN(0, "VMADDR_CID_HYPERVISOR")
		  ", svm_port=0, svm_flags=0}" },
		{ { AF_VSOCK, .svm_cid = 1, .svm_port = 1, .SVM_FLAGS = 1 },
		  "{sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
		  ", svm_cid=" XLAT_KNOWN(0x1, "VMADDR_CID_LOCAL")
		  ", svm_port=0x1"
		  ", svm_flags=" XLAT_KNOWN(0x1, "VMADDR_FLAG_TO_HOST") "}" },
		{ { AF_VSOCK, .svm_reserved1 = 0xdead, .svm_cid = 2,
		    .svm_port = 0xfacebeef, .SVM_FLAGS = 2, },
		  "{sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
		  ", svm_reserved1=0xdead"
		  ", svm_cid=" XLAT_KNOWN(0x2, "VMADDR_CID_HOST")
		  ", svm_port=0xfacebeef"
		  ", svm_flags=" XLAT_UNKNOWN(0x2, "VMADDR_FLAG_???") "}" },
		{ { AF_VSOCK, .svm_cid = 3,
		    .svm_port = 0xfffffffe, .SVM_FLAGS = 0xef,
		    .SVM_ZERO_FIRST = 0x42 },
		  "{sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
		  ", svm_cid=0x3, svm_port=0xfffffffe, svm_flags="
		  XLAT_KNOWN(0xef, "VMADDR_FLAG_TO_HOST|0xee")
		  ", svm_zero=\"\\x42\\x00\\x00\"}" },
		{ { AF_VSOCK, .svm_reserved1 = 0x1, .svm_cid = -1U,
		    .svm_port = -1U, .SVM_FLAGS = 0xfe,
		    .SVM_ZERO_FIRST = 0xae },
		  "{sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
		  ", svm_reserved1=0x1"
		  ", svm_cid=" XLAT_KNOWN(0xffffffff, "VMADDR_CID_ANY")
		  ", svm_port=" XLAT_KNOWN(0xffffffff, "VMADDR_PORT_ANY")
		  ", svm_flags=" XLAT_UNKNOWN(0xfe, "VMADDR_FLAG_???")
		  ", svm_zero=\"\\xae\\x00\\x00\"}" },
	};

	TAIL_ALLOC_OBJECT_CONST_PTR(struct sockaddr_vm, sa_vm);
	int rc;

	fill_memory(sa_vm, sizeof(*sa_vm));
	sa_vm->svm_family = AF_VSOCK;

	rc = connect(-1, (void *) sa_vm, sizeof(*sa_vm) + 1);
	printf("connect(-1, %p, %zu) = %s\n",
	       (void *) sa_vm, sizeof(*sa_vm) + 1, sprintrc(rc));

	rc = connect(-1, (void *) sa_vm, sizeof(*sa_vm) - 1);
	const char *errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
	       ", sa_data=");
	print_quoted_memory((void *) sa_vm + sizeof(sa_vm->svm_family),
			    sizeof(*sa_vm) - sizeof(sa_vm->svm_family)
					     - 1);
	printf("}, %zu) = %s\n", sizeof(*sa_vm) - 1, errstr);

	rc = connect(-1, (void *) sa_vm, sizeof(*sa_vm));
	errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x28, "AF_VSOCK")
	       ", svm_reserved1=%#x, svm_cid=%#x, svm_port=%#x, svm_flags=%#x"
	       NRAW(" /* VMADDR_FLAG_??? */") ", svm_zero=",
	       sa_vm->svm_reserved1, sa_vm->svm_cid, sa_vm->svm_port,
	       sa_vm->SVM_FLAGS);
	print_quoted_hex(sa_vm->SVM_ZERO,
			 (uint8_t *) sa_vm + sizeof(*sa_vm)
					   - (sa_vm->SVM_ZERO));
	printf("}, %zu) = %s\n",  sizeof(*sa_vm), errstr);

	for (size_t i = 0; i < ARRAY_SIZE(vsock_vecs); i++) {
		*sa_vm = vsock_vecs[i].sa;

		rc = connect(-1, (void *) sa_vm, sizeof(*sa_vm));
		printf("connect(-1, %s, %zu) = %s\n",
		       vsock_vecs[i].str, sizeof(*sa_vm), sprintrc(rc));
	}
}

static void
check_qrtr(void)
{
	static const struct {
		struct sockaddr_qrtr sa;
		const char *str;
	} qrtr_vecs[] = {
		{ { AF_QIPCRTR },
		  "{sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
		  ", sq_node=0, sq_port=0}" },
		{ { AF_QIPCRTR, .sq_node = 0xdeadface },
		  "{sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
		  ", sq_node=0xdeadface, sq_port=0}" },
		{ { AF_QIPCRTR, .sq_port = 0xdeedfade },
		  "{sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
		  ", sq_node=0, sq_port=0xdeedfade}" },
		{ { AF_QIPCRTR, .sq_node = 0xfffffffd, .sq_port = 0xfffffffd },
		  "{sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
		  ", sq_node=0xfffffffd, sq_port=0xfffffffd}" },
		{ { AF_QIPCRTR, .sq_node = 0xfffffffe, .sq_port = 0xfffffffe },
		  "{sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
		  ", sq_node=0xfffffffe, sq_port="
		  XLAT_KNOWN(0xfffffffe, "QRTR_PORT_CTRL") "}" },
		{ { AF_QIPCRTR, .sq_node = 0xffffffff, .sq_port = 0xffffffff },
		  "{sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
		  ", sq_node=" XLAT_KNOWN(0xffffffff, "QRTR_NODE_BCAST")
		  ", sq_port=0xffffffff}" },
	};

	TAIL_ALLOC_OBJECT_CONST_PTR(struct sockaddr_qrtr, sa_qrtr);
	int rc;

	fill_memory(sa_qrtr, sizeof(*sa_qrtr));
	sa_qrtr->sq_family = AF_QIPCRTR;

	rc = connect(-1, (void *) sa_qrtr, sizeof(*sa_qrtr) + 1);
	printf("connect(-1, %p, %zu) = %s\n",
	       (void *) sa_qrtr, sizeof(*sa_qrtr) + 1, sprintrc(rc));

	rc = connect(-1, (void *) sa_qrtr, sizeof(*sa_qrtr) - 1);
	const char *errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
	       ", sa_data=");
	print_quoted_memory((void *) sa_qrtr + sizeof(sa_qrtr->sq_family),
			    sizeof(*sa_qrtr) - sizeof(sa_qrtr->sq_family)
					     - 1);
	printf("}, %zu) = %s\n", sizeof(*sa_qrtr) - 1, errstr);

	rc = connect(-1, (void *) sa_qrtr, sizeof(*sa_qrtr));
	errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x2a, "AF_QIPCRTR")
	       ", sq_node=%#x, sq_port=%#x}, %zu) = %s\n",
	       sa_qrtr->sq_node, sa_qrtr->sq_port, sizeof(*sa_qrtr), errstr);

	for (size_t i = 0; i < ARRAY_SIZE(qrtr_vecs); i++) {
		*sa_qrtr = qrtr_vecs[i].sa;

		rc = connect(-1, (void *) sa_qrtr, sizeof(*sa_qrtr));
		printf("connect(-1, %s, %zu) = %s\n",
		       qrtr_vecs[i].str, sizeof(*sa_qrtr), sprintrc(rc));
	}
}

static void
check_xdp(void)
{
	const struct {
		struct sockaddr_xdp sa;
		const char *str;
	} xdp_vecs[] = {
		{ { AF_XDP },
		  "{sa_family=" XLAT_KNOWN(0x2c, "AF_XDP")
		  ", sxdp_flags=0, sxdp_ifindex=0, sxdp_queue_id=0}" },
		{ { AF_XDP, XDP_SHARED_UMEM },
		  "{sa_family=" XLAT_KNOWN(0x2c, "AF_XDP")
		  ", sxdp_flags=" XLAT_KNOWN(0x1, "XDP_SHARED_UMEM")
		  ", sxdp_ifindex=0, sxdp_queue_id=0"
		  ", sxdp_shared_umem_fd=0" FD0_PATH "}" },
		{ { AF_XDP, .sxdp_flags = 0xdead,
		    .sxdp_ifindex = ifindex_lo(),
		    .sxdp_queue_id = 0xfacebeef,
		    .sxdp_shared_umem_fd = 7 },
		  "{sa_family=" XLAT_KNOWN(0x2c, "AF_XDP") ", sxdp_flags="
		  XLAT_KNOWN(0xdead, "XDP_SHARED_UMEM|XDP_ZEROCOPY"
				     "|XDP_USE_NEED_WAKEUP|0xdea0")
		  ", sxdp_ifindex=" XLAT_KNOWN(1, IFINDEX_LO_STR)
		  ", sxdp_queue_id=4207853295"
		  ", sxdp_shared_umem_fd=7" FD7_PATH "}" },
		{ { AF_XDP, .sxdp_flags = 0xbad0,
		    .sxdp_ifindex = 0xcafefade,
		    .sxdp_queue_id = 0xba5ed,
		    .sxdp_shared_umem_fd = 0xfeedbead },
		  "{sa_family=" XLAT_KNOWN(0x2c, "AF_XDP") ", sxdp_flags=0xbad0"
		  NRAW(" /* XDP_??? */") ", sxdp_ifindex=3405707998"
		  ", sxdp_queue_id=763373, sxdp_shared_umem_fd=0xfeedbead}" },
	};

	TAIL_ALLOC_OBJECT_CONST_PTR(struct sockaddr_xdp, sa_xdp);
	int rc;

	fill_memory(sa_xdp, sizeof(*sa_xdp));
	sa_xdp->sxdp_family = AF_XDP;

	rc = connect(-1, (void *) sa_xdp, sizeof(*sa_xdp) + 1);
	printf("connect(-1, %p, %zu) = %s\n",
	       (void *) sa_xdp, sizeof(*sa_xdp) + 1, sprintrc(rc));

	rc = connect(-1, (void *) sa_xdp, sizeof(*sa_xdp) - 1);
	const char *errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x2c, "AF_XDP")
	       ", sa_data=");
	print_quoted_memory((void *) sa_xdp + sizeof(sa_xdp->sxdp_family),
			    sizeof(*sa_xdp) - sizeof(sa_xdp->sxdp_family)
					     - 1);
	printf("}, %zu) = %s\n", sizeof(*sa_xdp) - 1, errstr);

	rc = connect(-1, (void *) sa_xdp, sizeof(*sa_xdp));
	errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x2c, "AF_XDP")
	       ", sxdp_flags=");
#if XLAT_RAW || XLAT_VERBOSE
	printf("%#x" VERB(" /* "), sa_xdp->sxdp_flags);
#endif
#if !XLAT_RAW
	printflags(xdp_sockaddr_flags, sa_xdp->sxdp_flags, "XDP_???");
#endif
	printf(VERB(" */") ", sxdp_ifindex=%u, sxdp_queue_id=%u"
	       ", sxdp_shared_umem_fd=" BE_LE("%d", "%#x") "}, %zu) = %s\n",
	       sa_xdp->sxdp_ifindex, sa_xdp->sxdp_queue_id,
	       sa_xdp->sxdp_shared_umem_fd, sizeof(*sa_xdp), errstr);

	for (size_t i = 0; i < ARRAY_SIZE(xdp_vecs); i++) {
		*sa_xdp = xdp_vecs[i].sa;

		rc = connect(-1, (void *) sa_xdp, sizeof(*sa_xdp));
		printf("connect(-1, %s, %zu) = %s\n",
		       xdp_vecs[i].str, sizeof(*sa_xdp), sprintrc(rc));
	}
}

static void
check_mctp(void)
{
	static const struct {
		struct sockaddr_mctp sa;
		const char *str;
	} mctp_vecs[] = {
		{ { AF_MCTP },
		  "{sa_family=" XLAT_KNOWN(0x2d, "AF_MCTP")
		  ", smctp_network=" XLAT_KNOWN(0, "MCTP_NET_ANY")
		  ", smctp_addr={s_addr=" XLAT_KNOWN(0, "MCTP_ADDR_NULL") "}"
		  ", smctp_type=0, smctp_tag=0}" },
		{ { AF_MCTP, .__smctp_pad0 = 0xdead,
		    .smctp_network = MCTP_NET_ANY,
		    .smctp_addr = { .s_addr = MCTP_ADDR_NULL } },
		  "{sa_family=" XLAT_KNOWN(0x2d, "AF_MCTP")
		  ", __smctp_pad0=0xdead"
		  ", smctp_network=" XLAT_KNOWN(0, "MCTP_NET_ANY")
		  ", smctp_addr={s_addr=" XLAT_KNOWN(0, "MCTP_ADDR_NULL") "}"
		  ", smctp_type=0, smctp_tag=0}" },
		{ { AF_MCTP, .smctp_network = -1234567890,
		    .smctp_addr = { .s_addr = MCTP_ADDR_ANY },
		    .smctp_type = 1, .smctp_tag = 8, .__smctp_pad1 = 0xea },
		  "{sa_family=" XLAT_KNOWN(0x2d, "AF_MCTP")
		  ", smctp_network=0xb669fd2e"
		  ", smctp_addr={s_addr=" XLAT_KNOWN(0xff, "MCTP_ADDR_ANY") "}"
		  ", smctp_type=0x1, smctp_tag=0x8, __smctp_pad1=0xea}" },
		{ { AF_MCTP, .__smctp_pad0 = 0xface,
		    .smctp_network = 2134567890,
		    .smctp_addr = { .s_addr = 0x42 },
		    .smctp_type = 0x23, .smctp_tag = 0x69,
		    .__smctp_pad1= 0xda },
		  "{sa_family=" XLAT_KNOWN(0x2d, "AF_MCTP")
		  ", __smctp_pad0=0xface, smctp_network=0x7f3aebd2"
		  ", smctp_addr={s_addr=0x42}, smctp_type=0x23"
		  ", smctp_tag=0x69, __smctp_pad1=0xda}" },
	};

	TAIL_ALLOC_OBJECT_CONST_PTR(struct sockaddr_mctp, sa_mctp);
	int rc;

	fill_memory(sa_mctp, sizeof(*sa_mctp));
	sa_mctp->smctp_family = AF_MCTP;

	rc = connect(-1, (void *) sa_mctp, sizeof(*sa_mctp) + 1);
	printf("connect(-1, %p, %zu) = %s\n",
	       (void *) sa_mctp, sizeof(*sa_mctp) + 1, sprintrc(rc));

	rc = connect(-1, (void *) sa_mctp, sizeof(*sa_mctp) - 1);
	const char *errstr = sprintrc(rc);
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x2d, "AF_MCTP")
	       ", sa_data=");
	print_quoted_memory((void *) sa_mctp + sizeof(sa_mctp->smctp_family),
			    sizeof(*sa_mctp) - sizeof(sa_mctp->smctp_family)
					     - 1);
	printf("}, %zu) = %s\n", sizeof(*sa_mctp) - 1, errstr);

	rc = connect(-1, (void *) sa_mctp, sizeof(*sa_mctp));
	printf("connect(-1, {sa_family=" XLAT_KNOWN(0x2d, "AF_MCTP")
	       ", __smctp_pad0=%#hx, smctp_network=%#x"
	       ", smctp_addr={s_addr=%#hhx}, smctp_type=%#hhx, smctp_tag=%#hhx"
	       ", __smctp_pad1=%#hhx}, %zu) = %s\n",
	       sa_mctp->__smctp_pad0, sa_mctp->smctp_network,
	       sa_mctp->smctp_addr.s_addr, sa_mctp->smctp_type,
	       sa_mctp->smctp_tag, sa_mctp->__smctp_pad1, sizeof(*sa_mctp),
	       sprintrc(rc));

	for (size_t i = 0; i < ARRAY_SIZE(mctp_vecs); i++) {
		*sa_mctp = mctp_vecs[i].sa;

		rc = connect(-1, (void *) sa_mctp, sizeof(*sa_mctp));
		printf("connect(-1, %s, %zu) = %s\n",
		       mctp_vecs[i].str, sizeof(*sa_mctp), sprintrc(rc));
	}
}

int
main(void)
{
	SKIP_IF_PROC_IS_UNAVAILABLE;

	check_ll();
	check_in();
	check_in6();
#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
	check_sco();
	check_rc();
#endif
	check_nfc();
	check_vsock();
	check_qrtr();
	check_xdp();
	check_mctp();

	puts("+++ exited with 0 +++");
	return 0;
}
