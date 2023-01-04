#ifndef lint
static char sccsid[] = "@(#)subr_kudp.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

/*
 * subr_kudp.c
 * Subroutines to do UDP/IP sendto and recvfrom in the kernel
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */
#include "../h/param.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"

struct mbuf     *mclgetx();

/*
 * General kernel udp stuff.
 * The routines below are used by both the client and the server side
 * rpc code.
 */

/*
 * Kernel recvfrom.
 * Pull address mbuf and data mbuf chain off socket receive queue.
 */
struct mbuf *
ku_recvfrom(so, from)
	register struct socket *so;
	struct sockaddr_in *from;
{
	register struct mbuf	*m;
	register struct mbuf	*m0;
	int		len = 0;

#ifdef RPCDEBUG
	rpc_debug(4, "urrecvfrom so=%X\n", so);
#endif
	sblock(&so->so_rcv);
	m = so->so_rcv.sb_mb;
	if (m == NULL) {
		sbunlock(&so->so_rcv);
		return (m);
	}

	*from = *mtod(m, struct sockaddr_in *);
	sbfree(&so->so_rcv, m);
	MFREE(m, m0);
	if (m0 == NULL) {
		printf("cku_recvfrom: no body!\n");
		so->so_rcv.sb_mb = m0;
		sbunlock(&so->so_rcv);
		return (m0);
	}

	/*
	 * Walk down mbuf chain till m_act set (end of packet) or
	 * end of chain freeing socket buffer space as we go.
	 * After the loop m points to the last mbuf in the packet.
	 */
	m = m0;
	for (;;) {
		sbfree(&so->so_rcv, m);
		len += m->m_len;
		if (m->m_act || m->m_next == NULL) {
			break;
		}
		m = m->m_next;
	}

	so->so_rcv.sb_mb = m->m_next;
	m->m_next = NULL;
	if (len > UDPMSGSIZE) {
		printf("ku_recvfrom: len = %d\n", len);
	}

#ifdef RPCDEBUG
	rpc_debug(4, "urrecvfrom %d from %X\n", len, from->sin_addr.s_addr);
#endif
	sbunlock(&so->so_rcv);
	return (m0);
}

int Sendtries = 0;
int Sendok = 0;

/*
 * Kernel sendto.
 * Set addr and send off via UDP.
 * Use ku_fastsend if possible.
 */
int
ku_sendto_mbuf(so, m, addr)
	struct socket *so;
	struct mbuf *m;
	struct sockaddr_in *addr;
{
#ifdef SLOWSEND
	register struct inpcb *inp = sotoinpcb(so);
	int s;
#endif
	int error;

#ifdef RPCDEBUG
	rpc_debug(4, "ku_sendto_mbuf %X\n", addr->sin_addr.s_addr);
#endif
	Sendtries++;
#ifdef SLOWSEND
	s = splnet();
	if (error = in_pcbsetaddr(inp, addr)) {
		printf("pcbsetaddr failed %d\n", error);
		(void) splx(s);
		m_freem(m);
		return (error);
	}
	error = udp_output(inp, m);
	in_pcbdisconnect(inp);
	(void) splx(s);
#else
	error = ku_fastsend(so, m, addr);
#endif
	Sendok++;
#ifdef RPCDEBUG
	rpc_debug(4, "ku_sendto returning %d\n", error);
#endif
	return (error);
}

#ifdef RPCDEBUG
int rpcdebug = 2;

/*VARARGS2*/
rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
        int level;
        char *str;
        int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{

        if (level <= rpcdebug)
                printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif
