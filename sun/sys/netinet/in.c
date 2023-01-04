/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)in.c 1.6 86/12/29 SMI; from UCB 7.1 6/5/86
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../net/af.h"

#ifdef INET
u_long subnet_mask = 0;	/* only one subnet mask supported */
u_long subnet_net = 0;	/* network on which above applies */

inet_hash(sin, hp)
	register struct sockaddr_in *sin;
	struct afhash *hp;
{
	u_long net;

	net = in_netof(sin->sin_addr);
	if (net == subnet_net)
		hp->afh_nethash = sin->sin_addr.s_addr & subnet_mask;
	else
		hp->afh_nethash = net;
	hp->afh_hosthash = ntohl(sin->sin_addr.s_addr);
}

inet_netmatch(sin1, sin2)
	struct sockaddr_in *sin1, *sin2;
{
	u_long net1;

	net1 = in_netof(sin1->sin_addr);
	if (net1 == subnet_net && subnet_net != 0) {
		  /*
		   * Should possibly check for subnet zero being a wild-card
		   * matcher?
		   */
		return( (sin1->sin_addr.s_addr & subnet_mask) ==
		        (sin2->sin_addr.s_addr & subnet_mask) );
	}
	return (net1 == in_netof(sin2->sin_addr));
}

/*
 * Formulate an Internet address from network + host.  Used in
 * building addresses stored in the ifnet structure.
 */
struct in_addr
if_makeaddr(net, host)
	int net, host;
{
	u_long addr;

	if (net < 128)
		addr = (net << IN_CLASSA_NSHIFT) | (IN_CLASSA_HOST & host);
	else if (net < 65536)
		addr = (net << IN_CLASSB_NSHIFT) | (IN_CLASSB_HOST & host);
	else
		addr = (net << IN_CLASSC_NSHIFT) | (IN_CLASSC_HOST & host);
	addr = htonl(addr);
	return (*(struct in_addr *)&addr);
}

/*
 * Return the network (NOT subnet) number from an internet address.
 * we must shift this to be compatible with everybody else.
 */
in_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return (((i)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
	else if (IN_CLASSB(i))
		return (((i)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
	else
		return (((i)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
}

/*
 * Return the host portion of an internet address.
 */
in_lnaof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net, host;

	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		host = i & IN_CLASSA_HOST;
	} else if (IN_CLASSB(i)) {
		net = i & IN_CLASSB_NET;
		host = i & IN_CLASSB_HOST;
	} else {
		net = i & IN_CLASSC_NET;
		host = i & IN_CLASSC_HOST;
	}

	/*
	 * Check whether network is a subnet;
	 * if so, use the modified interpretation of `host'.
	 */
	 if (net == subnet_net)
	 	return(host & ~subnet_mask);
	return (host);
}

#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif
int subnetsarelocal = SUBNETSARELOCAL;
/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */

in_localaddr(in)
	struct in_addr in;
{
	register u_long net = ntohl(in.s_addr);
	register struct ifnet *ifp;

	if (subnetsarelocal || net != subnet_net) {
		for (ifp = ifnet; ifp; ifp = ifp->if_next)
			if (net == ifp->if_net)
				return (1);
	} else {
		for (ifp = ifnet; ifp; ifp = ifp->if_next)
			if ((in.s_addr & subnet_mask) == 
     (((struct sockaddr_in *)&ifp->if_addr)->sin_addr.s_addr & subnet_mask))
				return (1);
	}
	return (0);
}

/*
 * take an interface pointer and return the network number in the form
 * used for routing in this hybrid scheme: left justified with subnet bits
 * included when the network is subnetted.
 */
struct in_addr if_makenet(ifp)
	register struct ifnet *ifp;
{
	if (ifp->if_net !=0 && ifp->if_net == subnet_net) {
		struct sockaddr_in *sin;
		struct in_addr addr;

		sin = (struct sockaddr_in *)&ifp->if_addr;
		addr.s_addr = sin->sin_addr.s_addr & subnet_mask;
		return(addr);
   	}
	return(if_makeaddr(ifp->if_net, INADDR_ANY));
}

/*
 * Initialize an interface's routing
 * table entry according to the network.
 * INTERNET SPECIFIC.
 */
if_rtinit(ifp, flags)
	register struct ifnet *ifp;
	int flags;
{
	struct sockaddr_in sin;

	if ((ifp->if_flags & IFF_ROUTE) && (flags != -1))
		return;
	bzero((caddr_t)&sin, sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = if_makenet(ifp);
	rtinit((struct sockaddr *)&sin, &ifp->if_addr, flags);
}

/*
 * return the subnet mask for the given interface
 */
in_getnetmask(ifp, sin)
	register struct ifnet *ifp;
	struct sockaddr_in *sin;
{
	bzero((caddr_t)sin, sizeof(struct sockaddr) );
	sin->sin_family = AF_INET;
	if (subnet_net == (u_long)ifp->if_net) {
		sin->sin_addr.s_addr = subnet_mask;
	} else {
		sin->sin_addr.s_addr = 0;		
	}
}


/*
 * set the subnet mask for the given interface.
 * Must also hack the routing table entry for each interface on the given
 * subnetted network, since it may hash differently now.
 * Setting the mask to zero disables subnetting on the given interface.
 */
in_setnetmask(ifp, sin)
	register struct ifnet *ifp;
	struct sockaddr_in *sin;
{
	struct sockaddr_in subnet_sin;
	u_long save_net, save_mask;
	u_long new_net, new_mask;

	save_mask = subnet_mask;
	save_net = subnet_net;
	subnet_mask = sin->sin_addr.s_addr;
	subnet_net = (u_long)ifp->if_net;
	new_mask = subnet_mask;
	new_net = subnet_net;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	    if (ifp->if_net == new_net) {
		if ((ifp->if_flags & IFF_UP) == 0)
			continue;
		subnet_mask = save_mask;
		subnet_net = save_net;
		bzero((caddr_t)&subnet_sin, sizeof (subnet_sin));
		subnet_sin.sin_family = AF_INET;
		subnet_sin.sin_addr = if_makenet(ifp);
		rtinit((struct sockaddr *)&subnet_sin, 
			&ifp->if_addr, -1);
		  /*
		   * Above we temporarily set the mask and net to what
		   * it was previously, and removed the old route.
		   * Next we reset to the new values, and add a new route.
		   */
		subnet_mask = new_mask;
		if (subnet_mask == 0)
			subnet_net = 0;
		else
			subnet_net = new_net;
		subnet_sin.sin_addr =  if_makenet(ifp);
		rtinit((struct sockaddr *)&subnet_sin, 
			&ifp->if_addr, RTF_UP);
		ifp->if_broadaddr = *(struct sockaddr *)&subnet_sin;
	    }
	}
}

/*
 * Return 1 if the address is a broadcast address for any of the locally
 * connected networks.
 */
in_broadcast(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	u_long net, host;
	register struct ifnet *ifp;

	if (i == INADDR_ANY || i == INADDR_BROADCAST) {
		return(1);
	}

	if (IN_CLASSA(i)) {
		net = (i & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT;
		host = i & IN_CLASSA_HOST;
	} else if (IN_CLASSB(i)) {
		net = (i & IN_CLASSB_NET) >> IN_CLASSB_NSHIFT;
		host = i & IN_CLASSB_HOST;
	} else {
		net = (i & IN_CLASSC_NET) >> IN_CLASSC_NSHIFT;
		host = i & IN_CLASSC_HOST;
	}

	/*
	 * Check whether network is a subnet;
	 * if so, use the modified interpretation of `host'.
	 */
	 if (net == subnet_net) {
	 	host &= ~subnet_mask;
		return(host == INADDR_ANY || 
		       host == (INADDR_BROADCAST & ~subnet_mask));
	}
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_net == net) {
		    if (host == INADDR_ANY) return(1);
		    in = if_makeaddr((int)net, INADDR_BROADCAST);
		    return(i == in.s_addr);
		}
	return (0);
}

#endif
