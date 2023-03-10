#ifndef lint
static	char sccsid[] = "@(#)ypserv_timer.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * ypserv_timer.c
 * contains functions which are called from the
 * yellow pages server timer signal handler, yptimer.
 */

#include "ypsym.h"
/*
 * This selects one peer server to be pinged, then
 * calls ypping_peer to do the work of seeing whether he is reachable
 * and alive.  It rotates to a new domain, and picks out the least
 * recently polled peer which hasn't been polled in at least a defined
 * delta.  If no peer is eligible in the new domain, it continues to
 * rotate domains until it either finds an eligible peer, or until it
 * gets back to its starting state, at which point it gives up.  Sort of
 * breadth first.  Was that clear?
 */
void
ypping_eligible_peer() 
{
	struct domain_list_item *starting_domain;
	struct domain_list_item *domain_scan;
	struct peer_list_item *ppeer = (struct peer_list_item *) NULL;
	struct peer_list_item *peer_scan;
	unsigned long int min;
	struct peer_list_item *min_peer;

	if (!ypinitialization_done) {
		return;
	}
		
	starting_domain = (struct domain_list_item *) NULL;
	
	if (pingpeer_curr_domain) {
		starting_domain =
		    yppoint_at_next_domain(pingpeer_curr_domain, TRUE);
	}
		
	/*
	 * This next test picks up the case where the incoming state of
	 * pingpeer_curr_domain was NULL, and that in which starting_domain
	 * has just been set to NULL by the call to yppoint_at_next_domain.
	 */

	if (!starting_domain) {
		    starting_domain = yppoint_at_first_domain(TRUE);
	}

	if (!starting_domain) {		/* No domains!? */
		return;
	}

	domain_scan = starting_domain;

	while (!ppeer) {

		for (peer_scan = yppoint_at_peerlist(domain_scan),
		      min = tick_counter;
		    peer_scan != (struct peer_list_item *) NULL;
		    peer_scan = ypnext_peer(peer_scan) ) {

			if (peer_scan->peer_last_polled < min) {
				min = peer_scan->peer_last_polled;
				min_peer = peer_scan;
			}
		}

		if ( (tick_counter - min) > YPPEER_PINGDELTA) {
			ppeer = min_peer;
			pingpeer_curr_domain = domain_scan;
		} else {
			domain_scan =
			    yppoint_at_next_domain(domain_scan, TRUE);

			if (!domain_scan) {
				domain_scan = yppoint_at_first_domain(TRUE);
			}

			if (domain_scan == starting_domain) {
				break;
			}
		}
	}
	
	if (ppeer) {
		ypping_peer(ppeer);
	}
}

/*
 * This selects one map to be pinged, then calls
 * ypping_map to do the work of checking the order number, and adding
 * it to the map transfer list if our copy is out of date.  This is
 * EXACTLY the same as ypping_eligible_peer, with the names changed to
 * protect the data structures.
 */
void
ypping_eligible_map()
{
	struct domain_list_item *starting_domain;
	struct domain_list_item *domain_scan;
	struct map_list_item *pmap = (struct map_list_item *) NULL;
	struct map_list_item *map_scan;
	unsigned long int min;
	struct map_list_item *min_map;

	if (!ypinitialization_done)
		return;
		
	starting_domain = (struct domain_list_item *) NULL;
	
	if (pingmap_curr_domain) {
		starting_domain =
		    yppoint_at_next_domain(pingmap_curr_domain, TRUE);
	}
		
	/*
	 * This next test picks up the case where the incoming state of
	 * pingmap_curr_domain was NULL, and that in which starting_domain
	 * has just been set to NULL by the call to yppoint_at_next_domain.
	 */

	if (!starting_domain) {
		    starting_domain = yppoint_at_first_domain(TRUE);
	}

	if (!starting_domain) {		/* No domains!? */
		return;
	}

	domain_scan = starting_domain;

	while (!pmap) {

		for (map_scan = yppoint_at_maplist(domain_scan),
		      min = tick_counter;
		    map_scan != (struct map_list_item *) NULL;
		    map_scan = yppoint_at_next_map(map_scan) ) {

			if (map_scan->map_last_polled < min) {
				min = map_scan->map_last_polled;
				min_map = map_scan;
			}
		}

		if ( (tick_counter - min) > YPMAP_PINGDELTA) {
			pmap = min_map;
			pingmap_curr_domain = domain_scan;
		} else {
			domain_scan =
			    yppoint_at_next_domain(domain_scan, TRUE);

			if (!domain_scan) {
				domain_scan = yppoint_at_first_domain(TRUE);
			}

			if (domain_scan == starting_domain) {
				break;
			}
		}
	}
	
	if (pmap) {
		ypping_map(pmap);
	}
}

