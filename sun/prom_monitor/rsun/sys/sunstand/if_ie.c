#ifndef IEBOOT
#ifndef lint
static	char sccsid[] = "@(#)if_ie.c 1.1 84/12/21 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983,1984 by Sun Microsystems, Inc.
 */

/*
 * For Model 50 debugging we want DEBUG on.  For production, turn it off.
 */
/* #define DEBUG 1 */

/*
 * For PROM space compaction, two symbols are defined here:
 *	MBSUPPORT	 -- support Multibus Ethernet
 *	OBSUPPORT	 -- support On-Board Ethernet
 * For standalone code both symbols should be defined.
 */
#define MBSUPPORT
#define OBSUPPORT

#ifdef IEBOOT
# ifdef VME
#  undef MBSUPPORT
# else  VME
#  undef OBSUPPORT
# endif VME
#endif IEBOOT


/*
 * Sun Intel Ethernet Controller interface
 */
#include "saio.h"
#include "sasun.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../sunif/if_iereg.h"
#include "../sunif/if_mie.h"
#include "../sunif/if_obie.h"
#include "../mon/idprom.h"

/* FIXME: we need a good place for this; nd uses 3000-4000. */
#define SOFTCBASE 0x0A0000

int	iexmit(), iepoll(), iereset();

#ifdef MBSUPPORT
unsigned int iestd[] = { 0x88000, 0x8C000, 0 };
#endif MBSUPPORT

struct saif ieif = {
	iexmit,
	iepoll,
	iereset,
};

#define	IEVVSIZ		1024		/* # of pages in page map */
#define IEPHYMEMSIZ	(8*1024)
#define IEVIRMEMSIZ	(8*1024)
#define IEPAGSIZ	1024
#define	IERBUFSIZ	1600
#define	IEXBUFSIZ	1600

/* controller types */
#define IE_MB	1	/* Multibus */
#define	IE_OB	2	/* onboard */

struct ie_softc {
	union {
		struct	ieiaddr	es_iaddr;
		struct	ieconf	es_ic;
		char	es_fill[IEPAGSIZ-sizeof(struct iescp)];
	} es_junk;
	struct	iescp	es_scp;
	struct	ieiscp	es_iscp;
	struct	iescb	es_scb;
	struct	ierbd	es_rbd;
	struct	ierbd	es_rbd2;	/* Hack for 82586 ucode bugs */
	struct	ierfd	es_rfd;
	struct	ietfd	es_tfd;
	struct	ietbd	es_tbd;
	struct	mie_device *es_mie;
	struct	obie_device *es_obie;
	short	es_type;
	char	es_rbuf[IERBUFSIZ];
	char	es_xbuf[IEXBUFSIZ];	/* Only used in Multibus version */
	char	es_rbuf2[10];		/* Hack for 82586 ucode bugs */
};

#ifdef OBSUPPORT
struct obie_device obie_reset = {0, 0, 0, 0, 0, 0, 0};
#endif OBSUPPORT

ieoff_t to_ieoff();
ieaddr_t to_ieaddr();

#ifdef DEBUG
ieint_t from_ieint();
#endif DEBUG

/*
 * Probe for device.
 * Must return -1 for failure for monitor probe
 */
ieprobe()
{
	register struct mie_device *mie;
	register short *sp;
#ifdef OBSUPPORT
	struct idprom id;

	if (IDFORM_1 == idprom(IDFORM_1, &id)
	    && id.id_machine == IDM_CPU_VME) {
		/* onboard Ethernet */
		return (0);
	} else
#endif OBSUPPORT
	{
#ifdef MBSUPPORT
		/* Multibus Ethernet */
		mie = (struct mie_device *) (MBMEM_BASE + iestd[0]);
		sp = (short *)mie;
		if (poke(sp, 0))
			return (-1);
		sp = &mie->mie_prom[0];
		if (poke(sp, 0x6789))
			return (-1);
		if (peek(sp) == 0x6789)
			return (-1);
		return (0);
#else  MBSUPPORT
		return (-1);
#endif MBSUPPORT
	} 
}

/*
 * Open Intel Ethernet nd connection, return -1 for errors.
 */
ieopen(sip)
	struct saioreq *sip;
{
	register int result;

	sip->si_sif = &ieif;
	ndinit();
	if ( ieinit(sip) || (result = ndopen(sip)) < 0 ) {
		ieclose(sip);		/* Make sure we kill chip */
		return (-1);
	}
	return (result);
}

/*
 * Set up memory maps and Ethernet chip.
 * Returns 1 for error, 0 for ok.
 */
int
ieinit(sip)
	struct saioreq *sip;
{
	register struct ie_softc *es;
	int paddr;
	int i;
#ifdef OBSUPPORT
	struct idprom id;
	
	if (IDFORM_1 == idprom(IDFORM_1, &id)
	    && id.id_machine == IDM_CPU_VME
	    && sip->si_ctlr == 0) {
		/* onboard Ethernet */
		register struct obie_device *obie;

		es = (struct ie_softc *) SOFTCBASE;	/* FIXME */
		es->es_type = IE_OB;
		es->es_obie = obie = ETHER_BASE;
	} else
#endif OBSUPPORT
	{
#ifdef MBSUPPORT
		register struct mie_device *mie;
		struct miepg *pg;
		short *ap;

		if (sip->si_ctlr < ((sizeof iestd)/sizeof *iestd) - 1)
			mie = (struct mie_device *)
					     (MBMEM_BASE+iestd[sip->si_ctlr]);
		else
			mie = (struct mie_device *)(MBMEM_BASE+sip->si_ctlr);
		mie->mie_peack = 1;
		mie->mie_noloop = 0;
		mie->mie_ie = 0;
		mie->mie_pie = 0;
		paddr = mie->mie_mbmhi << 16;
		ap = (short *)mie->mie_pgmap;
		for (i=0; i<IEVVSIZ; i++)	/* note: sets p2mem -> 0 */
			*ap++ = 0;
		for (i=0; i<IEPHYMEMSIZ/IEPAGSIZ; i++) {
			mie->mie_pgmap[0].mp_pfnum = i;
			bzero(MBMEM_BASE+paddr, IEPAGSIZ);
		}
		pg = &mie->mie_pgmap[0];
		for (i=0; i<IEVIRMEMSIZ/IEPAGSIZ; i++) {
			pg->mp_swab = 1;
			pg->mp_pfnum = i;
			pg++;
		}
		/* last page for chip init */
		mie->mie_pgmap[IEVVSIZ-1].mp_pfnum = 0;
		es = (struct ie_softc *)(MBMEM_BASE + paddr);
		es->es_type = IE_MB;
		es->es_mie = mie;
#else  MBSUPPORT
		return 1;		/* OB only, but no ID prom */
#endif MBSUPPORT
	}
	sip->si_devdata = (caddr_t)es;
	return iereset(es);
}

/*
 * Basic 82586 initialization
 * Returns 1 for error, 0 for ok.
 */
int
iereset(es)
	register struct ie_softc *es;
{
	struct ieiscp *iscp = &es->es_iscp;
	struct iescb *scb = &es->es_scb;
	struct ieiaddr *iad = &es->es_junk.es_iaddr;
	struct ieconf *ic = &es->es_junk.es_ic;
	int i, j;
	char gotit;
	struct iescp saveoldscp;
	int savepmap;
	int savepl;
	register struct mie_device *mie = es->es_mie;
	register struct obie_device *obie = es->es_obie;

	for (j = 0; j < 10; j++) {
		/* Set up the control blocks for initializing the chip */
		bzero((caddr_t)&es->es_scp, sizeof (struct iescp));
		es->es_scp.ie_iscp = to_ieaddr(es, (caddr_t)iscp);
		bzero((caddr_t)iscp, sizeof (struct ieiscp));
		iscp->ie_busy = 1;
		iscp->ie_cbbase = 0;
		iscp->ie_scb = to_ieoff(es, (caddr_t)scb);
		iscp->ie_cbbase = to_ieaddr(es, (caddr_t)es);
		bzero((caddr_t)scb, sizeof (struct iescb));
#ifdef OBSUPPORT
		if (es->es_type == IE_OB) {
			/* We have to go into loopback due to chip bugs */
			*obie = obie_reset;	/* Reset chip & interface */
			DELAY(20);
			obie->obie_noreset = 1;	/* stay in loopback */
			DELAY(200);
			/*
			 * Now set up to let the Ethernet chip read the SCP.
			 * Intel wired in the address of the SCP.  It happens
			 * to be 0xFFFFF6, which we use for other things.
			 * So we have to save that page map entry, make it
			 * RAM, save the last 10 bytes of the RAM, and put
			 * the right stuff there.  Then reverse the process
			 * once the chip is initialized.  We disable
			 * interrupts during this, for safety.
			 */
			/* FIXME: savepl = spl7(); */
			savepmap = getpgmap((char *)0xFFFFF6);
			setpgmap((char *)0xFFFFF6,
				getpgmap((char *)&es->es_scp));
			saveoldscp = *(struct iescp *)0xFFFFF6;
			(*(struct iescp *)0xFFFFF6) = es->es_scp;
		} else /* if (es->es_type == IE_MB) */
#endif OBSUPPORT
		{
#ifdef MBSUPPORT
			mie->mie_reset = 1;
			DELAY(20);
			mie->mie_reset = 0;
			DELAY(200);
#endif MBSUPPORT
		}
		ieca(es);
		gotit = 0;
		for (i=0; i<100000; i++) {
			if (iscp->ie_busy != 1) {
				gotit = 1;
				break;
			}
		}
#ifdef OBSUPPORT
		/* Whether or not it worked, we have to clean up. */
		if (es->es_type == IE_OB) {
			/* If it didn't init, reset chip again. */
			if (!gotit) obie->obie_noreset = 0;
			/*
			 * Reverse the process described above.
			 * Restore last 10 bytes of the page, put the old
			 * page map back anyway, then restore the interrupt
			 * level.
			 */
			(*(struct iescp *)0xFFFFF6) = saveoldscp;
			setpgmap((char *)0xFFFFF6, savepmap);
			/* FIXME: (void) splx(savepl); */
		}
#endif OBSUPPORT
		if (gotit) break;	/* Exit loop once we got it */
	}
	if (!gotit) {
		printf("ie: cannot initialize\n");
		return 1;
	}
	bzero((caddr_t)iad, sizeof (struct ieiaddr));
	iad->ieia_cb.ie_cmd = IE_IADDR;
	localetheraddr(NULL, (struct ether_addr *)iad->ieia_addr);
	iesimple(es, &iad->ieia_cb);
	iedefaultconf(ic);
	iesimple(es, &ic->ieconf_cb);
	/*
	 * Take the Ethernet interface chip out of loopback mode, i.e.
	 * put us on the wire.  We can't do this before having initialized
	 * the Ethernet chip because the chip does random things if its
	 * 'wire' is active between the time it's reset and the first CA.
	 */
#ifdef OBSUPPORT
	if (es->es_type == IE_OB)
		obie->obie_noloop = 1;
#endif OBSUPPORT
#ifdef MBSUPPORT
	if (es->es_type == IE_MB)
		mie->mie_noloop = 1; 
#endif MBSUPPORT
	ierustart(es);
	return 0;
}

/*
 * Initialize and start the Receive Unit
 */
ierustart(es)
	register struct ie_softc *es;
{
	register struct ierbd *rbd = &es->es_rbd;
	struct ierbd *rbd2 = &es->es_rbd2;
	register struct ierfd *rfd = &es->es_rfd;
	register struct iescb *scb = &es->es_scb;

	/*
	 * Stop the RU, since we're sharing buffers with it.
	 * Then wait until it has really stopped.
	 */
	while (scb->ie_rus == IERUS_READY) {
		while (scb->ie_cmd != 0)		/* XXX */
			;
		scb->ie_cmd = IECMD_RU_ABORT;
		ieca(es);
	}

	while (scb->ie_cmd != 0)		/* XXX */
		;

	/* Real receive buffer descriptor */
	*(short *)rbd = 0;
	rbd->ierbd_next = to_ieoff(es, (caddr_t)rbd2);		/* Fake */
	rbd->ierbd_el = 0;					/* Fake */
	rbd->ierbd_buf = to_ieaddr(es, es->es_rbuf);
	rbd->ierbd_sizehi = IERBUFSIZ >> 8;
	rbd->ierbd_sizelo = IERBUFSIZ & 0xFF;

	/* Fake receive buffer to avoid chip lockups in B0 mask */
	*(short *)rbd2 = 0;
	rbd2->ierbd_next = -1;		/* unnecessary since el = 1 */
	rbd2->ierbd_el = 1;
	rbd2->ierbd_buf = to_ieaddr(es, es->es_rbuf2);
	rbd2->ierbd_sizehi = 0;
	rbd2->ierbd_sizelo = sizeof(es->es_rbuf2) & 0xFF;

	/* Real receive frame descriptor */
	*(short *)rfd = 0;
	rfd->ierfd_next = -1;		/* Unused since el = 1 */
	rfd->ierfd_el = 1;
	rfd->ierfd_susp = 1;		/* Suspend after receiving one */
	rfd->ierfd_rbd = to_ieoff(es, (caddr_t)rbd);

	/*
	 * Start the RU again.
	 */
	scb->ie_rfa = to_ieoff(es, (caddr_t)rfd);
	scb->ie_cmd = IECMD_RU_START;
	ieca(es);
}

ieca(es)
	register struct ie_softc *es;
{
#ifdef OBSUPPORT
	if (es->es_type == IE_OB) {
		es->es_obie->obie_ca = 1;
		es->es_obie->obie_ca = 0;
	} else /* if (es->es_type == IE_MB) */
#endif OBSUPPORT
	{
#ifdef MBSUPPORT
		es->es_mie->mie_ca = 1;
		es->es_mie->mie_ca = 0;
#endif MBSUPPORT
	}
}

iesimple(es, cb)
	register struct ie_softc *es;
	register struct iecb *cb;
{
	register struct iescb *scb = &es->es_scb;

	*(short *)cb = 0;	/* clear status bits */
	cb->ie_el = 1;
	cb->ie_next = 0;

	/* start CU */
	while (scb->ie_cmd != 0)	/* XXX */
		;
	scb->ie_cbl = to_ieoff(es, (caddr_t)cb);
	scb->ie_cmd = IECMD_CU_START;
	ieca(es);
	while (!cb->ie_done)
		;
	while (scb->ie_cmd != 0)	/* XXX */
		;
	if (scb->ie_cx)
		scb->ie_cmd |= IECMD_ACK_CX;
	if (scb->ie_cnr)
		scb->ie_cmd |= IECMD_ACK_CNR;
	ieca(es);
}

/*
 * Transmit a packet.
 * Multibus has to copy the packet onto the board.
 * Onboard just plays it where it lies.
 */
iexmit(es, buf, count)
	register struct ie_softc *es;
	char *buf;
	int count;
{
	register struct ietbd *tbd = &es->es_tbd;
	register struct ietfd *td = &es->es_tfd;

	bzero((caddr_t)tbd, sizeof *tbd);
	tbd->ietbd_eof = 1;
	tbd->ietbd_cntlo = count & 0xFF;
	tbd->ietbd_cnthi = count >> 8;
#ifdef OBSUPPORT
	if (es->es_type == IE_OB) {
		tbd->ietbd_buf = to_ieaddr(es, buf);
	} else /* if (es->es_type == IE_MB) */
#endif OBSUPPORT
	{
#ifdef MBSUPPORT
		bcopy(buf, es->es_xbuf, count);
		tbd->ietbd_buf = to_ieaddr(es, es->es_xbuf);
#endif MBSUPPORT
	}
	td->ietfd_tbd = to_ieoff(es, (caddr_t)tbd);
	td->ietfd_cmd = IE_TRANSMIT;
	iesimple(es, (struct iecb *)td);
#ifdef DEBUG
	if (!td->ietfd_ok) {
		/* Print status bits from transmit command */
		printf("ie xmit failed: %x\n", *(unsigned short *)td);
	}
#endif DEBUG
	if (td->ietfd_ok)
		return (0);
	if (td->ietfd_xcoll)
		printf("ie: Ethernet cable problem\n");
	return (-1);
}



iepoll(es, buf)
	register struct ie_softc *es;
	char *buf;
{
	register struct ierbd *rbd = &es->es_rbd;
	register struct ierfd *rfd = &es->es_rfd;
	register struct iescb *scb = &es->es_scb;
	int len;
	
#ifdef DEBUG
	/* Check for error status, and printf if so */
	/*
	 * Resource errors should be ignored, they happen even when we
	 * are not interested in listening.
	 */
	if (scb->ie_crcerrs || scb->ie_alnerrs || scb->ie_ovrnerrs) {
		printf("ie recv %x CRC %d, ALN %d, OVRN %d\n",
			scb->ie_rus,
			from_ieint(scb->ie_crcerrs),
			from_ieint(scb->ie_alnerrs),
			from_ieint(scb->ie_ovrnerrs));
		scb->ie_crcerrs = 0;
		scb->ie_alnerrs = 0;
		scb->ie_ovrnerrs = 0;
	}
#endif DEBUG

	/*
	 * Note, this assumes that the RU is set up to get out of READY
	 * state after receiving one packet.
	 */
	if (scb->ie_rus == IERUS_READY) {
		return (0);			/* No packet yet */
	} else {
		/* RU not ready, see if because we got a packet. */
		if (!rfd->ierfd_done || !rbd->ierbd_eof) {
			/* No, randomness zapped it. */
#ifdef DEBUG
			printf("ie recv restart from %x: scb %x frm %x buf %x\n"
				, scb->ie_rus, 
				*(unsigned short *)scb, *(unsigned short *)rfd,
				*(unsigned short *)rbd);
#endif DEBUG
			ierustart(es);
			return (0);
		}
	}

	/*
	 * We got a packet.
	 */
	len = (rbd->ierbd_cnthi << 8) + rbd->ierbd_cntlo;
	bcopy(es->es_rbuf, buf, len);
	while (scb->ie_cmd != 0)	/* XXX */
		;
	if (scb->ie_fr)
		scb->ie_cmd |= IECMD_ACK_FR;
	if (scb->ie_rnr)
		scb->ie_cmd |= IECMD_ACK_RNR;
	ieca(es);
	ierustart(es);
	return (len);
}

/*
 * Convert a CPU virtual address into an Ethernet virtual address.
 *
 * For Multibus, we assume it's in the Ethernet's memory space, and we
 * just subtract off the start of the memory space (==es).  For Model 50,
 * the Ethernet chip has full access to supervisor virtual memory.
 */
ieaddr_t
to_ieaddr(es, cp)
	struct ie_softc *es;
	caddr_t cp;
{
	union {
		int	n;
		char	c[4];
	} a, b;

	if (es->es_type == IE_MB)
		a.n = cp - (caddr_t)es;
	else /* if (es->es_type == IE_OB) */
		a.n = (int)cp;
	b.c[0] = a.c[3];
	b.c[1] = a.c[2];
	b.c[2] = a.c[1];
	b.c[3] = 0;
	return (b.n);
}

/*
 * Convert a CPU virtual address into a 16-bit offset for the Ethernet
 * chip.
 *
 * This is the same for Onboard and Multibus, since the offset is based
 * on the absolute address supplied in the initial system configuration
 * block -- which we customize for Multibus or Onboard.
 */
ieoff_t
to_ieoff(es, addr)
	register struct ie_softc *es;
	caddr_t addr;
{
	union {
		short	s;
		char	c[2];
	} a, b;

	a.s = (short)(addr - (caddr_t)es);
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}


#ifdef DEBUG
ieint_t
from_ieint(n)
	short n;
{
	union {
		short	s;
		char	c[2];
	} a, b;

	a.s = n;
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}
#endif DEBUG

/*
 * Set default configuration parameters
 * As spec'd by Intel, except acloc == 1 for header in data
 */
iedefaultconf(ic)
	register struct ieconf *ic;
{
	bzero((caddr_t)ic, sizeof (struct ieconf));
	ic->ieconf_cb.ie_cmd = IE_CONFIG;
	ic->ieconf_bytes = 12;
	ic->ieconf_fifolim = 8;
	ic->ieconf_pream = 2;		/* 8 byte preamble */
	ic->ieconf_alen = 6;
	ic->ieconf_acloc = 1;
	ic->ieconf_space = 96;
	ic->ieconf_slttmh = 512 >> 8;
	ic->ieconf_minfrm = 64;
	ic->ieconf_retry = 15;
}

/*
 * Close down intel ethernet device.
 * On the Model 50, we reset the chip and take it off the wire, since
 * it is sharing main memory with us (occasionally reading and writing),
 * and most programs don't know how to deal with that -- they just assume
 * that main memory is theirs to play with.
 */
ieclose(sip)
	struct saioreq *sip;
{
#ifdef OBSUPPORT
	register struct ie_softc *es = (struct ie_softc *) sip->si_devdata;

	if (es->es_type == IE_OB) {
		*es->es_obie = obie_reset;
	}
#endif OBSUPPORT
}
