/*	@(#)if_levar.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef	_IF_LEVAR_
#define	_IF_LEVAR_


/*
 * Definitions for data structures that the
 * AMD 7990 LANCE driver uses internally.
 *
 * This file exists primarily to allow network monitoring
 * programs easy access to the definition of le_softc.
 */


/*
 * Transmit and receive buffer layout.
 *	The chip sees only the fields from lb_ehdr onwards; the
 *	preceding fields are for the driver's benefit. The e_to_lb
 *	and b_to_lb macros defined below convert the address of the
 *	start of a lb_ehdr or lb_buffer field to the address of the
 *	start of the containing le_buf structure.
 *
 *	The buffer size is chosen to give room for the maximum ether
 *	transmission unit, an overrun consisting of the entire fifo
 *	contents, and slop that experience indicates is necessary.
 *	(The exact amount of slop required is still unknown.)
 */
struct le_buf {
	/* Fields used only by driver: */
	struct le_buf	*lb_next;		/* Link to next buffer */
	struct le_softc	*lb_es;			/* Link back to sw status */
	/* Fields seen by LANCE chip: */
	struct ether_header	lb_ehdr;	/* Packet's ether header */
	u_char		lb_buffer[MAXBUF];	/* Packet's data */
};

/*
 * Bookkeeping structure for packet(s) under transmission.  Each such
 * packet is represented by an initial mbuf chain (possibly null) and
 * a trailing buffer/length pair.  This arrangement corresponds to the
 * way we set tmds up for an outgoing packet.  All tmds (except possibly
 * the last) point to successive mbufs of the chain representing the packet.
 * The last points to a statically allocated buffer containing the rest
 * of the packet.  (This case arises when the driver is about to run out
 * of tmds.)
 */
struct tdesc {
	struct mbuf	*td_mb;		/* mbuf chain head */
	u_char		*td_tail;	/* buffer for rest of packet... */
	u_int		td_tlen;	/* ... and its length */
};


/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * "es" indicates Ethernet Software status.
 *
 * The buffer-related declarations allow for more than one packet to
 * be transmitted at a time, although the driver (and other declarations)
 * currently allows for only one at a time.
 */
struct	le_softc {
	struct	arpcom es_ac;		/* common ethernet structures */
#define	es_if		es_ac.ac_if	/* network-visible interface */
#define	es_enaddr	es_ac.ac_enaddr	/* hardware ethernet address */

	struct	le_init_block *es_ib;	/* Initialization block */

	/* LANCE message descriptor info */
	struct	le_md *es_rdrp;		/* Receive Descriptor Ring Ptr */
	struct	le_md *es_rdrend;	/* Receive Descriptor Ring End */
	int	es_nrmdp2;		/* log(2) Num. Rcv. Msg. Descs. */
	int	es_nrmds;		/* Num. Rcv. Msg. Descs. */
	struct	le_md *es_tdrp;		/* Transmit Descriptor Ring Ptr */
	struct	le_md *es_tdrend;	/* Receive Descriptor Ring End */
	int	es_ntmdp2;		/* log(2) Num. Tran. Msg. Descs. */
	int	es_ntmds;		/* Num. Xmit. Msg. Descs. */
	struct	le_md *es_his_rmd;	/* Next descriptor in ring */
	struct	le_md *es_cur_tmd;	/* Tmd for start of current packet */
	struct	le_md *es_nxt_tmd;	/* Tmd for start of next packet */

	/* Buffer info */
	struct	le_buf *es_rbufs;	/* Receive Buffers */
	int	es_nrbufs;		/* Number of Receive Buffers */
	struct	le_buf *es_tbufs;	/* Transmit Buffers */
	int	es_ntbufs;		/* Number of Transmit Buffers */
	struct	le_buf *es_rbuf_free;	/* Head of free list */
	struct	le_buf *es_rbuf_loaned;	/* Head of loan-out list */
#ifdef notdef				/* Unused field... */
	struct	le_buf *es_tbuf_free;	/* Head of free list */
#endif notdef

	struct	tdesc es_tpack;		/* Packet being transmitted */

	u_int	es_flags;		/* State info: see below */

	/* Error counters */
	int	es_extrabyte;		/* Rev C,D LANCE extra byte problem */
	int	es_fram;		/* Receive Framing Errors (dribble) */
	int	es_crc;			/* Receive CRC Errors */
	int	es_oflo;		/* Receive overruns */
	int	es_uflo;		/* Transmit underruns */
	int	es_retries;		/* Transmit retries */
	int	es_missed;		/* Number of missed packets */
	int	es_noheartbeat;		/* Number of nonexistent heartbeats */
	int	es_tBUFF;		/* BUFF bit in tmd occurrences */
	int	es_tlcol;		/* Transmit late collisions */
	int	es_trtry;		/* Transmit retry errors */
	int	es_tnocar;		/* No carrier errors */

	/* Performance statistics counters */
	int	es_started;		/* Times through lestart with > 0
					   packets ready to go out */
	int	es_started2;		/* Times through lestart with > 1
					   packet ready to go out */
	int	es_potential_rloans;	/* Number of opportunities to loan
					   out a receive buffer */
	int	es_actual_rloans;	/* Cumulative number of receive buffers
					   loaned to protocol layers */
	int	es_no_tmds;		/* Number of resource exhaustion
					   instances on output */
	int	es_requeues;		/* Number of output packets requeued */
};

/*
 * Bit definitions for es_flags field:
 */
#define LE_TBUSY	0x01	/* Packet transmission in progress */
#define LE_TOPENDING	0x02	/* Transmit timeout pending */

#endif	_IF_LEVAR_
