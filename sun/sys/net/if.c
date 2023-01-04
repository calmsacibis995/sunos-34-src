/*	@(#)if.c 1.3 86/10/13 SMI; from UCB 6.2 83/09/27	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../net/nit.h"

#include "../netinet/in.h"

int	ifqmaxlen = IFQ_MAXLEN;

/*
 * Network interface utility routines.
 *
 * Routines with if_ifwith* names take sockaddr *'s as
 * parameters.  Other routines take value parameters,
 * e.g. if_ifonnetof takes the network number.
 */

ifinit()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_init) {
			(*ifp->if_init)(ifp->if_unit);
			if (ifp->if_snd.ifq_maxlen == 0)
				ifp->if_snd.ifq_maxlen = ifqmaxlen;
		}
	if_slowtimo();
}

#ifdef vax
/*
 * Call each interface on a Unibus reset.
 */
ifubareset(uban)
	int uban;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_reset)
			(*ifp->if_reset)(ifp->if_unit, uban);
}
#endif

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
if_attach(ifp)
	struct ifnet *ifp;
{
	register struct ifnet **p = &ifnet;

	while (*p)
		p = &((*p)->if_next);
	*p = ifp;
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifnet *
if_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;

#define	equal(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_addr.sa_family != addr->sa_family)
			continue;
		if (equal(&ifp->if_addr, addr))
			break;
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    equal(&ifp->if_broadaddr, addr))
			break;
	}
	return (ifp);
}

/*
 * Locate an interface based on a complete destination.
 */
/*ARGSUSED*/
struct ifnet *
if_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifnet *bifp = (struct ifnet *)0;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_addr.sa_family != addr->sa_family)
			continue;
		if ((ifp->if_flags & IFF_POINTOPOINT) &&
			equal(&ifp->if_dstaddr, addr))
			break;
		if (equal(&ifp->if_addr, addr))
			break;
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    equal(&ifp->if_broadaddr, addr))
			bifp = ifp;
	}
	return (ifp==0 ? bifp : ifp);
}

/*
 * Find an interface which reaches a specific destination.  If many, choice
 * is first found.
 */
struct ifnet *
if_ifwithnet(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register u_int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= AF_MAX)
		return (0);
	netmatch = afswitch[af].af_netmatch;
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (af != ifp->if_addr.sa_family)
			continue;
		if (ifp->if_flags&IFF_POINTOPOINT) {
			if (equal(&ifp->if_dstaddr, addr))
				break;
		} else if ((*netmatch)(addr, &ifp->if_addr))
			break;
	}
	return (ifp);
}

/*
 * As above, but parameter is network number.
 */
struct ifnet *
if_ifonnetof(net)
	register int net;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_net == net)
			break;
	return (ifp);
}

/*
 * Find an interface using a specific address family
 */
struct ifnet *
if_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_addr.sa_family == af)
			break;
	return (ifp);
}

/*
 * Find an UP interface using a specific address family
 */
struct ifnet *
if_ifwithafup(af)
	register int af;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if ((ifp->if_flags & IFF_UP) && ifp->if_addr.sa_family == af)
			break;
	return (ifp);
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splnet or eqivalent.
 */
if_down(ifp)
	register struct ifnet *ifp;
{
	register int (*rtn)() = ifp->if_ioctl;
	int flags = ifp->if_flags;

	flags &= ~IFF_UP;
	if (rtn == NULL || (*rtn)(ifp, SIOCSIFFLAGS, &flags) != 0)
		ifp->if_flags = flags;
	pfctlinput(PRC_IFDOWN, (caddr_t)&ifp->if_addr);
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
if_slowtimo()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp->if_unit);
	}
	timeout(if_slowtimo, (caddr_t)0, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name, size)
	register char *name;
	register size;
{
	register char *cp;
	register struct ifnet *ifp;
	int unit;
	int namlen = 0;

	for (cp = name; cp < name + size && *cp; cp++) {
		if (*cp >= '0' && *cp <= '9')
			break;
		namlen++;
	}
	if (*cp == '\0' || cp == name + size || cp == name)
		return ((struct ifnet *)0);
	unit = 0;
	while (*cp && cp < name + IFNAMSIZ) 
		unit = 10*unit + (*cp++ - '0');
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (unit == ifp->if_unit && namlen == strlen(ifp->if_name) &&
		    bcmp(ifp->if_name, name, namlen) == 0)
			return (ifp);
	return ((struct ifnet *)0);
}

/*
 * Interface ioctls.
 */
#ifndef NIT
/*ARGSUSED*/
#endif !NIT
ifioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	register struct ifnet *ifp, *oifp;
	register struct ifreq *ifr;
	int error = 0;

	switch (cmd) {

	case SIOCGIFCONF:
		return (ifconf(cmd, data));

	case SIOCSARP:
	case SIOCDARP:
		if (!suser())
			return (u.u_error);
		/* fall through */
	case SIOCGARP:
#include "ether.h"
#if NETHER == 0
		return (ENOPROTOOPT);
#else
		return (arpioctl(cmd, data));
#endif

	case SIOCSIFADDR:
	case SIOCSIFFLAGS:
	case SIOCSIFDSTADDR:
	case SIOCSIFMTU:
		if (!suser())
			return (u.u_error);
		break;
	}

	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name, IFNAMSIZ);
#ifndef NIT
	if (ifp == 0)
		return (ENXIO);
#else NIT
	if (ifp == 0 || (so->so_proto->pr_family == PF_NIT &&
		    so->so_proto->pr_type == SOCK_RAW)) {
		/* handle direct connects of ioctl's to interfaces */
		/* XXX consider attempting other paths if this one fails. */
		register struct rawcb *rcb = (struct rawcb *)so->so_pcb;

		if (ifp == 0 && rcb->rcb_flags & RAW_LADDR &&
		    rcb->rcb_laddr.sa_family == AF_NIT)
			ifp = nit_ifwithaddr(&rcb->rcb_laddr);
		if (ifp == 0)
			return (ENXIO);
		if (ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		if (!suser())		/* XXX redundant */
			return (u.u_error);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
	}
#endif NIT

	switch (cmd) {

	case SIOCGIFMTU:
		*(int *)ifr->ifr_data = ifp->if_mtu;
		break;

	case SIOCGIFADDR:
		ifr->ifr_addr = ifp->if_addr;
		break;

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		ifp->if_dstaddr = ifr->ifr_dstaddr;
		if (ifp->if_dstaddr.sa_family == AF_INET) {
			struct sockaddr_in *sin;

			sin = (struct sockaddr_in *)&ifp->if_dstaddr;
			ifp->if_net = in_netof(sin->sin_addr);
		}
		break;

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		ifr->ifr_dstaddr = ifp->if_dstaddr;
		break;

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags&IFF_UP) &&
		    (ifr->ifr_flags&IFF_UP) == 0) {
			int s = splimp();
			if_down(ifp);
			(void) splx(s);
		}

		if (ifp->if_ioctl != 0)
			/* tell the interface too */
			error = (*ifp->if_ioctl)(ifp, cmd, ifr->ifr_data);
		if (error && error != EINVAL)			/* XXX */
			return (error);
		if (ifp->if_ioctl == 0 || error == EINVAL)
			ifp->if_flags = ifr->ifr_flags;
		break;

	case SIOCSIFADDR:
#include "nd.h"
#if NND > 0
		{
			extern struct ifnet *ndif;

			if (ifr->ifr_addr.sa_family != AF_UNSPEC &&
			    ifp == ndif &&
			    (error = nd_chknewaddr(&ifr->ifr_addr)))
				return (error);
		}
#endif
		return (if_setaddr(ifp, &ifr->ifr_addr));

	case SIOCUPPER:
		oifp = ifunit(ifr->ifr_oname, IFNAMSIZ);
		if (oifp == 0)
			return (ENXIO);
		if (oifp->if_input == 0)
			return (EINVAL);
		ifp->if_upper = oifp;
		break;

	case SIOCLOWER:
		oifp = ifunit(ifr->ifr_oname, IFNAMSIZ);
		if (oifp == 0)
			return (ENXIO);
		if (oifp->if_output == 0)
			return (EINVAL);
		ifp->if_lower = oifp;
		break;

	case SIOCSIFNETMASK:
		in_setnetmask(ifp, (struct sockaddr_in *)&ifr->ifr_addr);
		break;

	case SIOCGIFNETMASK:
		in_getnetmask(ifp, (struct sockaddr_in *)&ifr->ifr_addr);
		break;

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		ifr->ifr_addr = ifp->if_broadaddr;
		break;

	case SIOCSIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		ifp->if_broadaddr = ifr->ifr_addr;
		break;

	default:
		if (ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, ifr->ifr_data));
	}
	return (0);
}

/*
 * Set a new interface address.
 * Called by ND address assignment as well as above
 * XXX Most of what the drivers do is device independent!
 */
if_setaddr(ifp, sa)
	register struct ifnet *ifp;
	struct sockaddr *sa;
{
	if (ifp->if_ioctl == 0)
		return (EOPNOTSUPP);
	return ((*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)sa));
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
ifconf(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp = ifnet;
	register char *cp, *ep;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;

	ifrp = ifc->ifc_req;
	ep = ifr.ifr_name + sizeof (ifr.ifr_name) - 2;
	for (; space > sizeof (ifr) && ifp; ifp = ifp->if_next) {
		bcopy(ifp->if_name, ifr.ifr_name, sizeof (ifr.ifr_name) - 2);
		for (cp = ifr.ifr_name; cp < ep && *cp; cp++)
			;
		*cp++ = '0' + ifp->if_unit; *cp = '\0';
		ifr.ifr_addr = ifp->if_addr;
		error = copyout((caddr_t)&ifr, (caddr_t)ifrp, sizeof (ifr));
		if (error)
			break;
		space -= sizeof (ifr), ifrp++;
	}
	ifc->ifc_len -= space;
	return (error);
}
