/*	@(#)uipc_socket.c 1.1 86/09/25 SMI; from UCB 6.2 83/09/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/un.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../net/if.h"

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO:
 *	sostat
 *	test socketpair
 *	PR_RIGHTS
 *	clean up select, async
 *	out-of-band is a kludge
 */
/*ARGSUSED*/
socreate(dom, aso, type, proto)
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register struct mbuf *m;
	register int error;

	if (proto)
		prp = pffindproto(dom, proto);
	else
		prp = pffindtype(dom, type);
	if (prp == 0)
		return (EPROTONOSUPPORT);
	if (prp->pr_type != type)
		return (EPROTOTYPE);
	m = m_getclr(M_WAIT, MT_SOCKET);
	if (m == 0)
		return (ENOBUFS);
	so = mtod(m, struct socket *);
	so->so_options = 0;
	so->so_state = 0;
	so->so_type = type;
	if (u.u_uid == 0)
		so->so_state = SS_PRIV;
	so->so_proto = prp;
	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	error = (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s = splnet(), error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		(void) splx(s);
		return (error);
	}
	if (so->so_q == 0) {
		so->so_q = so;
		so->so_q0 = so;
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = MIN(backlog, SOMAXCONN);
	(void) splx(s);
	return (0);
}

sofree(so)
	register struct socket *so;
{

	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1))
			panic("sofree dq");
		so->so_head = 0;
	}
	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;
	sbrelease(&so->so_snd);
	sorflush(so);
	(void) m_free(dtom(so));
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */
soclose(so)
	register struct socket *so;
{
	int s = splnet();		/* conservative */
	int error;

	if (so->so_options & SO_ACCEPTCONN) {
		while (so->so_q0 != so)
			(void) soabort(so->so_q0);
		while (so->so_q != so)
			(void) soabort(so->so_q);
	}
	if (so->so_pcb == 0)
		goto discard;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so, (struct mbuf *)0);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO))
				goto drop;
			while (so->so_state & SS_ISCONNECTED)
				(void) sleep((caddr_t)&so->so_timeo, PZERO+1);
		}
	}
drop:
	if (so->so_pcb) {
		int error2 =
		    (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0)
			error = error2;
	}
discard:
	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
	(void) splx(s);
	return (error);
}

/*
 * Must be called at splnet...
 */
soabort(so)
	struct socket *so;
{

	return (
	    (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
}

soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING)) {
		error = EISCONN;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
bad:
	(void) splx(s);
	return (error);
}

soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int s = splnet();
	int error;

	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
	    (struct mbuf *)0, (struct mbuf *)so2, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

sodisconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
bad:
	(void) splx(s);
	return (error);
}

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 */
sosend(so, nam, uio, flags, rights)
	register struct socket *so;
	struct mbuf *nam;
	register struct uio *uio;
	int flags;
	struct mbuf *rights;
{
	struct mbuf *top = 0;
	register struct mbuf *m, **mp = &top;
	register int space;
	int len, error = 0, s, dontroute, first = 1;
	struct sockbuf sendtempbuf;

	if (sosendallatonce(so) && uio->uio_resid > so->so_snd.sb_hiwat)
		return (EMSGSIZE);
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);
restart:
	sblock(&so->so_snd);
#define	snderr(errno)	{ error = errno; (void) splx(s); goto release; }

	u.u_ru.ru_msgsnd++;
again:
	s = splnet();
	if (so->so_state & SS_CANTSENDMORE) {
		psignal(u.u_procp, SIGPIPE);
		snderr(EPIPE);
	}
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0;				/* ??? */
		(void) splx(s);
		goto release;
	}
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		if (so->so_proto->pr_flags & PR_CONNREQUIRED)
			snderr(ENOTCONN);
		if (nam == 0)
			snderr(EDESTADDRREQ);
	}
	if (flags & MSG_OOB)
		space = 1024;
	else {
		space = sbspace(&so->so_snd);
		if (space <= 0 ||
		    sosendallatonce(so) && space < uio->uio_resid) {
			/*
			 * Need to send everything in a single crack,
			 * but don't have enough space to do it.
			 * If non-blocking, return EWOULDBLOCK, unless
			 * some data has already been written out.
			 */
			if (so->so_state & SS_NBIO) {
				if (first)
					error = EWOULDBLOCK;
				(void) splx(s);
				goto release;
			}
			sbunlock(&so->so_snd);
			sbwait(&so->so_snd);
			(void) splx(s);
			goto restart;
		}
	}
	(void) splx(s);
	/*
	 * Temporary kludge-- don't want to update so_snd in this loop
	 * (will be done when sent), but need to recalculate
	 * space on each iteration.  For now, copy so_snd into a tmp.
	 */
	sendtempbuf = so->so_snd;
	while (uio->uio_resid > 0 && space > 0) {
		register struct iovec *iov = uio->uio_iov;

		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("sosend");
			continue;
		}
		MGET(m, M_WAIT, MT_DATA);
		if (m == NULL) {
			error = ENOBUFS;			/* SIGPIPE? */
			goto release;
		}
		if (iov->iov_len >= MCLBYTES && space >= MCLBYTES) {
			if (mclget(m) == 0)
				goto nopages;
			len = MCLBYTES;
		} else {
nopages:
			len = MIN(MLEN, iov->iov_len);
		}
		if (uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio))
			error = EFAULT;
		m->m_len = len;
		*mp = m;
		if (error)
			goto release;
		mp = &m->m_next;
		if (flags & MSG_OOB)
			space -= len;
		else {
			sballoc(&sendtempbuf, m);
			space = sbspace(&sendtempbuf);
		}
	}

	if (dontroute)
		so->so_options |= SO_DONTROUTE;
	error = (*so->so_proto->pr_usrreq)(so,
	    (flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
	    top, (caddr_t)nam, rights);
	if (dontroute)
		so->so_options &= ~SO_DONTROUTE;
	top = 0;
	first = 0;
	if (error) {
		(void) splx(s);
		goto release;
	}
	mp = &top;

	if (uio->uio_resid == 0) {
		(void) splx(s);
		goto release;
	}
	goto again;

release:
	sbunlock(&so->so_snd);
	if (top)
		m_freem(top);
	return (error);
}

soreceive(so, aname, uio, flags, rightsp)
	register struct socket *so;
	struct mbuf **aname;
	register struct uio *uio;
	int flags;
	struct mbuf **rightsp;
{
	register struct mbuf *m, *n;
	register int len, error = 0, s, eor, tomark;
	struct protosw *pr = so->so_proto;
	int moff;

	if (rightsp)
		*rightsp = 0;
	if (aname)
		*aname = 0;
	if (flags & MSG_OOB) {
		m = m_get(M_WAIT, MT_DATA);
		if (m == 0)
			return (ENOBUFS);
		error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
		    m, (struct mbuf *)0, (struct mbuf *)0);
		if (error)
			goto bad;
		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;
			if (uiomove(mtod(m, caddr_t), (int)len, UIO_READ,uio))
				error = EFAULT;
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		return (error);
	}

restart:
	sblock(&so->so_rcv);
	s = splnet();

#define	rcverr(errno)	{ error = errno; (void) splx(s); goto release; }
	if (so->so_rcv.sb_cc == 0) {
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			(void) splx(s);
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
			(void) splx(s);
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED))
			rcverr(ENOTCONN);
		if (uio->uio_resid == 0)
			goto release;
		if (so->so_state & SS_NBIO)
			rcverr(EWOULDBLOCK);
		sbunlock(&so->so_rcv);
		sbwait(&so->so_rcv);
		(void) splx(s);
		goto restart;
	}
	u.u_ru.ru_msgrcv++;
	m = so->so_rcv.sb_mb;
	if (m == 0)
		panic("receive");
	if (pr->pr_flags & PR_ADDR) {
		if ((flags & MSG_PEEK) == 0) {
			so->so_rcv.sb_cc -= m->m_len;
			so->so_rcv.sb_mbcnt -= MSIZE;
		}
		if (aname) {
			if (flags & MSG_PEEK) {
				*aname = m_copy(m, 0, m->m_len);
				if (*aname == NULL)
					panic("receive 2");
			} else
				*aname = m;
			m = m->m_next;
			(*aname)->m_next = 0;
		} else
			if (flags & MSG_PEEK)
				m = m->m_next;
			else
				m = m_free(m);
		if (m == 0)
			panic("receive 2a");
		if (rightsp) {
			if (pr->pr_flags & PR_RIGHTS) {
				if (m->m_len)
					*rightsp = m_copy(m, 0, m->m_len);
				else {
					*rightsp = m_get(M_DONTWAIT, MT_SONAME);
					if (*rightsp)
						(*rightsp)->m_len = 0;
				}
#ifdef notdef
				if (*rightsp == NULL)
					panic("receive 2b");
#endif
			} else
				*rightsp = 0;
		}
		if (pr->pr_flags & PR_RIGHTS) {
			if (flags & MSG_PEEK)
				m = m->m_next;
			else {
				so->so_rcv.sb_cc -= m->m_len;
				so->so_rcv.sb_mbcnt -= MSIZE;
				m = m_free(m);
			}
			if (m == 0)
				panic("receive 3");
		}
		if ((flags & MSG_PEEK) == 0)
			so->so_rcv.sb_mb = m;
	}
	eor = 0;
	moff = 0;
	tomark = so->so_oobmark;
	do {
		if (uio->uio_resid <= 0)
			break;
		len = uio->uio_resid;
		so->so_state &= ~SS_RCVATMARK;
		if (tomark && len > tomark)
			len = tomark;
		if (moff+len > m->m_len - moff)
			len = m->m_len - moff;
		(void) splx(s);
		if (uiomove(mtod(m, caddr_t) + moff, (int)len, UIO_READ, uio))
			error = EFAULT;
		s = splnet();
		if (len == m->m_len) {
			eor = (int)m->m_act;
			if (flags & MSG_PEEK)
				m = m->m_next;
			else {
				sbfree(&so->so_rcv, m);
				MFREE(m, n);
				m = n;
				so->so_rcv.sb_mb = m;
			}
			moff = 0;
		} else {
			if (flags & MSG_PEEK)
				moff += len;
			else {
				m->m_off += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if ((flags & MSG_PEEK) == 0 && so->so_oobmark) {
			so->so_oobmark -= len;
			if (so->so_oobmark == 0) {
				so->so_state |= SS_RCVATMARK;
				break;
			}
		}
		if (tomark) {
			tomark -= len;
			if (tomark == 0)
				break;
		}
	} while (m && error == 0 && !eor);
	if (flags & MSG_PEEK)
		goto release;
	if ((so->so_proto->pr_flags & PR_ATOMIC) && eor == 0)
		do {
			if (m == 0)
				panic("receive 4");
			sbfree(&so->so_rcv, m);
			eor = (int)m->m_act;
			so->so_rcv.sb_mb = m->m_next;
			MFREE(m, n);
			m = n;
		} while (eor == 0);
	if ((so->so_proto->pr_flags & PR_WANTRCVD) && so->so_pcb)
		(*so->so_proto->pr_usrreq)(so, PRU_RCVD,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
release:
	sbunlock(&so->so_rcv);
	if (error == 0 && rightsp &&
	    *rightsp && so->so_proto->pr_family == AF_UNIX)
		error = unp_externalize(*rightsp);
	(void) splx(s);
	return (error);
}

soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr = so->so_proto;

	how++;
	if (how & FREAD)
		sorflush(so);
	if (how & FWRITE)
		return ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
	return (0);
}

sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	register int s;
	struct sockbuf asb;

	sblock(sb);
	s = splimp();
	socantrcvmore(so);
	sbunlock(sb);
	asb = *sb;
	bzero((caddr_t)sb, sizeof (*sb));
	(void) splx(s);
	if (pr->pr_family == AF_UNIX && (pr->pr_flags & PR_RIGHTS))
		unp_scan(asb.sb_mb, unp_discard);
	sbrelease(&asb);
}

sosetopt(so, level, optname, m)
	register struct socket *so;
	int level, optname;
	register struct mbuf *m;
{

	if (level != SOL_SOCKET)
		return (EINVAL);	/* XXX */
	switch (optname) {

	case SO_DEBUG:
	case SO_KEEPALIVE:
	case SO_DONTROUTE:
	case SO_USELOOPBACK:
	case SO_REUSEADDR:
		so->so_options |= optname;
		break;

	case SO_LINGER:
		if (m == NULL || m->m_len != sizeof (int))
			return (EINVAL);
		so->so_options |= SO_LINGER;
		so->so_linger = *mtod(m, int *);
		break;

	case SO_DONTLINGER:
		so->so_options &= ~SO_LINGER;
		so->so_linger = 0;
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

sogetopt(so, level, optname, m)
	register struct socket *so;
	int level, optname;
	register struct mbuf *m;
{

	if (level != SOL_SOCKET)
		return (EINVAL);	/* XXX */
	switch (optname) {

	case SO_USELOOPBACK:
	case SO_DONTROUTE:
	case SO_DEBUG:
	case SO_KEEPALIVE:
	case SO_LINGER:
	case SO_REUSEADDR:
		if ((so->so_options & optname) == 0)
			return (ENOPROTOOPT);
		if (optname == SO_LINGER && m != NULL) {
			*mtod(m, int *) = so->so_linger;
			m->m_len = sizeof (int);
		}
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

sohasoutofband(so)
	register struct socket *so;
{

	if (so->so_pgrp == 0)
		return;
	if (so->so_pgrp > 0)
		gsignal(so->so_pgrp, SIGURG);
	else {
		struct proc *p = pfind(-so->so_pgrp);

		if (p)
			psignal(p, SIGURG);
	}
}
