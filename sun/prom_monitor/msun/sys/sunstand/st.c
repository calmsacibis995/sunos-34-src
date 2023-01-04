#ifndef STBOOT
#ifndef lint
static	char sccsid[] = "@(#)st.c	1.15 85/02/19	Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "saio.h"
#include "sasun.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../sundev/screg.h"
#include "../sundev/streg.h"

extern int scdoit();
#define min(a,b)	((a)<(b)? (a): (b))

u_int ststd[] = { (u_int)SCSI_BASE, (u_int)(MBMEM_BASE+0x84000), 0 };

#define	TAPE_TARGET		4	/* default SCSI target # for tape */

#define SENSE_BITS \
"\020\17NoCart\16NoDrive\15WriteProt\14EndMedium\13HardErr\12WrongBlock\
\11FileMark\7InvCmd\6NoData\5Flaking\4BOT\0034\0022\1GotReset"


struct stparam {
	int	st_target;
	int	st_unit;
	int	st_eof;
	int	st_lastcmd;
	struct scsi_ha_reg *st_har;
};

#ifndef STBOOT
struct stparam stparam[32];
#endif STBOOT

#define	DEVDATA		((char *) (MBMEM_BASE + 0x200))
#define STSBUF		((char *) (MBMEM_BASE + DMADDR))
#define STBUF		((char *) (MBMEM_BASE + DMADDR + SENSE_LENGTH))

#define SENSELOC	4	/* sysgen returns sense at this offset */

#define	ROUNDUP(x)	((x + DEV_BSIZE - 1) & ~(DEV_BSIZE - 1))

#ifndef STBOOT
extern char *scerrmsg;		/* Error msg set by scdoit */
#endif STBOOT


/*
 * Open the SCSI Tape controller
 */
int
stopen(sip)
	register struct saioreq *sip;
{
	register struct stparam *stp;
	int skip;

#ifdef STBOOT
	stp = (struct stparam *)DEVDATA;
#else  STBOOT
	if (sip->si_unit >= (sizeof(stparam)/sizeof(struct stparam)) ) {
		printf("st: bad unit number\n");
		return -1;
	}
	/* FIXME: This won't work with two scsi boards both using unit 0 */
	stp = &stparam[sip->si_unit];
#endif STBOOT
	bzero( (char *)stp, (sizeof (struct stparam)));
	sip->si_devdata = (char *)stp;
	stp->st_target = sip->si_unit >> 3;
	if (stp->st_target == 0) {
		stp->st_target = TAPE_TARGET;
	}
	stp->st_unit = sip->si_unit & 0x07;
	if (sip->si_ctlr < ((sizeof ststd)/sizeof *ststd) - 1)
		stp->st_har = 
		    (struct scsi_ha_reg *) ststd[sip->si_ctlr];
	else
		stp->st_har = 
		    (struct scsi_ha_reg *) (MBMEM_BASE + sip->si_ctlr);
	stp->st_har->icr = 0;
#ifdef RESETIT
	stp->st_har->icr = ICR_RESET;
	DELAY(50);
	stp->st_har->icr = 0;
	DELAY(1000000);
#endif RESETIT
	/*
	 * Rewind a few times until it works.  First one will fail if
	 * the SCSI bus is permanently busy if a previous op was interrupted
	 * in mid-transfer.  Second one will fail with POR status, after
	 * the scsi bus is reset from the busy.  Third one should work.  
	 */
	if (stcmd(SC_REWIND, stp, 0, (char *)0, 0) == 0 &&
	    stcmd(SC_REWIND, stp, 0, (char *)0, 0) == 0 &&
	    stcmd(SC_REWIND, stp, 0, (char *)0, 1) == 0) {
		return(-1);
	}

	/*
	 * Skip to specified file.
	 * If file >= 0x100, retension tape and do file%100.
	 */
	skip = sip->si_boff;
	if (skip >= 0x100) {
		if (stcmd(SC_RETENSION, stp, 0, (char *)0, 1) == 0)
			return(-1);
		skip -= 0x100;
	}
	while (skip--) {
		if (stcmd(SC_SPACE_FILE, stp, 0, (char *)0, 1) == 0) {
			return(-1);
		}
	}
	stp->st_eof = 0;
	stp->st_lastcmd = 0;
	return(0);
}

/*
 * Close the tape drive.
 */
stclose(sip)
	register struct saioreq *sip;
{
	register struct stparam *stp;

	stp = (struct stparam *) sip->si_devdata;
	if (stp->st_lastcmd == SC_WRITE) {
		(void) stcmd(SC_WRITE_FILE_MARK, stp, DEV_BSIZE, (char *) 0,
			0);
	}
	(void) stcmd(SC_REWIND, stp, 0, (char *) 0, 0);
}


/*
 * Perform a read or write of the SCSI tape.
 */
int
ststrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	register struct stparam *stp;

#ifdef DEBUG
	printf("ststrat: bn %d cc %d=0x%x ma %x\n", sip->si_bn,
		sip->si_cc, sip->si_ma);
#endif DEBUG
	stp = (struct stparam *) sip->si_devdata;
	if (stp->st_eof) {
#ifdef DEBUG
		printf("ststrat: eof\n");
#endif DEBUG
		stp->st_eof = 0;
		return(0);
	}
	return(stcmd(rw == WRITE ? SC_WRITE : SC_READ, stp,
		sip->si_cc, sip->si_ma, 1));
}


/*
 * Execute a scsi tape command
 */
int
stcmd(cmd, stp, count, buf, errprint)
	int cmd;
	register struct stparam *stp;
	int count;
	register char *buf;
	int errprint;
{
	struct scsi_cdb cdb, scdb;
	struct scsi_scb scb, sscb;
	register int r, i, c;

	if (cmd == SC_WRITE && buf != STBUF) {
		bcopy(buf, STBUF, (unsigned)count);
	}
	/* set up cdb */
	bzero((char *) &cdb, sizeof cdb);
	cdb.cmd = cmd;
	cdb.lun = stp->st_unit;
	c = ROUNDUP(count) / DEV_BSIZE;
	cdb.high_count = c >> 16;
	cdb.mid_count = (c >> 8) & 0xFF;
	cdb.low_count = c & 0xFF;
	if (cmd == SC_RETENSION) {
		/*
		 * Note cdb.cmd is already set, since low 8 bits of 
		 * SC_RETENSION is SC_REWIND.
		 */
		cdb.vu_57 = 1;		/* Set retension bit */
	}
	if (cmd == SC_SPACE_FILE) {
		cdb.cmd = SC_SPACE;	/* the real SCSI cmd */
		cdb.t_code = 1;		/* space file, not rec */
		cdb.low_count = 1;	/* space 1 file, not 0 */
	}
	r = scdoit(&cdb, &scb, ROUNDUP(count), STBUF, stp->st_har,
		   1 << stp->st_target);
	if (r == -1) {
#ifndef STBOOT
		if (errprint) {
			printf("st: scsi %s\n", scerrmsg);
		}
#endif STBOOT
		return(0);
	}
	if (cmd == SC_READ && buf != STBUF) {
		bcopy(STBUF, buf, (unsigned)(min(count, r)) );
	}
	if (scb.chk) {
		bzero((char *) &scdb, sizeof scdb);
		scdb.cmd = SC_REQUEST_SENSE;
		scdb.lun = stp->st_unit;
		scdb.count = SENSE_LENGTH;
		i = scdoit(&scdb, &sscb, SENSE_LENGTH, STSBUF,
			   stp->st_har, 1 << stp->st_target);
		if (i != SENSE_LENGTH) {
			if (errprint) {
				printf("st: sense error\n");
			}
			stp->st_eof = 1;
			return(0);
		} else if (((struct sc_sense *)STSBUF)->qic_sense.file_mark) {
			stp->st_eof = 1;
			return(r);
		} else {
			if (errprint) {
#ifdef STBOOT
				printf("st: error %x\n", 
					*(unsigned short *)&STSBUF[SENSELOC]);
#else  STBOOT
				printf("st: error %b\n", 
					*(unsigned short *)&STSBUF[SENSELOC],
					SENSE_BITS);
#endif STBOOT
			}
			stp->st_eof = 1;
			return(r);	/* Even w/funny sense, say how many */
		}
	}
	if (r >= count) {
		return(count ? count : 1);
	} else {
		if (errprint) {
			printf("st: short transfer\n");
		}
		return(r);
	}
}

