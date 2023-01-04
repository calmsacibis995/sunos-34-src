/*	@(#)pup.c 1.1 86/09/25 SMI; from UCB 4.5 83/05/30	*/

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/af.h"
#include "../netpup/pup.h"

#ifdef PUP
pup_hash(spup, hp)
	struct sockaddr_pup *spup;
	struct afhash *hp;
{

	hp->afh_nethash = spup->spup_net;
	hp->afh_hosthash = spup->spup_host;
}

pup_netmatch(spup1, spup2)
	struct sockaddr_pup *spup1, *spup2;
{

	return (spup1->spup_net == spup2->spup_net);
}
#endif
