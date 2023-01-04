#ifndef lint
static  char sccsid[] = "@(#)if_le.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* Conditional compilation symbols for debugging: */
/* #define DEBUG	*/
/* #define LEDEBUG	*/

/*
 * Guide to FIXMEs:
 *
 * FIXME MULTICAST means that this section of code may have to be changed
 *   if the driver has to handle Multicasting.
 * Other FIXMEs are usually deficiencies in the code.
 */

#include "le.h"

#if NLE > 0

/*
 * AMD Am7990 LANCE Ethernet Controller driver
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/map.h"
#include "../h/socket.h"
#include "../h/errno.h"

#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"

#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../sundev/mbvar.h"

#include "../sunif/if_lereg.h"
#include "../sunif/if_levar.h"


int	leprobe(), leattach(), leintr();
struct	mb_device *leinfo[NLE];
struct	mb_driver ledriver = {
	leprobe, 0, leattach, 0, 0, leintr,
	sizeof (struct le_device), "le", leinfo, 0, 0, 0,
};

int	leinit(),leioctl(),leoutput(),lereset();

struct mbuf *copy_to_mbufs();
struct mbuf *mclgetx();


/*
 * Patchable variable -- set this to 1 to enable retransmission
 * after the chip detects a failed packet transmission attempt.
 */
int	le_retransmit = 1;


/*
 * Ethernet software status per interface.
 */
struct le_softc	le_softc[NLE];


/*
 * Resource amounts.
 *	For now, at least, these never change, and resources
 *	are allocated once and for all.  However, we potentially
 *	could recast them into advisory variables (changing to
 *	one instance per interface) with the meaning that, the
 *	next time resources are allocated to the interface, grab
 *	these amounts.
 *
 *	Reducing the number of tmds below 16 causes them to run
 *	out occasionally.  (This is somewhat surprising.)  Since
 *	the incremental cost of going from 8 to 16 is only 64
 *	bytes, we stick with the higher number.
 *
 *	Reducing the number of rmds below 16 leads to occasional
 *	missed packets.  By the same reasoning, we stick with
 *	the higher value.  [Even 16 doesn't seem to be enough on
 *	a loaded net, so we'll try 32 for a while.]
 */
/* Numbers of ring descriptors */
int le_ntmdp2 = 4;
int le_nrmdp2 = 5;

/* Numbers of buffers */
int le_ntbufs = 1;
int le_nrbufs = 40;	/* Ouch! one per rmd plus eight for loan-outs */


/*
 * Given a lb_ehdr or lb_buffer address, produce the address of its
 * containing le_buf structure.
 *
 * N.B.: These definitions assume that the compiler inserts no padding
 * between the fields of the le_buf structure.  (If it did, we'd be
 * in trouble on other grounds as well.)
 */
#define e_to_lb(e) \
	(struct le_buf *)((caddr_t)(e) - sizeof(struct le_softc *) \
			- sizeof(struct le_buf *))
#define b_to_lb(b) \
	(struct le_buf *)((caddr_t)(b) - sizeof(struct ether_header) \
			- sizeof(struct le_softc *) - sizeof(struct le_buf *))

/*
 * Allocate an array of "number" structures
 * of type "structure" in kernel memory.
 */
#define getstruct(structure, number)   \
	((structure *) kmem_alloc( (u_int) (sizeof(structure) * (number)) ))

/*
 * Convert a stored Lance address to a CPU virtual address.
 *	FIXME: make this reasonable.
 */
#define DVMA_KLUDGE 0x0F000000
#define le_buf_addr(md)  (DVMA_KLUDGE + \
			    (u_char *)((md->lmd_hadr << 16) + md->lmd_ladr))

/*
 * Return the address of an adjacent descriptor in the given ring.
 */
#define next_rmd(es,rmdp)	((rmdp) == (es)->es_rdrend		\
					? (es)->es_rdrp : ((rmdp) + 1))
#define next_tmd(es,tmdp)	((tmdp) == (es)->es_tdrend		\
					? (es)->es_tdrp : ((tmdp) + 1))
#define prev_tmd(es,tmdp)	((tmdp) == (es)->es_tdrp		\
					? (es)->es_tdrend : ((tmdp) - 1))

/*
 * Buffer manipulation macros (see below for routines).
 *
 * A receive buffer can be in one of three states:
 *	free:		inactive and available for use
 *	attached:	awaiting packet reception and visible to
 *			the LANCE chip as a packet receptacle
 *	loaned:		contains packet data that is being processed
 *			by the higher-level protocol code
 * Free buffers are chained together through their lb_next fields into
 * a free list rooted at the le_softc.es_rbuf_free field.  Attached
 * buffers can be found only by working back from the receive ring
 * descriptors.  Loaned buffers are chained onto the list rooted at
 * le_softc.es_rbuf_loaned.
 *
 * List manipulation consitutes a critical region.  Callers of these
 * routines must lock things appropriately.
 */

/*
 * Link the buffer *bp into the list rooted at lp.
 */
#define le_linkin(lp, bp) \
	(bp)->lb_next = (lp), \
	(lp) = (bp)

/*
 * Take a receive buffer and put it on the free or loaned list.
 */
#define free_rbuf(es, rb)	le_linkin(es->es_rbuf_free, rb)
#define loan_rbuf(es, rb)	le_linkin(es->es_rbuf_loaned, rb)

/*
 * Return nonzero iff *bp is on the list rooted at *lpp.
 *	Assumes lpp and bp are both nonnull.
 */
#define le_onlist(lpp, bp)	le_lpredp(lpp, bp)

struct le_buf	*get_rbuf();
struct le_buf	**le_lpredp();

/*
 * Probe for device.
 */
leprobe(reg)
	caddr_t reg;
{
	register struct le_device *le = (struct le_device *)reg;

	if (pokec((char *)&le->le_rdp, 0))	/* FIXME - need better test */
		return (0);
	return (sizeof (struct le_device));
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
leattach(md)
	struct mb_device *md;
{
	struct le_softc *es = &le_softc[md->md_unit];

	/* Reset the chip. */
	((struct le_device *)md->md_addr)->le_rap = LE_CSR0;
	((struct le_device *)md->md_addr)->le_csr = LE_STOP;

	(void) localetheraddr((struct ether_addr *)NULL, &es->es_enaddr);

#ifdef notdef
	/*
	 * We would like to allocate buffer and descriptor memory
	 * now, but we can't do it now, since kmem_alloc hasn't been
	 * initialized yet.  Thus we defer it to init time.
	 */
	le_alloc_buffers(es);
#endif notdef

	/* Do hardware-independent attach stuff. */
	ether_attach(&es->es_if, md->md_unit, "le",
		     leinit, leioctl, leoutput, lereset);
}

/*
 * Grab memory for message descriptors and buffers and set fields
 * in the interfaces's software status structure accordingly.  The
 * memory obtained is _not_ intialized; this is done later.
 * Called from leinit, since attach time is too early.
 *
 * It must be possible to call this routine repeatedly with
 * no ill effects.
 */
le_alloc_buffers(es)
	register struct le_softc *es;
{
	register caddr_t	a;
	extern caddr_t		kmem_alloc();

	/*
	 * If resources are already allocated, don't mess with them.
	 *	We could consider dynamic resource reallocation here,
	 *	but it would require adding code to wait for active
	 *	resources to become free.
	 */
	if (es->es_ib == (struct le_init_block *)0) {
		/* Initialization block */
		es->es_ib = getstruct(struct le_init_block, 1);

		/* Set numbers of message descriptors. */
		es->es_nrmdp2 = le_nrmdp2;
		es->es_ntmdp2 = le_ntmdp2;
		es->es_nrmds = 1 << es->es_nrmdp2;
		es->es_ntmds = 1 << es->es_ntmdp2;

		/* Set numbers of buffers. */
		es->es_nrbufs = le_nrbufs;
		es->es_ntbufs = le_ntbufs;

		/*
		 * Allocate the message descriptor rings.
		 *	Force 8-byte alignment by allocating an extra
		 *	one and then rounding the starting address.
		 *	This code depends on message descriptors being
		 *	>= 8 bytes long.  (Bletch!)
		 */
		a = kmem_alloc((u_int)
			(sizeof (struct le_md) * (es->es_nrmds + 1)));
		es->es_rdrp = ((struct le_md *) (((u_long) a) & ~7)) + 1;
		a = kmem_alloc((u_int)
			(sizeof (struct le_md) * (es->es_ntmds + 1)));
		es->es_tdrp = ((struct le_md *) (((u_long) a) & ~7)) + 1;
		/*
		 * Remember address of last descriptor in the ring for
		 * ease of bumping pointers around the ring.
		 */
		es->es_rdrend = &((es->es_rdrp)[es->es_nrmds-1]);
		es->es_tdrend = &((es->es_tdrp)[es->es_ntmds-1]);

		/* Allocate buffers. */
		es->es_rbufs = getstruct(struct le_buf , es->es_nrbufs);
		es->es_tbufs = getstruct(struct le_buf , es->es_ntbufs);
	}
}

/*
 * Reset of interface after system reset.
 */
lereset(unit)
	int unit;
{
	register struct mb_device *md;

	if (unit >= NLE || (md = leinfo[unit]) == 0 || md->md_alive == 0)
		return;
	printf(" le%d", unit);
	leinit(unit);
}


/*
 * Initialization of interface; clear recorded pending
 * operations.
 */
leinit(unit)
	int unit;
{
	register struct le_softc *es = &le_softc[unit];
	register struct le_device *le;
	register struct le_init_block *ib;
	register int s;
	struct ifnet *ifp = &es->es_if;

	s = splimp();

	/*
	 * Freeze the chip before doing anything else.
	 */
	le = (struct le_device *)leinfo[unit]->md_addr;
	le->le_csr = LE_STOP;

	ifp->if_flags &= ~(IFF_UP | IFF_RUNNING);

	if (!address_known(ifp)) {
		(void) splx(s);
		return;
	}

	/*
	 * Insure that resources are available.
	 *	(The first time through, this will allocate them.)
	 */
	le_alloc_buffers(es);

	/*
	 * Reset message descriptors.
	 */
	es->es_his_rmd = es->es_rdrp;
	es->es_cur_tmd = es->es_tdrend;
	es->es_nxt_tmd = es->es_tdrp;

	/* Initialize buffer allocation information. */

	/*
	 * Set up the receive buffer free list.
	 *	All receive buffers but those that are currently
	 *	loaned out go on it.
	 *
	 *	Note that this code relies on the integrity of
	 *	the loan-out list.
	 */
	{
		register int i;

		es->es_rbuf_free = (struct le_buf *)0;
		for (i = 0; i < es->es_nrbufs; i++) {
			register struct le_buf	*rb = &es->es_rbufs[i];

			if (le_onlist(&es->es_rbuf_loaned, rb))
				continue;
			free_rbuf(es, rb);
		}
	}

	/* Construct the initialization block */
	ib = es->es_ib;
	bzero((caddr_t)ib, (u_int) sizeof (struct le_init_block));

	/*
	 * Mode word 0 should be all zeroes except
	 * possibly for the promiscuous mode bit.
	 */
	if (ifp->if_flags & IFF_PROMISC)
		ib->ib_prom = 1;

	ib->ib_padr[0] = es->es_enaddr.ether_addr_octet[1];
	ib->ib_padr[1] = es->es_enaddr.ether_addr_octet[0];
	ib->ib_padr[2] = es->es_enaddr.ether_addr_octet[3];
	ib->ib_padr[3] = es->es_enaddr.ether_addr_octet[2];
	ib->ib_padr[4] = es->es_enaddr.ether_addr_octet[5];
	ib->ib_padr[5] = es->es_enaddr.ether_addr_octet[4];
						
	/* No multicast filter yet, FIXME MULTICAST, leave zeros. */

	ib->ib_rdrp.drp_laddr = (long)es->es_rdrp;
	ib->ib_rdrp.drp_haddr = (long)es->es_rdrp >> 16;
	ib->ib_rdrp.drp_len   = (long)es->es_nrmdp2;
	ib->ib_tdrp.drp_laddr = (long)es->es_tdrp;
	ib->ib_tdrp.drp_haddr = (long)es->es_tdrp >> 16;
	ib->ib_tdrp.drp_len   = (long)es->es_ntmdp2;

	/* Clear all the descriptors */
	bzero((caddr_t)es->es_rdrp,
			(u_int) (es->es_nrmds * sizeof (struct le_md)));
	bzero((caddr_t)es->es_tdrp,
			(u_int) (es->es_ntmds * sizeof (struct le_md)));

	/* Hang out the receive buffers. */
	{
		register struct le_buf *rb;

		while (rb = get_rbuf(es)) {
			register struct le_md *rmd = es->es_his_rmd;

			install_buf_in_rmd(rb, rmd);
			rmd->lmd_flags = LMD_OWN;
			es->es_his_rmd = next_rmd(es, rmd);
			if (es->es_his_rmd == es->es_rdrp)
				break;
		}
		/*
		 * Verify that all receive ring descriptor slots
		 * were filled.
		 */
		if (es->es_his_rmd != es->es_rdrp) {
			identify(&es->es_if);
			panic("leinitrbufs");
		}
	}

	/* Give the init block to the chip */
	le->le_rap = LE_CSR1;	/* select the low address register */
	le->le_rdp = (long)ib & 0xffff;

	le->le_rap = LE_CSR2;	/* select the high address register */
	le->le_rdp = ((long)ib >> 16) & 0xff;

	le->le_rap = LE_CSR3;	/* Bus Master control register */
	le->le_rdp = LE_BSWP;

	le->le_rap = LE_CSR0;	/* main control/status register */
	le->le_csr = LE_INIT;

	{
		int i;

		for (i = 10000; ! (le->le_csr & LE_IDON); i-- ) {
			if (i <= 0) {
				identify(&es->es_if);
				panic("chip didn't initialize");
			}
		}
	}
	le->le_csr = LE_IDON;		/* Now reset the interrupt */

	/*
	 * Clear software record of pending operations
	 * and clear pending timeouts.
	 */
	if (es->es_flags & LE_TBUSY) {
		register struct tdesc	*td = &es->es_tpack;

		if (td->td_mb)
			m_freem(td->td_mb);
		td->td_tail = (u_char *) 0;
		td->td_tlen = 0;
		es->es_flags &= ~LE_TBUSY;
	}
	if (es->es_flags & LE_TOPENDING) {
		int	le_xmit_reset();

		untimeout(le_xmit_reset, (caddr_t) es);
		es->es_flags &= ~LE_TOPENDING;
	}

	/* (Re)start the chip. */
	le->le_csr = LE_STRT | LE_INEA;

	ifp->if_flags |= IFF_UP|IFF_RUNNING;
	if (ifp->if_snd.ifq_head)
		lestart(unit);

	(void) splx(s);

	route_arp(ifp, &es->es_ac);
}

/*
 * Set the receive descriptor rmd to refer to the buffer rb.
 *	Note that we don't give the descriptor/buffer pair
 *	back to the chip here, since our caller may not be
 *	ready to give it up yet.
 */
install_buf_in_rmd(rb, rmd)
	struct le_buf *rb;
	register struct le_md *rmd;
{
	/*
	 * Careful here -- the chip considers the buffer to start
	 * with an ether header, so we really have to point it at
	 * the lb_ehdr field of our buffer structure.
	 */
	register u_char *buffer = (u_char *) &rb->lb_ehdr;

	rmd->lmd_ladr = (u_short) buffer;
	rmd->lmd_hadr = (long)buffer >> 16;
	rmd->lmd_bcnt = -MAXBUF;
	rmd->lmd_mcnt = 0;
}

/*
 * Repossess a loaned-out receive buffer.
 *	Called from within MFREE by the loanee when disposing
 *	of the cluster-type mbuf wrapped around the buffer.
 *
 *	Assumes called with lists locked.
 */
leclfree(lbp)
	register struct le_buf	*lbp;
{
	register struct le_softc	*es;
	register struct le_buf		**predp;

	if (!lbp)
		panic("leclfree");

	es = lbp->lb_es;
	if (!es) {
		printf("lbp: %x\n", lbp);
		panic("leclfree2");
	}

	/*
	 * Transfer it from the loan-out list to the free list.
	 */
	predp = le_lpredp(&es->es_rbuf_loaned, lbp);
	if (!predp) {
		identify(&es->es_if);
		printf("lbp: %x, es: %x\n", lbp, es);
		panic("leclfree4");
	}
	*predp = lbp->lb_next;
	free_rbuf(es, lbp);
}

/*
 * Handle transmitter freeze-up by resetting the chip.
 *	This routine shouldn't be necessary, but the LANCE is still buggy.
 *	This code could be rewritten to eliminate the LE_TOPENDING flag,
 *	but retaining it makes it more obvious what's going on.
 */
le_xmit_reset(es)
	struct le_softc	*es;
{
	register int		s = splimp();

	/*
	 * The timeout occurred and therefore is no longer pending.
	 */
	es->es_flags &= ~LE_TOPENDING;

	identify(&es->es_if);
	printf("transmitter frozen -- resetting\n");

	leinit(es - le_softc);

	(void) splx(s);
}

/*
 * Start or restart output on interface.
 * If interface is already active, then this is a nop.
 * If interface is not already active, get another packet
 * to send from the interface queue, and map it to the
 * interface before starting the output.
 */
lestart(dev)
	dev_t dev;
{
	int			unit = minor(dev);
	struct le_softc		*es = &le_softc[unit];
	struct le_device	*le;
	register struct le_md	*t,
				*t0;
	struct le_md		*tnext;
	u_char			*tbuf;
	register int		bcnt;
	register struct mbuf	*m,
				*mprev;
	struct tdesc		*tpack = &es->es_tpack;
	extern struct mbuf	*ether_pullup();

	/*
	 * Check for transmit buffer busy.
	 */
	if (es->es_flags & LE_TBUSY)
		return;
	le = (struct le_device *)leinfo[unit]->md_addr;

	/*
	 * FIXME BACK_TO_BACK: (Well, maybe not...)
	 * We may eventually want to pull as many packets as possible off
	 * the send queue and put them in the transmit descriptor ring
	 * for back-to-back transmission.  For now, however, we'll keep
	 * it simple and just send one transmit buffer at a time.
	 *
	 * In fact experience indicates that it's unprofitable to
	 * contemplate back-to-back transmission.  The frequency
	 * of arriving here with more than one packet to send turns
	 * out to be very low.  Moreover, supporting back-to-back
	 * transmission would make the code that avoids copying
	 * outgoing packet mbufs much more hairy.
	 */

	IF_DEQUEUE(&es->es_if.if_snd, m);
	if (m == (struct mbuf *) 0)
		return;

	/* Gather statistics. */
	es->es_started++;
	if (es->es_if.if_snd.ifq_len > 0)
		es->es_started2++;

	/* Set the ethernet source address because the hardware won't. */
	mtod(m, struct ether_header *)->ether_shost = es->es_enaddr;

	/*
	 * Force the first buffer of the outgoing packet
	 * to be at least the minimum size the chip requires.
	 *	There's a potential inefficiency here.  If we end up
	 *	copying the entire packet into a transmit buffer after
	 *	having pulled up to the min TU, we'll have done a
	 *	redundant copy.  Add an additional check?
	 */
	m = ether_pullup(m, LANCE_MIN_TU);
	if (m == (struct mbuf *) 0) {
		identify(&es->es_if);
		printf("out of mbufs: output packet dropped\n");
		return;
	}
	/*
	 * If the resulting length is still small, we know that
	 * the packet is entirely contained in the pulled-up
	 * mbuf.  In this case, we must insure that the packet's
	 * length is at least the Ethernet minimum transmission
	 * unit.  We can reach in and adjust the mbuf's size with
	 * impunity because: ether_pullup has returned us an mbuf
	 * whose data begins at the head of mbuf's data area (so
	 * that the entire mbuf is available), and the required size
	 * is less than the mbuf size.
	 */
	if (m->m_len < ETHER_MIN_TU)
		m->m_len = ETHER_MIN_TU;
	/*
	 * Record addresses of buffer locations for the current outgoing
	 * packet, so that we can reclaim the buffers when the chip's
	 * through with them or requeue the packet if necessary.
	 *	N.B.: This scheme assumes there's at most one outgoing
	 *	packet transmission in progress at a time.  If this
	 *	assumption changes, this code will require nontrivial
	 *	modification.
	 */
	tpack->td_mb = m;
	tpack->td_tail = (u_char *) 0;
	tpack->td_tlen = 0;
	/*
	 * Wrap a transmit descriptor around each of the packet's
	 * mbufs.  Check for resource exhaustion and handle it by
	 * copying the remainder of the packet into a statically
	 * allocated transmit buffer, pointing the last tmd at it.
	 */
	mprev = (struct mbuf *) 0;
	t0 = t = es->es_cur_tmd = es->es_nxt_tmd;
	do {
		tnext = next_tmd(es, t);

		if (tnext != t0 || ! m->m_next) {
			/* Normal case: use the mbuf itself as the buffer. */
			tbuf = mtod(m, u_char *);
			bcnt = m->m_len;
		}
		else {
			/*
			 * Resource exhaustion: copy the remainder,
			 * after breaking it off the mbuf chain
			 * leading to it.
			 */
			es->es_no_tmds++;
			if (mprev)
				mprev->m_next = (struct mbuf *) 0;
			else
				tpack->td_mb = (struct mbuf *) 0;
			tbuf = tpack->td_tail =
				(u_char *) &es->es_tbufs->lb_ehdr;
			bcnt = tpack->td_tlen = copy_from_mbufs (tbuf, m);
			m = (struct mbuf *) 0;
		}
		if (t->lmd_flags & LMD_OWN) {
			identify(&es->es_if);
			panic("lestart: tmd ownership conflict");
		}
		/*
		 * Point the tmd at the buffer.
		 */
		t->lmd_ladr = (u_short) tbuf;
		t->lmd_hadr = (int) tbuf >> 16;
		t->lmd_bcnt = -bcnt;
		t->lmd_flags3 = 0; 
		t->lmd_flags = 0;

		t = tnext;
		mprev = m;
	} while (m && (m = m->m_next));

	/*
	 * T and tnext now point to the first slot to be used
	 * for the next packet.
	 */
	es->es_nxt_tmd = t;

	/*
	 * Fire off the packet by giving each of its associated descriptors to
	 * the LANCE chip, setting the start- and end-of-packet flags as well.
	 *	We give the descriptors back in reverse order to prevent
	 *	race conditions with the chip.
	 */
	t0->lmd_flags = LMD_STP;
	prev_tmd(es,t)->lmd_flags |= LMD_ENP;
	do {
		t = prev_tmd(es, t);
		t->lmd_flags |= LMD_OWN;
	} while (t != t0);
	es->es_flags |= LE_TBUSY;

	le->le_csr = LE_TDMD | LE_INEA;

	/*
	 * Schedule a timeout to guard against transmitter freeze-up.
	 * This timeout is cancelled when we get the transmitter interrupt
	 * for this packet.  Eventually (when we get bug free parts) we
	 * should be able to remove it altogether.
	 */
	es->es_flags |= LE_TOPENDING;
	timeout(le_xmit_reset, (caddr_t) es, hz << 2);
}

/*
 * Put the current output packet back on the output queue.  Adjust the
 * bookkeeping fields in the softc structure to reflect the altered
 * state.
 *	This code depends on being able to recreate the mbuf chain for
 *	the packet in question from the information in es->es_tpack.
 */
le_requeue(es)
	register struct le_softc	*es;
{
	register struct tdesc	*td = &es->es_tpack;
	register struct mbuf	*mtail = (struct mbuf *) 0,
				*m;

#ifdef LEDEBUG
	if (td->td_tail)
		printf("le_requeue: td: %x, td->td_tlen: %d, td->td_tail: %x, td->td_mb %x\n",
			td, td->td_tlen, td->td_tail, td->td_mb);
#endif LEDEBUG

	/*
	 * Convert the tail buffer back into an mbuf chain.
	 */
	if (td->td_tlen > 0) {
		mtail = copy_to_mbufs((caddr_t) td->td_tail,
					(int) td->td_tlen, 0);
		if (!mtail) {
			identify(&es->es_if);
			printf("out of mbufs for packet requeue\n");
			return;
		}
	}
	/*
	 * Glue the two mbuf chains together and
	 * set m to point to the result.
	 */
	if (m = td->td_mb) {
		if (mtail) {
			while (m->m_next)
				m = m->m_next;
			m->m_next = mtail;
			m = td->td_mb;
		}
	}
	else
		m = mtail;

	/*
	 * Requeue the reconstituted packet.
	 */
	if (!IF_QFULL(&es->es_if.if_snd))
		IF_PREPEND(&es->es_if.if_snd, m);

	/*
	 * Remove record of committed storage for the packet.
	 */
	td->td_mb = (struct mbuf *) 0;
	td->td_tail = (u_char *) 0;
	td->td_tlen = 0;
}

/*
 * Ethernet interface interrupt.
 */
leintr()
{
	register struct le_softc *es;
	register struct le_device *le;
	register struct le_md *lmd;
	register struct mb_device *md;
	register int unit;
	int serviced = 0;

	es = &le_softc[0];
	for (unit = 0; unit < NLE; unit++, es++) {
		if ((md = leinfo[unit]) == 0 || md->md_alive == 0)
			continue;
		le = (struct le_device *)md->md_addr;

		if (!(le->le_csr & LE_INTR))
			continue;

		/* Keep statistics for lack of heartbeat */
		if (le->le_csr & LE_CERR) {
			le->le_csr = LE_CERR | LE_INEA;
			es->es_noheartbeat++;
		}

		/*
		 * The chip's internal (externally invisble) pointer
		 * always points to the rmd just after the last
		 * one that the software has taken from the chip.
		 * Es_his_rmd always points to the rmd just after the
		 * last one that was given to the chip.
		 */

/*
 * It is possible to omit the RINT test and to just check for
 * the OWN bit clear in the next rmd.  However, the RINT bit
 * provides a nice consistency check that should probably
 * stay in until the driver is stable.
 */
		/* Check for receive activity */
		if ( (le->le_csr & LE_RINT) && (le->le_csr & LE_RXON) ) {
			/* Pull packets off interface */
			for (lmd = es->es_his_rmd;
			     !(lmd->lmd_flags & LMD_OWN);
			     es->es_his_rmd = lmd = next_rmd(es, lmd)) {
				serviced = 1;

/*
 * We acknowledge the RINT inside the loop so that the own bit for the
 * next packet will be checked *after* RINT is acknowledged.  If we were
 * to acknowledge the RINT just once after the loop, a packet could come in
 * between the last test of the own bit and the time we do the RINT,
 * in which case we might not see the packet (because we cleared its
 * RINT indication but we did not see the own bit become clear).
 *
 * Race prevention: since the chip uses the order <clear own bit, set RINT>,
 * we must use the opposite order <clear RINT, set own bit>.
 */
				le->le_csr = LE_RINT | LE_INEA;

				leread(es, lmd);

				/*
				 * Give the descriptor and associated
				 * buffer back to the chip.
				 */
				lmd->lmd_mcnt = 0;
				lmd->lmd_flags = LMD_OWN;
			}
			if (!serviced) {
				/*
				 * This code isn't satisfactory -- we
				 * really should be able to handle cases
				 * where it turns out there's nothing
				 * to do.
				 */
				identify(&es->es_if);
#ifdef DEBUG
				halt("RINT with buffer owned by chip");
#else DEBUG
				panic("RINT with buffer owned by chip");
#endif DEBUG
			}
		}

		/* Check for transmit activity */
		if ((le->le_csr & LE_TINT) && (le->le_csr & LE_TXON)) {
		    int	retransmit = le_retransmit;
		    int	xerr = 0;

		    /*
		     * Check each of the packet's descriptors
		     * for errors.
		     */
		    lmd = es->es_cur_tmd;
		    do {
			/*
			 * Check for loss of descriptor synchronization with
			 * the LANCE chip.  Following a transmit error the chip
			 * immediately sets TINT and then clears the OWN bit
			 * for the remaining tmds of the packet, opening a
			 * small window where it can appear we've gotten out of
			 * sync.  Therefore we check again after delaying for
			 * a bit.
			 */
			if (lmd->lmd_flags & LMD_OWN) {
			    DELAY(1000);
			    if (lmd->lmd_flags & LMD_OWN) {
				printf("tmd=%x, flags=%x, chip=%x, csr=%x\n",
				    lmd, lmd->lmd_flags, le, le->le_csr);
				panic("TINT but buffer owned by LANCE");
			    }
			}

			/*
			 * Keep retry statistics.  Note that they don't result
			 * in aborted transmissions.  Since the chip duplicates
			 * this information into each descriptor for a packet,
			 * we check only the first.  We can keep only an
			 * approximate count of collisions, since the chip
			 * gives us only 0/1/more-than-1 information.  We
			 * count the last case as 2.
			 */
			if (  (lmd->lmd_flags & (TMD_MORE | TMD_ONE))
			   && (lmd->lmd_flags & LMD_STP)
			   ) {
				es->es_retries++;
				es->es_if.if_collisions +=
				    (lmd->lmd_flags & TMD_MORE) ? 2 : 1;
			}

			/*
			 * These errors cause the packet to be aborted.  (What
			 * happens if the packet's aborted before the chip gets
			 * to its last descriptor?  Does the chip advance past
			 * the offending descriptor?)
			 */
			if (lmd->lmd_flags3 &
			    (TMD_BUFF|TMD_UFLO|TMD_LCOL|TMD_LCAR|TMD_RTRY)) {
				xerr = 1;
				retransmit &=
				    le_xmit_error(es, lmd->lmd_flags3);
				es->es_if.if_oerrors++;
			}
			lmd = next_tmd(es, lmd);
		    } while (lmd != es->es_nxt_tmd);

		    le->le_csr = LE_TINT | LE_INEA;
		    serviced = 1;
		    es->es_if.if_opackets++;

		    /*
		     * Cancel the watchdog transmitter timeout.
		     */
		    if (es->es_flags & LE_TOPENDING) {
			    untimeout(le_xmit_reset, (caddr_t) es);
			    es->es_flags &= ~LE_TOPENDING;
		    }

		    /*
		     * Arrange to retry a failed transmission attempt.
		     */
		    if (xerr && retransmit)
			le_requeue(es);

		    /*
		     * Free the mbuf chain associated with the
		     * packet that just completed transmission.
		     */
		    if (es->es_flags & LE_TBUSY) {
			    struct tdesc	*td = &es->es_tpack;

			    if (td->td_mb)
				    m_freem(td->td_mb);
			    td->td_tail = (u_char *) 0;
			    td->td_tlen = 0;
			    es->es_flags &= ~LE_TBUSY;
		    }
		    else {
			    identify(&es->es_if);
			    printf("stray transmitter interrupt\n");
		    }

		    /* Send more packets if there are any. */
		    if (es->es_if.if_snd.ifq_head)
			    lestart(unit);
		}

		/*
		 * Check for errors not specifically related
		 * to transmission or reception.
		 */
		if ( (le->le_csr & (LE_BABL|LE_MERR|LE_MISS|LE_TXON|LE_RXON))
		     != (LE_RXON|LE_TXON) ) {
			serviced = 1;
			le_chip_error(es, le);
		}
	}
	return (serviced);
}

/*
 * Move info from driver toward protocol interface
 */
leread(es, rmd)
	register struct le_softc *es;
	register struct le_md *rmd;
{
	register struct ether_header *header;
	register caddr_t buffer;
	register struct mbuf *m;
	register struct le_buf *rcvbp;
	struct le_buf *rplbp;
	int length;
	int off;

	es->es_if.if_ipackets++;

	/* Check for packet errors. */

	/*
	 * ! ENP is an error because we have allocated huge receive buffers.
	 * I.e., we don't do buffer chaining.
	 */
	if ((rmd->lmd_flags & ~RMD_OFLO) != (LMD_STP|LMD_ENP)) {
		le_rcv_error(es, rmd->lmd_flags);
		es->es_if.if_ierrors++;
		return;
	}

	/*
	 * Convert the buffer address embedded in the receive
	 * descriptor to the address of the le_buf structure
	 * containing the buffer.
	 */
	rcvbp = e_to_lb(le_buf_addr(rmd));
	/*
	 * Get input data length (minus ethernet header and crc),
	 * pointer to ethernet header, and address of data buffer.
	 */
	length = rmd->lmd_mcnt - sizeof (struct ether_header) - 4;
	header = &rcvbp->lb_ehdr;
	buffer = (caddr_t) &rcvbp->lb_buffer[0];

	/* Check for runt packet */
	if (length <= 0) {
		identify(&es->es_if);
		printf("runt packet\n");
		es->es_if.if_ierrors++;
		return;
	}

	/*
	 * Check for unreported packet errors.  Revs C and D of the LANCE
	 * chip have a bug that can cause "random" bytes to be prepended
	 * to the start of the packet.  The work-around is to make sure
	 * that the Ethernet destination address in the packet matches
	 * our address.
	 */
#define ether_addr_not_equal(a,b)	\
	(  ( *(long  *)(&(a).ether_addr_octet[0]) != \
	     *(long  *)(&(b).ether_addr_octet[0]) )  \
	|| ( *(short *)(&(a).ether_addr_octet[4]) != \
	     *(short *)(&(b).ether_addr_octet[4]) )  \
	)

	/*
	 * FIXME MULTICAST: this disallows multicast packet reception.
	 * The fix is either to check against the entire list of multicast
	 * addresses (which we don't even have a mechanism for setting
	 * right now), or to get chips that don't have this bug.
	 *
	 * Moreover, if we've gone into promiscuous mode, there's no
	 * way at all we can tell whether we've been hit by the bug.
	 * In this case, we must disable the check.
	 */
	if (ether_addr_not_equal(header->ether_dhost, es->es_enaddr) &&
	    ether_addr_not_equal(header->ether_dhost, etherbroadcastaddr) &&
	    ! (es->es_if.if_flags & IFF_PROMISC)) {
#ifdef LEDEBUG
		identify(&es->es_if);
		printf("LANCE Rev C/D Extra Byte(s) bug; Packet dropped\n");
#endif LEDEBUG
		es->es_extrabyte++;
		es->es_if.if_ierrors++;
		return;
	}

	if (check_trailer(header, buffer, &length, &off)) {
		identify(&es->es_if);
		printf("trailer error\n");
		es->es_if.if_ierrors++;
		return;
	}

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; copy_to_mbufs will then force this header
	 * information to be at the front and will drop the extra trailer
	 * type and length fields.
	 */
	/*
	 * Receive buffer loan-out:
	 *	We're willing to loan the buffer containing this
	 *	packet to the higher protocol layers provided that
	 *	the packet's big enough and doesn't have a trailer
	 *	and that we have a spare receive buffer to use as
	 *	a replacement for it.
	 *
	 * FIXME STATISTICS: keep track of current and maximum loan-out
	 * list length.
	 */
	if (length > MLEN && off == 0)
		es->es_potential_rloans++;
	/*
	 * If everything's go, wrap the receive buffer up
	 * in a cluster-type mbuf and make sure it worked.
	 */
	if (  length > MLEN && off == 0
	   && (rplbp = es->es_rbuf_free)
	   && (m = mclgetx(leclfree, (int) rcvbp, buffer, length, M_DONTWAIT))
	   ) {
		es->es_actual_rloans++;
		/*
		 * Link the receive buffer into the loan-out list
		 * and set its softc pointer.
		 */
		loan_rbuf(es, rcvbp);
		rcvbp->lb_es = es;
		/*
		 * Replace the newly loaned buffer and move the
		 * replacement out of the free list.
		 */
		install_buf_in_rmd(rplbp, rmd);
		es->es_rbuf_free = rplbp->lb_next;
	}
	else
		m = copy_to_mbufs(buffer, length, off);
	if (m == (struct mbuf *) 0)
		return;

	do_protocol(header, m, &es->es_ac, length);
}

leoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	return (ether_output(ifp, m0, dst, lestart,
	                             &le_softc[ifp->if_unit].es_ac));
}

/*
 * Process an ioctl request.
 */
leioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register int			unit = ifp->if_unit;
	register struct le_softc	*es = &le_softc[unit];
	register int			error = 0;
	int				s = splimp();

	switch (cmd) {

	case SIOCSIFADDR:
		error = set_if_addr(ifp, data, &es->es_enaddr);
		break;

	case SIOCGIFADDR:
		bcopy((caddr_t) &es->es_enaddr,
			(caddr_t) &((struct ifreq *)data)->ifr_addr.sa_data[0],
			sizeof (struct ether_addr));
		break;

	case SIOCGIFFLAGS:
		*(short *) data = ifp->if_flags;
		break;

	case SIOCSIFFLAGS:
	    {
		/*
		 * Set interface flags.  Possibilities are: bringing
		 * the interface up or down and setting it into or out
		 * of promiscuous mode.
		 */
		short		flags = *(short *) data;
		register int	up;
		register int	promisc;

		/*
		 * See whether anything needs to change.
		 */
		up = flags & IFF_UP;
		promisc = flags & IFF_PROMISC;
		if (  up != (ifp->if_flags & IFF_UP)
		   || promisc != (ifp->if_flags & IFF_PROMISC)
		   ) {
			struct sockaddr	sa;

			/*
			 * To bring the interface down, we temporarily
			 * zap its address, so that leinit will decide
			 * it can't meaningfully be initialized.
			 */
			if (!up) {
				sa = ifp->if_addr;
				bzero((caddr_t) &ifp->if_addr,
					(u_int) sizeof (ifp->if_addr));
			}

			/* Bring the new state into effect. */
			ifp->if_flags = flags;
			leinit(unit);

			/*
			 * Restore the interface address and make
			 * sure that the interface flags are mutually
			 * consistent.
			 */
			if (!up)
				ifp->if_addr = sa;
			if (ifp->if_flags & IFF_UP)
				ifp->if_flags |= IFF_RUNNING;
			else
				ifp->if_flags &= ~(IFF_RUNNING | IFF_PROMISC);

			if (promisc & ! (ifp->if_flags & IFF_PROMISC))
				error = ENETDOWN;
		}
		else {
			/*
			 * Interface itself unaffected.  Just copy new flags.
			 */
			ifp->if_flags = flags;
		}
		break;
	    }

	default:
		error = EINVAL;
	}

	(void) splx(s);

	return (error);
}

le_rcv_error(es, flags)
	struct le_softc *es;
	u_char flags;
{
	if (flags & RMD_FRAM)
		es->es_fram++;
	if (flags & RMD_CRC )
		es->es_crc++;
	if (flags & RMD_OFLO)
		es->es_oflo++;
	if (flags & RMD_BUFF) {
		identify(&es->es_if);
		printf("Receive buffer error - BUFF bit set in rmd\n");
	}
	if (!(flags & LMD_STP)) {
		identify(&es->es_if);
		printf("Received packet with STP bit in rmd cleared\n");
	}
	if (!(flags & LMD_ENP)) {
		identify(&es->es_if);
		printf("Received packet with ENP bit in rmd cleared\n");
	}
}

/*
 * Report on transmission errors and return 1 if said errors
 * definitely prevented the subject packet from being transmitted.
 * return 0 otherwise.
 *
 * If le_retransmit is on (implying that packets will be requeued)
 * we suppress the printouts but still update counters.
 * 
 *	Partially transmitted packets are treated as not having been
 *	transmitted at all, the rationale being that they'll be rejected
 *	with bad checksums on the receiving end.
 */
le_xmit_error(es, flags)
	struct le_softc *es;
	u_short flags;
{
	register int	didntgo = 0;

	/*
	 * The BUFF bit isn't valid if either of the RTRY or LCOL bits is set.
	 */
	if ((flags & (TMD_BUFF | TMD_RTRY | TMD_LCOL)) == TMD_BUFF) {
		if (!le_retransmit) {
			identify(&es->es_if);
			printf("Transmit buffer error - BUFF bit set in tmd\n");
		}
		es->es_tBUFF++;
		didntgo = 1;
	}
	if (flags & TMD_UFLO) {
		if (!le_retransmit) {
			identify(&es->es_if);
			printf("Transmit underflow error\n");
		}
		es->es_uflo++;
		didntgo = 1;
	}
	if (flags & TMD_LCOL) {
		if (!le_retransmit) {
			identify(&es->es_if);
			printf("Transmit late collision - net problem?\n");
		}
		es->es_tlcol++;
		didntgo = 1;
	}
	if (flags & TMD_LCAR) {
		/*
		 * The chip continues transmitting after detecting this
		 * condition, so the packet may actually have made it out.
		 */
		identify(&es->es_if);
		printf("No carrier - transceiver cable problem?\n");
		es->es_tnocar++;
	}
	if (flags & TMD_RTRY) {
		if (!le_retransmit) {
			identify(&es->es_if);
			printf("Transmit retried more than 16 times - net jammed\n");
		}
		es->es_trtry++;
		didntgo = 1;
	}

	return (didntgo);
}

/* Handles errors that are reported in the chip's status register */
/* ARGSUSED */
le_chip_error(es, le)
	struct le_softc  *es;
	struct le_device *le;
{
	register u_short	csr = le->le_csr;
	int restart = 0;

	if (csr & LE_MISS) {
		es->es_missed++;
#ifdef LEDEBUG
		identify(&es->es_if);
		printf("missed packet\n");
#endif			
		le->le_csr = LE_MISS | LE_INEA;
	}

	if (csr & LE_BABL) {
	    identify(&es->es_if);
	    printf("Babble error - sent a packet longer than the maximum length\n");
	    le->le_csr = LE_BABL | LE_INEA;
	}
	/*
	 * If a memory error has occurred, both the transmitter
	 * and the receiver will have shut down.
	 */
	if (csr & LE_MERR) {
	    identify(&es->es_if);
	    printf("Memory Error!  Ethernet chip memory access timed out\n");
	    le->le_csr = LE_MERR | LE_INEA;
	}
	if ( !(csr & LE_RXON) ) {
	    identify(&es->es_if);
	    printf("Reception stopped\n");
	    restart++;
	}
	if ( !(csr & LE_TXON) ) {
	    identify(&es->es_if);
	    printf("Transmission stopped\n");
	    restart++;
	}
	if (restart) {
	    identify(&es->es_if);
	    le_print_csr(csr);
	    leinit(es - le_softc);
	}
}

/*
 * Print out a csr value in a nicely formatted way.
 */
le_print_csr (csr)
	register u_short	csr;
{
	printf("csr: %b\n", csr,
"\20\20ERR\17BABL\16CERR\15MISS\14MERR\13RINT\12TINT\11IDON\10INTR\7INEA\6RXON\5TXON\4TDMD\3STOP\2STRT\1INIT\n");
}


/*
 * Buffer manipulation routines.
 */

/*
 * Remove a receive buffer from the free list, returning a pointer to it.
 * Any free buffer will do.
 *
 *	Should this be done in-line for speed?
 *
 *	Perhaps should be generalized to grab the first entry of
 *	any of the buffer lists.
 */
struct le_buf *
get_rbuf(es)
	register struct le_softc *es;
{
	register struct le_buf *rb = es->es_rbuf_free;

	if (rb)
		es->es_rbuf_free = rb->lb_next;
	return (rb);
}

/*
 * Given a list rooted at *lpp, return the address of the pointer
 * leading to *ip or NULL if *ip isn't contained in the list.
 */
struct le_buf **
le_lpredp(lpp, ip)
	struct le_buf	**lpp;
	register struct le_buf	*ip;
{
	register struct le_buf	*trailp,
				*p;

	/* Special case for first entry on list. */
	if (!lpp || ((p = *lpp) == ip && ip))
		return (lpp);

	trailp = (struct le_buf *) 0;
	while (p) {
		if (p == ip)
			return (&trailp->lb_next);
		trailp = p;
		p = p->lb_next;
	}

	return ((struct le_buf **) 0);
}


#ifdef LEDEBUG

/*
 * Debugging routines.
 */

le_plist(list, llen)
	struct le_buf	*list;
	register int	llen;
{
	register struct le_buf *p = list;

	do {
		printf("LEDEBUG: le_plist: 0x%x\n", p);
	} while (llen-- > 0 && (p = p->lb_next));
}

le_chkmbuf(m)
	register struct mbuf *m;
{
	while (m) {
		if ((m->m_type != 1 && m->m_type != 2) || m != dtom(m))
			panic("le_chkmbuf");
		m = m->m_next;
	}
}

/*
 * Print out a transmit message descriptor.
 */
le_print_tmd(t)
	register struct le_md	*t;
{
	printf("tmd: %x, count: %d, flags: %b, flags3: %b\n",
		t, t->lmd_bcnt, t->lmd_flags,
		"\20\10OWN\7ERR\5MORE\4ONE\3DEF\2STP\1ENP",
		t->lmd_flags3,
		"\20\20BUFF\17UFLO\15LCOL\14LCAR\13RTRY");
}
#endif LEDEBUG

#endif NLE > 0
