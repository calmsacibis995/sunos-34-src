#ifndef lint
static	char sccsid[] = "@(#)if_ie.c 1.4 87/01/05 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Parameters controlling TIMEBOMB action: times out if chip hangs.
 *
 */
#define	TIMEBOMB	1000000		/* One million times around is all... */
#define	TIMEOUT		-1		/* if (function()) return (TIMEOUT); */
#define	OK		0		/* Traditional OK value */


/*
 * Sun Intel Ethernet Controller interface
 */
#include "saio.h"
#include "param.h"
#include "../h/socket.h"
#include "../sunif/if_iereg.h"
#include "../sunif/if_mie.h"
#include "../sunif/if_obie.h"
#include "../mon/idprom.h"
#include "../mon/cpu.map.h"

int	iexmit(), iepoll(), iereset();

unsigned long iestd[] = { 0x88000, 0x8C000 };
#define NSTD 2

struct saif ieif = {
	iexmit,
	iepoll,
	iereset,
};

#define	IEVVSIZ		1024		/* # of pages in page map */
#define IEPHYMEMSIZ	(16*1024)
#define IEVIRMEMSIZ	(16*1024)
#define IEPAGSIZ	1024
#define	IERBUFSIZ	1600
#define	IEXBUFSIZ	1600

#ifdef SUN2
#define	IEDMABASE	0x000000	/* Can access all of memory */
#endif SUN2
#ifdef SUN3
#define	IEDMABASE	0x0F000000	/* Can access top 16MB of memory */
#endif SUN3

/* Location in virtual memory of the SCP (System Configuration Pointer) */
#define SCP_LOC (char *)(IEDMABASE+IESCPADDR)

/* controller types */
#define IE_MB	1	/* Multibus */
#define	IE_OB	2	/* onboard */

struct ie_softc {
	/*
	 * This first item does two things.  First, it reserves space for
	 * all the various protocols that need to put their locals somewhere
	 * (it was decided that HERE is convenient!).  Second, it fills
	 * out the structure to align es_scp to the wierd-in address
	 * that Intel (thanks guys) fetches it from.
	 */
	char	es_fill[PROTOSCRATCH-sizeof(struct iescp)]; /* Align */
	struct	iescp	es_scp;		/* System config pointer (used once) */
	struct	ieiscp	es_iscp;	/* Intermediate sys config ptr (once) */
	struct	iescb	es_scb;		/* Sys Control Block, the real mama */
	struct	ierbd	es_rbd;
	struct	ierbd	es_rbd2;	/* Hack for 82586 ucode bugs */
	struct	ierfd	es_rfd;
	struct	ietfd	es_tfd;
	struct	ietbd	es_tbd;
	struct	mie_device *es_mie;
	struct	obie_device *es_obie;
	short	es_type;
	struct	ieiaddr	es_iaddr;	/* Cmd to set up our Ethernet addr */
	struct	ieconf	es_ic;		/* Cmd to configure the chip */
	char	es_rbuf[IERBUFSIZ];
	char	es_xbuf[IEXBUFSIZ];	/* buffer accessible by chip */
	char	es_rbuf2[10];		/* Hack for 82586 ucode bugs */
};

/*
 * This initializes the onboard Ethernet control reg to:
 *	Reset is active
 *	Loopback is NOT active
 *	Channel Attn is not active
 *	Interrupt enable is not active.
 * Loopback is deactivated due to a bug in the Intel serial interface
 * chip.  This chip powers-up in a locked up state if Loopback is active.
 * It "unlocks" itself if you release Loopback; then it's OK to reassert it.
 */
struct obie_device obie_reset = {0, 1, 0, 0, 0, 0, 0};

ieoff_t to_ieoff();
ieaddr_t to_ieaddr();

#ifdef DEBUG
ieint_t from_ieint();
#endif DEBUG

struct devinfo ieinfo = {
	sizeof (struct mie_device),
	sizeof(struct ie_softc),
	0,
	NSTD,
	iestd,
	MAP_MBMEM,
	0,				/* transfer size handled by ND */
};

int xxprobe(), xxboot(), ieopen(), ieclose(), etherstrategy(), nullsys();

struct boottab iedriver = {
	"ie",	xxprobe,	xxboot,	ieopen,		ieclose,
	etherstrategy,	"ie: Sun/Intel Ethernet",	&ieinfo,
};

/*
 * Open Intel Ethernet nd connection, return -1 for errors.
 */
ieopen(sip)
	struct saioreq *sip;
{
	register int result;

	sip->si_sif = &ieif;
	if ( ieinit(sip) || (result = etheropen(sip)) < 0 ) {
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
	struct idprom id;
	
	if (idprom(IDFORM_1, &id) == IDFORM_1 &&
	    id.id_machine != IDM_SUN2_MULTI &&
	    (sip->si_ctlr == 0 || sip->si_ctlr == iestd[0])) {
		/* onboard Ethernet */
		register struct obie_device *obie;

		es = (struct ie_softc *)sip->si_dmaaddr;
		bzero((char *)es, sizeof *es);
		es->es_type = IE_OB;
		es->es_obie = obie = (struct obie_device *)
			devalloc(MAP_OBIO, VIOPG_ETHER << BYTES_PG_SHIFT,
				sizeof(*obie));
	} else {
		register struct mie_device *mie;
		struct miepg *pg;
		short *ap;

		mie = (struct mie_device *) sip->si_devaddr;
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
		}
		pg = &mie->mie_pgmap[0];
		for (i=0; i<IEVIRMEMSIZ/IEPAGSIZ; i++) {
			pg->mp_swab = 1;
			pg->mp_pfnum = i;
			pg++;
		}
		/* last page for chip init */
		mie->mie_pgmap[IEVVSIZ-1].mp_pfnum = (PROTOSCRATCH/IEPAGSIZ)-1;
		es = (struct ie_softc *) 
			devalloc(MAP_MBMEM, paddr, sizeof(struct ie_softc));
		bzero((char *)es, sizeof *es);
		es->es_type = IE_MB;
		es->es_mie = mie;
	}
	/* FIXME, release multibus resources ifdef. */
	sip->si_devdata = (caddr_t)es;
	return iereset(es, sip);
}

/*
 * Basic 82586 initialization
 * Returns 1 for error, 0 for ok.
 */
int
iereset(es, sip)
	register struct ie_softc *es;
	struct saioreq *sip;
{
	struct ieiscp *iscp = &es->es_iscp;
	struct iescb *scb = &es->es_scb;
	struct ieiaddr *iad = &es->es_iaddr;
	struct ieconf *ic = &es->es_ic;
	int j;
	int savepmap;
	register struct mie_device *mie = es->es_mie;
	register struct obie_device *obie = es->es_obie;

	for (j = 0; j < 10; j++) {
		/* Set up the control blocks for initializing the chip */
		bzero((caddr_t)&es->es_scp, sizeof (struct iescp));
		es->es_scp.ies_iscp = to_ieaddr(es, (caddr_t)iscp);
		bzero((caddr_t)iscp, sizeof (struct ieiscp));
		iscp->ieis_busy = 1;
		iscp->ieis_scb = to_ieoff(es, (caddr_t)scb);
		iscp->ieis_cbbase = to_ieaddr(es, (caddr_t)es);
		bzero((caddr_t)scb, sizeof (struct iescb));
		if (es->es_type == IE_OB) {
			/*
			 * The 82586 has bugs that require us to be in 
			 * loopback mode while we initialize it, to avoid
			 * transitions on CRS (carrier sense).
			 * 
			 * However, the Intel serial interface chip used
			 * on Carrera also has bugs: if it powers-up in
			 * loopback, you have to release loopback at least
			 * once to make it behave -- it powers up with
			 * CRS and CDT permanently active, which rapes the 586.
			 *
			 * How do you spell "broken"?  I-N-T-E-L...
			 */
			*obie = obie_reset;	/* Reset chip & interface */
			DELAY(20);
			obie->obie_noloop = 0;	/* Put it in loopback now */
			DELAY(20);
			obie->obie_noreset = 1;	/* Release Reset on 82586 */
			DELAY(200);
			/*
			 * Now set up to let the Ethernet chip read the SCP.
			 * Intel wired in the address of the SCP.  It happens
			 * to be 0xFFFFF6.
			 */
			savepmap = getpgmap(SCP_LOC);
			setpgmap(SCP_LOC, getpgmap((char *)&es->es_scp));
		} else {
			mie->mie_reset = 1;
			DELAY(20);
			mie->mie_reset = 0;
			DELAY(200);
		}

		/*
		 * We are set up.  Give the chip a zap, then wait up to
		 * 1 sec, or until chip comes ready.
		 */
		ieca(es);
		CDELAY(iscp->ieis_busy != 1, 1000);

		/* Whether or not it worked, we have to clean up. */
		if (es->es_type == IE_OB) {
			/* If it didn't init, reset chip again. */
			if (iscp->ieis_busy == 1)
				obie->obie_noreset = 0;
			setpgmap(SCP_LOC, savepmap);
		}
		if (iscp->ieis_busy == 1)
			continue;	/* Continue loop until we get it */

		/*
		 * Now try to run a few simple commands before we say "OK".
		 */
		bzero((caddr_t)iad, sizeof (struct ieiaddr));
		iad->ieia_cb.iec_cmd = IE_IADDR;
		myetheraddr((struct ether_addr *)iad->ieia_addr);
		if (iesimple(es, &iad->ieia_cb)) {
			printf("ie: hang while setting Ethernet address.\n");
			continue;
		}

		iedefaultconf(ic);
		if (iesimple(es, &ic->ieconf_cb)) {
			printf("ie: hang while setting chip config.\n");
			continue;
		}

		/*
		 * Take the Ethernet interface chip out of loopback mode, i.e.
		 * put us on the wire.  We can't do this before having
		 * initialized the Ethernet chip because the chip does random
		 * things if its 'wire' is active between the time it's reset
		 * and the first CA.  Also, the IA-setup and configure commands
		 * will hang under some conditions unless the interface is
		 * very quiet and still.
		 */
		if (es->es_type == IE_OB)
			obie->obie_noloop = 1;
		if (es->es_type == IE_MB)
			mie->mie_noloop = 1; 
		if (ierustart(es)) {
			printf("ie: hang while starting receiver.\n");
			continue;
		}
			
		return 0;		/* It all worked! */
	}

	/* We tried a bunch of times, no luck. */
	printf("ie: cannot initialize\n");
	return 1;
}

/*
 * Initialize and start the Receive Unit
 */
int
ierustart(es)
	register struct ie_softc *es;
{
	register struct ierbd *rbd = &es->es_rbd;
	struct ierbd *rbd2 = &es->es_rbd2;
	register struct ierfd *rfd = &es->es_rfd;
	register struct iescb *scb = &es->es_scb;
	int timebomb = TIMEBOMB;

	/*
	 * Stop the RU, since we're sharing buffers with it.
	 * Then wait until it has really stopped.
	 */
	while (scb->ie_rus == IERUS_READY) {
		while (scb->ie_cmd != 0)	/* XXX */
			if (timebomb-- <= 0) return TIMEOUT;
		scb->ie_cmd = IECMD_RU_ABORT;
		ieca(es);
	}

	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;

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
	return OK;
}

ieca(es)
	register struct ie_softc *es;
{
	if (es->es_type == IE_OB) {
		es->es_obie->obie_ca = 1;
		es->es_obie->obie_ca = 0;
	} else {
		es->es_mie->mie_ca = 1;
		es->es_mie->mie_ca = 0;
	}
}

int
iesimple(es, cb)
	register struct ie_softc *es;
	register struct iecb *cb;
{
	register struct iescb *scb = &es->es_scb;
	register timebomb = TIMEBOMB;

	*(short *)cb = 0;	/* clear status bits */
	cb->iec_el = 1;
	cb->iec_next = 0;

	/* start CU */
	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	scb->ie_cbl = to_ieoff(es, (caddr_t)cb);
	scb->ie_cmd = IECMD_CU_START;
	if (scb->ie_cx)
		scb->ie_cmd |= IECMD_ACK_CX;
	if (scb->ie_cnr)
		scb->ie_cmd |= IECMD_ACK_CNR;
	ieca(es);
	while (!cb->iec_done)		/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	if (scb->ie_cx)
		scb->ie_cmd |= IECMD_ACK_CX;
	if (scb->ie_cnr)
		scb->ie_cmd |= IECMD_ACK_CNR;
	ieca(es);
	return OK;
}

/*
 * Transmit a packet.
 * Always copy the packet for the sake of Multibus
 * boards and Sun3's.  This is not a performance critical situation
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
	bcopy(buf, es->es_xbuf, count);
	tbd->ietbd_buf = to_ieaddr(es, es->es_xbuf);
	td->ietfd_tbd = to_ieoff(es, (caddr_t)tbd);
	td->ietfd_cmd = IE_TRANSMIT;
	if (iesimple(es, (struct iecb *)td)) {
		printf("ie: xmit hang\n");
		return -1;
	}
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



int
iepoll(es, buf)
	register struct ie_softc *es;
	char *buf;
{
	register struct ierbd *rbd = &es->es_rbd;
	register struct ierfd *rfd = &es->es_rfd;
	register struct iescb *scb = &es->es_scb;
	int len;
	int timebomb = TIMEBOMB;
	
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
		if (timebomb-- <= 0) return TIMEOUT;
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
 * For Carrera, the chip can access the top 16MB of virtual memory,
 * but we never give it addresses outside that range, so the high
 * order bits can be ignored.
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

#ifdef SUN3
#ifdef DEBUG
	if (cp < (char *)0x0F000000 || cp >= (char *)0x10000000) {
		/* printf("Bad ptr to_ieaddr(%x)\n", cp); */
		;  asm(" .word 0xFFFF") ; ;
	}
#endif DEBUG
#endif SUN3
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
	ic->ieconf_cb.iec_cmd = IE_CONFIG;
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
	register struct ie_softc *es = (struct ie_softc *) sip->si_devdata;

	if (es->es_type == IE_OB)
		*es->es_obie = obie_reset;
}
