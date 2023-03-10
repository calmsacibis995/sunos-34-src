#ifndef lint
static	char sccsid[] = "@(#)ypserv_peer.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * This contains yp server modules which operate on the peer data bases.
 */

#include "ypsym.h"
static struct in_addr null_addr;

/*
 * Funny external reference to inet_addr to make the reference agree with the
 * code, not the documentation.  Should be:
 * 	extern struct in_addr inet_addr();
 * according to the documentation, but that's not what the code does.
 */
extern u_long inet_addr();
char ypservers[] = YPSERVERS;
static struct timeval gettimeout = {	/* udp total timeout for peer comm */
	0,			/* Seconds */
	0				/* uSecs */
};

/*
 * This sets up the peer lists for all supported domains.
 */
void
ypbuild_peer_lists()
{
	struct domain_list_item *pdom;

	/* Do for each supported domain */

	for (pdom = yppoint_at_first_domain(TRUE);
	    pdom != (struct domain_list_item *) NULL;
	    pdom = yppoint_at_next_domain(pdom, TRUE) ) {
		ypbld_dom_peerlist(pdom);
	}
}

/*
 * Builds the peer list for a single domain, and calls ypget_all_peer_addrs to
 * load the internet addresses.
 */
void
ypbld_dom_peerlist(pdom)
	struct domain_list_item *pdom;
{
	bool status;
	unsigned long error;
	struct in_addr null_addr;
	datum key;
	char *peer_name;
	struct peer_list_item *ppeer;

	if (!pdom) {
		return;
	}
	
	if (!ypset_current_map(ypservers, pdom->dom_name, &error) ) {
		fprintf(stderr,
"ypserv:  ypbld_dom_peerlist can't set peer server map for domain %s.\n",
		    pdom->dom_name);
		return;
	}

	/* Do for each known server */

	for (key = firstkey(); key.dptr != NULL; key = nextkey(key) ) {

		/*
		 * Knock out key-value pairs from the
		 * map file which are yp private symbols.
		 */

		if (key.dsize >= YPSYMBOL_PREFIX_LENGTH && (!bcmp(key.dptr,
		    YPSYMBOL_PREFIX, YPSYMBOL_PREFIX_LENGTH) ) ) {
			continue;
		}
					
		if ( (peer_name = (char *) malloc(key.dsize + 1) ) ==
		    (char *) NULL) {
			fprintf(stderr,
		      "ypserv: ypbld_dom_peerlist: malloc failed - peer %.*s.\n",
			    key.dsize, key.dptr);
			continue;
		}

		bcopy(key.dptr, peer_name, key.dsize);
		peer_name[key.dsize] = '\0';

		status = ypadd_one_peer(peer_name, pdom, &null_addr);

		if (!status) {
			fprintf(stderr,
		"ypserv: ypbuild_peer_lists: can't add peer %s, domain %s.\n",
			     peer_name, pdom->dom_name);
			free(peer_name);
			continue;
		}
	}

	ypget_all_peer_addrs(pdom);		/* Try to set up the internet
					 	*   addresses for the lot */
}


/*
 * This gets an internet address for a peer server by fetching the datum for
 * the passed hostname in hosts.byname for the passed domain, and parsing the
 * address out by calling the standard routine inet_addr.  Returns peer server's
 * inet address in peer_addr.
 */
bool
ypget_peer_addr(name, domain, peer_addr)
	char *name;
	char *domain;
	struct in_addr *peer_addr;
{
	int len;
	datum key;
	datum val;
	struct in_addr tempaddr;
	int error;
	char tmpbuf[256];
	char *tmpptr;

	if (!name || (strlen(name) == 0) || !domain ||
	    ((len = strlen(domain)) == 0) || (len > YPMAXDOMAIN) ) {
		return(FALSE);
	}
	
	if (!ypset_current_map(YPHOSTS_BYNAME, domain, &error) ) {
		return(FALSE);
	}

	key.dsize = strlen(name);
	key.dptr = name;
	val = fetch(key);

	if (val.dptr == (char *) NULL) {	/* No entry for peer */
		return(FALSE);
	}

	/*
	 * We are going to recopy the record out of dbm's static memory, because
	 * we don't know if it's null terminated before the end of the memory
	 * segment, or if there are garbage numeric chars in there which may
	 * louse up our value.
	 */

	tmpptr = tmpbuf;

	if (val.dsize > 255) {

		if ( (tmpptr = (char *) malloc(val.dsize +1) ) == NULL) {
		   fprintf(stderr, "ypserv: ypget_peer_addr: malloc failure.\n");
			return(FALSE);
		}
	}

	bcopy(val.dptr, tmpptr, val.dsize);
	tmpptr[val.dsize] = '\0';
	
	/*
	 * Do the conversion from a sting in /etc/hosts format to an internet
	 * address in network order.
	 */
	 
	tempaddr.s_addr = inet_addr(tmpptr);
	*peer_addr = tempaddr;

	if (tmpptr != tmpbuf) {
		free(tmpptr);
	}
	
	if ( (int) tempaddr.s_addr !=  -1) {
		return(TRUE);
	} else {
		fprintf(stderr,
	      "ypserv: ypget_peer_addr: garbage entry for peer %s, domain %s.\n",
		    name, domain); 
		return(FALSE);
	}
}

/*
 *
 * This adds a peer_list_item to the peer list of a passed domain.  Parameter
 * name is the name of the peer.  This should be in an area of memory which has
 * been malloc'ed by the caller.
 *
 * Note:  If a peer is already on the peerlist, this function will return TRUE,
 * and the field peer_in_new_map will be set TRUE.  In the failure case the
 * memory malloced by the caller to hold the peer name will not be freed.  In
 * the success case, either that memory will be associated with a new peer
 * list entry, or, if a peer list entry with the same peer name exists,
 * the name memory will be freed.
 */
bool
ypadd_one_peer(name, pdom, peer_addr)
	char *name;
	struct domain_list_item *pdom;
	struct in_addr *peer_addr;
{
	struct peer_list_item *pnewpeer;

	if (!name || (strlen(name) == 0) || !pdom) {
		return(FALSE);
	}
	
	if (pnewpeer = yppoint_at_peer(name, pdom) ) {
		pnewpeer->peer_in_new_map = TRUE;
		free(name);
		return(TRUE);
	}
	
	pnewpeer = (struct peer_list_item *)
	    malloc(sizeof(struct peer_list_item));

	if (pnewpeer == (struct peer_list_item *) NULL) {
		fprintf(stderr, "ypserv: ypadd_one_peer: malloc failure.\n");
		return(FALSE);
	}
	
	pnewpeer->peer_pnext = pdom->dom_ppeerlist;
	pdom->dom_ppeerlist = pnewpeer;
	pnewpeer->peer_pname = name;
	pnewpeer->peer_addr = *peer_addr;
	pnewpeer->peer_port = htons( (unsigned short) YPNOPORT);
	pnewpeer->peer_reachable = TRUE;	/* Assume everyone's reachable */
	pnewpeer->peer_in_new_map = TRUE;
	pnewpeer->peer_last_polled = 0;
	return(TRUE);

}

/*
 * This removes a peer_list_item from a domain.  Memory associated with the
 * peer_list_item and the host name are returned to the system.  The state of
 * the peer list may be changed.  Also, any map in the domain which refers to
 * the peer will have that reference changed to NULL.
 */
void
ypdel_one_peer(ppeer, pdom)
	struct peer_list_item *ppeer;
	struct domain_list_item *pdom;
{
	struct peer_list_item *scan;
	struct map_list_item *pmap;
	bool cleanup = FALSE;

	if (!ppeer || !pdom) {
		return;
	}

	if ( (scan = yppoint_at_peerlist(pdom) ) !=
	    (struct peer_list_item *) NULL) {

		if (scan == ppeer) {
			pdom->dom_ppeerlist = ppeer->peer_pnext;
			cleanup = TRUE;
		} else {
		
			for (;
		    	    ( (scan != (struct peer_list_item *) NULL) &&
		                (scan != ppeer) );
		    	    scan = scan->peer_pnext) {
			}

			if (scan != (struct peer_list_item *) NULL) {
				scan->peer_pnext = ppeer->peer_pnext;
				cleanup = TRUE;
			} else {
				fprintf(stderr,
			   "ypserv: ypdel_one_peer can't find peer on list.\n");
			}
		}

		if (cleanup) {

			if (ppeer->peer_pname) {
				free(ppeer->peer_pname);
			}

			for (pmap = yppoint_at_maplist(pdom);
			    pmap != (struct map_list_item *) NULL;
			    pmap = yppoint_at_next_map(pmap) ) {

				if (pmap->map_master == ppeer) {
					pmap->map_master =
					    (struct peer_list_item *) NULL;
				}
				
				if (pmap->map_alternate == ppeer) {
					pmap->map_alternate =
					    (struct peer_list_item *) NULL;
				}
			}

			free(ppeer);
		}
		
	} else {
		fprintf(stderr,
	      "ypserv: ypdel_one_peer called for domain with null peerlist.\n");
	}
}

/*
 * This returns a pointer to a named peer_list_item within a named domain.
 */
struct peer_list_item *
yppoint_at_peer(peer, pdom)
	char *peer;
	struct domain_list_item *pdom;
{
	struct peer_list_item *scan;

	if ( (peer == (char *) NULL) || (strlen(peer) == 0) || !pdom) {
		return( (struct peer_list_item *) NULL);
	}

	scan = yppoint_at_peerlist(pdom);

	while (( scan != (struct peer_list_item *) NULL) &&
	    (strcmp(scan->peer_pname, peer) ) ) {
		scan = scan->peer_pnext;
	}

	return(scan);
}

/*
 * This returns a pointer to the next peer on a list of peer servers, or NULL.
 */
struct peer_list_item *
ypnext_peer(ppeer)
	struct peer_list_item *ppeer;
{
	if (ppeer) {
		return(ppeer->peer_pnext);
	} else {
		return( (struct peer_list_item *) NULL);
	}
}

/*
 * This returns a peer server's internet address given a pointer to the peer
 * list item.
 */
struct in_addr
ypreturn_peer_addr(ppeer)
	struct peer_list_item *ppeer;
{
	
	if (ppeer) {
		return(ppeer->peer_addr);
	} else {
		null_addr.s_addr = 0;
		return(null_addr);
	}
}

/*
 * This loads all the peer addresses for a domain.  The peer_addr fields of
 * each peer on the list may be loaded with its internet address.
 *
 * Note:  Any old garbage which was in the peer_addr fields will remain there
 * in a failure case.
 */
void
ypget_all_peer_addrs(pdom)
	struct domain_list_item *pdom;
{
	struct peer_list_item *ppeer;

	if (!pdom) {
		return;
	}

	for (ppeer = yppoint_at_peerlist(pdom);
	    ppeer != (struct peer_list_item *) NULL;
	    ppeer = ypnext_peer(ppeer) ) {
		(void) ypget_peer_addr(ppeer->peer_pname,
		    pdom->dom_name, &(ppeer->peer_addr) );
	    }
}

/*
 * This sets all the peer addresses for a domain to -1, an out-of-range value.
 * (It's the failure value returned by intet_addr...)
 */
void
ypreset_all_peer_addrs(pdom)
	struct domain_list_item *pdom;
{
	struct peer_list_item *ppeer;

	if (!pdom) {
		return;
	}

	for (ppeer = yppoint_at_peerlist(pdom);
	    ppeer != (struct peer_list_item *) NULL;
	    ppeer = ypnext_peer(ppeer) ) {
		ppeer->peer_addr.s_addr = (u_long) -1;
	    }
}

/*
 * This sends a "get" request to a named peer.
 */
void
ypsend_getreq(ppeer, pmap)
	struct peer_list_item *ppeer;
	struct map_list_item *pmap;
{
	struct yprequest req;
	enum clnt_stat clnt_stat;
	int reason;

	if (!ppeer || !pmap) {
		return;
	}

	if (ypset_xfr_peer(ppeer) ) {
	
		/* Do the get request */
		
		req.yp_reqtype = YPGET_REQTYPE;
		req.ypget_req_domain = pmap->map_domain->dom_name;
		req.ypget_req_map = pmap->map_name;
		req.ypget_req_ordernum = pmap->map_order;
		req.ypget_req_owner = pmap->map_master->peer_pname;
		
		clnt_stat = (enum clnt_stat) clnt_call(xfr_binding.dom_client,
	    	    YPPROC_GET, xdr_yprequest, &req, xdr_void, 0, gettimeout);
	}
}
