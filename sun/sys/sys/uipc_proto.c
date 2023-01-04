/*	@(#)uipc_proto.c 1.1 86/09/25 SMI; from UCB 4.28 83/05/27	*/

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"

/*
 * Definitions of protocols supported in the UNIX domain.
 */

int	uipc_usrreq();
int	raw_init(),raw_usrreq(),raw_input(),raw_ctlinput();

struct protosw unixsw[] = {
{ SOCK_STREAM,	PF_UNIX,	0,		PR_CONNREQUIRED|PR_WANTRCVD,
  0,		0,		0,		0,
  uipc_usrreq,
  0,		0,		0,		0,
},
{ SOCK_DGRAM,	PF_UNIX,	0,		PR_ATOMIC|PR_ADDR|PR_RIGHTS,
  0,		0,		0,		0,
  uipc_usrreq,
  0,		0,		0,		0,
},
{ 0,		0,		0,		0,
  raw_input,	0,		raw_ctlinput,	0,
  raw_usrreq,
  raw_init,	0,		0,		0,
}
};

struct domain unixdomain =
    { AF_UNIX, "unix", unixsw, &unixsw[sizeof(unixsw)/sizeof(unixsw[0])] };
