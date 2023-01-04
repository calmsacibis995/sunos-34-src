/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)input.c 1.5 87/01/08 SMI"; /* from UCB 5.6 6/3/86 */
#endif

/*
 * Routing Table Management Daemon
 */
#include "defs.h"

/*
 * Process a newly received packet.
 */
rip_input(from, size)
	struct sockaddr *from;
	int size;
{
	register struct rt_entry *rt;
	struct netinfo *n;
	struct interface *ifp;
	int newsize;
	register struct afswitch *afp;
	int answer = supplier;
	struct entryinfo	*e;

	ifp = 0;
	TRACE_INPUT(ifp, from, size);
	if (from->sa_family >= af_max ||
	    (afp = &afswitch[from->sa_family])->af_hash == (int (*)())0) {
		return;
	}
	switch (msg->rip_cmd) {

	case RIPCMD_POLL:		/* request specifically for us */
		answer = 1;
		/* fall through */
	case RIPCMD_REQUEST:		/* broadcasted request */
		newsize = 0;
		size -= 4 * sizeof (char);
		n = msg->rip_nets;
		while (size > 0) {
			if (size < sizeof (struct netinfo))
				break;
			size -= sizeof (struct netinfo);

			if (msg->rip_vers > 0) {
				n->rip_dst.sa_family =
					ntohs(n->rip_dst.sa_family);
				n->rip_metric = ntohl(n->rip_metric);
			}
			/* 
			 * A single entry with sa_family == AF_UNSPEC and
			 * metric ``infinity'' means ``all routes''.
			 */
			if (n->rip_dst.sa_family == AF_UNSPEC &&
			    n->rip_metric == HOPCNT_INFINITY && size == 0) {
				if (answer) 
					supply(from, 0, ifp);
				return;
			}
			rt = rtlookup(&n->rip_dst);
			n->rip_metric = rt == 0 ? HOPCNT_INFINITY :
				min(rt->rt_metric+1, HOPCNT_INFINITY);
			if (msg->rip_vers > 0) {
				n->rip_dst.sa_family =
					htons(n->rip_dst.sa_family);
				n->rip_metric = htonl(n->rip_metric);
			}
			n++, newsize += sizeof (struct netinfo);
		}
		if (answer && newsize > 0) {
			msg->rip_cmd = RIPCMD_RESPONSE;
			newsize += sizeof (int);
			(*afp->af_output)(s, 0, from, newsize);
		}
		return;

	case RIPCMD_TRACEON:
	case RIPCMD_TRACEOFF:
		/* verify message came from a privileged port */
		if ((*afp->af_portcheck)(from) == 0)
			return;
		packet[size] = '\0';
		if (msg->rip_cmd == RIPCMD_TRACEON)
			traceon(msg->rip_tracefile);
		else
			traceoff();
		return;

	case RIPCMD_RESPONSE:
		/* verify message came from a router */
		if ((*afp->af_portmatch)(from) == 0)
			return;
		(*afp->af_canon)(from);
		/* are we talking to ourselves? */
		ifp = if_ifwithaddr(from);
		if (ifp) {
			rt = rtfind(from);
			if (rt == 0)
				addrouteforif(ifp);
			else
				rt->rt_timer = 0;
			return;
		}
		size -= 4 * sizeof (char);
		n = msg->rip_nets;
		for (; size > 0; size -= sizeof (struct netinfo), n++) {
			if (size < sizeof (struct netinfo))
				break;
			if (n->rip_dst.sa_family >= af_max ||
			    (afp = &afswitch[n->rip_dst.sa_family])->af_hash ==
			    (int (*)())0) {
				if (ftrace)
printf(ftrace,"route in unsupported address family (%d), from %s (af %d)\n",
				   n->rip_dst.sa_family,
				   (*afswitch[from->sa_family].af_format)(from),
				   from->sa_family);
				continue;
			}
			if (((*afp->af_checkhost)(&n->rip_dst)) == 0) {
				if (ftrace)
		printf(ftrace,"bad host in route from %s (af %d)\n",
				   (*afswitch[from->sa_family].af_format)(from),
				   from->sa_family);
				continue;
			}
			if (n->rip_metric < 0 ||
			    n->rip_metric > HOPCNT_INFINITY)
				continue;
			rt = rtlookup(&n->rip_dst);
			if (rt && (rt->rt_state & RTS_INTERNAL) != 0) {
			  if (from->sa_family == AF_INET &&
			      n->rip_dst.sa_family == AF_INET &&
			      inet_netonly(from, &n->rip_dst))
			   /*
			    * replace an internal route (subnet zero)
			    * with a real route if it ever shows up from
			    * inside our network only.
			    */
				rt = 0;
			}
			if (rt == 0) {
				if (n->rip_metric < HOPCNT_INFINITY)
				    rtadd(&n->rip_dst, from, n->rip_metric, 0);
				continue;
			}

			/*
			 * Update if from gateway and different,
			 * shorter, or getting stale and equivalent.
			 */
			if (equal(from, &rt->rt_router)) {
				if (n->rip_metric == HOPCNT_INFINITY) {
					rtdelete(rt);
					continue;
				}
				if (n->rip_metric != rt->rt_metric)
					rtchange(rt, from, n->rip_metric);
				rt->rt_timer = 0;
			} else if ((unsigned) (n->rip_metric) < rt->rt_metric ||
			    (rt->rt_timer > (EXPIRE_TIME/2) &&
			    rt->rt_metric == n->rip_metric)) {
				rtchange(rt, from, n->rip_metric);
				if (n->rip_metric < HOPCNT_INFINITY)
					rt->rt_timer = 0;
			}
		}
		return;
	case RIPCMD_POLLENTRY:
		rt = rtlookup(&msg->rip_nets->rip_dst);
		n = msg->rip_nets;
		newsize = sizeof (struct entryinfo);
		if (rt) {	/* don't bother to check rip_vers */
			e = (struct entryinfo *) n;
			e->rtu_dst = rt->rt_dst;
			e->rtu_dst.sa_family =
				ntohs(e->rtu_dst.sa_family);
			e->rtu_router = rt->rt_router;
			e->rtu_router.sa_family =
				ntohs(e->rtu_router.sa_family);
			e->rtu_flags = ntohs(rt->rt_flags);
			e->rtu_state = ntohs(rt->rt_state);
			e->rtu_timer = ntohl(rt->rt_timer);
			e->rtu_metric = ntohl(rt->rt_metric);
			e->int_flags = ntohl(rt->rt_ifp->int_flags);
			strncpy(e->int_name, rt->rt_ifp->int_name,
			    sizeof(e->int_name));
		}
		else
			bzero(n, newsize);
		(*afp->af_output)(s, 0, from, newsize);
		return;
	}
}


