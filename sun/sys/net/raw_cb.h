/*	@(#)raw_cb.h 1.1 86/09/25 SMI; from UCB 4.5 83/06/30	*/

/*
 * Raw protocol interface control block.  Used
 * to tie a socket to the generic raw interface.
 */
struct rawcb {
	struct	rawcb *rcb_next;	/* doubly linked list */
	struct	rawcb *rcb_prev;
	struct	socket *rcb_socket;	/* back pointer to socket */
	struct	sockaddr rcb_faddr;	/* destination address */
	struct	sockaddr rcb_laddr;	/* socket's address */
	caddr_t	rcb_pcb;		/* protocol specific stuff */
	struct	route rcb_route;	/* routing information */
	int	rcb_cc;			/* bytes of rawintr queued data */
	int	rcb_mbcnt;		/* bytes of rawintr queued mbufs */
	short	rcb_flags;
};

/*
 * Since we can't interpret canonical addresses,
 * we mark an address present in the flags field.
 */
#define	RAW_LADDR	0x01
#define	RAW_FADDR	0x02
#define	RAW_DONTROUTE	0x04		/* no routing, XXX: Q/default */
#define RAW_TALLY	0x08		/* tally delivered packets */

#define	sotorawcb(so)		((struct rawcb *)(so)->so_pcb)

/*
 * Nominal space allocated to a raw socket.
 */
#define	RAWSNDQ		2048
#define	RAWRCVQ		2048

/*
 * Format of raw interface header prepended by
 * raw_input after call from protocol specific
 * input routine.
 */
struct raw_header {
	struct	sockproto raw_proto;	/* format of packet */
	struct	sockaddr raw_dst;	/* dst address for rawintr */
	struct	sockaddr raw_src;	/* src address for sbappendaddr */
};

#ifdef KERNEL
struct rawcb rawcb;			/* head of list */
#endif
