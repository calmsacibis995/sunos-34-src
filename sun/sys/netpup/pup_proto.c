/*	@(#)pup_proto.c 1.1 86/09/25 SMI; from UCB 5.3 83/06/30	*/

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"

/*
 * PUP-I protocol family: raw interface
 */
int	rpup_output();
extern	int raw_usrreq();

struct protosw pupsw[] = {
{ SOCK_RAW,	PF_PUP,		0,		PR_ATOMIC|PR_ADDR,
  0,		rpup_output,	0,		0,
  raw_usrreq,
  0,		0,		0,		0,
},
};

struct domain pupdomain =
    { AF_PUP, "pup", pupsw, &pupsw[sizeof(pupsw)/sizeof(pupsw[0])] };

#ifdef notdef
/*
 * 3 Mb/s Ethernet link protocol family: raw interface
 */
int	raw_enoutput();
extern	int raw_usrreq();

struct protosw ensw[] = {
{ SOCK_RAW,	PF_ETHERLINK,	0,		PR_ATOMIC|PR_ADDR,
  0,		raw_enoutput,	0,		0,
  raw_usrreq,
  0,		0,		0,		0,
},
};

struct domain endomain =
    { AF_ETHERLINK "ether", ensw, &ensw[sizeof(ensw)/sizeof(ensw[0])] };
#endif
