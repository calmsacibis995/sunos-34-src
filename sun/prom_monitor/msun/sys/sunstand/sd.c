#ifndef SDBOOT
#ifndef lint
static	char sccsid[] = "@(#)sd.c	1.1 84/12/21	Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "saio.h"
#include "sasun.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../sundev/screg.h"

u_int sdstd[] = { (u_int)SCSI_BASE, (u_int)(MBMEM_BASE+0x84000), 0 };

struct sdparam {
	int	sd_target;
	int	sd_unit;
	int	sd_boff;
	struct scsi_ha_reg *sd_har;
};

/*
 * Record an error message from scsi
 */
#if (!defined(SDBOOT)) & !defined(STBOOT)

#define	sc_error(msg)	scerrmsg = msg
extern char *scerrmsg;

#else

extern sc_error();	/* It's in sc.c */

#endif

#ifndef SDBOOT
struct sdparam sdparam[32];
#endif SDBOOT

#define	DEVDATA		((char *) (MBMEM_BASE + 0x200))
#define SDBUF		((char *) (MBMEM_BASE + DMADDR))
#define label		((struct dk_label *) SDBUF)

#define SECSIZE	512

#ifdef SDBOOT
/*
 * Determine existence of SCSI host adapter.
 * Note that this doesn't check for existence of the disk controller.
 * If it's not there, we'll fail trying to open it.
 */
sdprobe()
{
	struct scsi_ha_reg *har;
	int i;

	for (i = 0; i < ((sizeof sdstd)/sizeof *sdstd) - 1; i++) {
		har = (struct scsi_ha_reg *)sdstd[i];
		if (peek(&har->dma_count) == -1)
			continue;
		har->dma_count = 0x6789;
		if (har->dma_count != 0x6789)
			continue;
		return i;
	}
	return -1;
}


/*
 * Test routine for isspinning() to see if SCSI disk is running.
 */
sdspin(sdp)
	register struct sdparam *sdp;
{
	return(sdcmd(SC_TEST_UNIT_READY, sdp, 0, 0, (char *) 0, 0));
}

#endif SDBOOT


/*
 * Open the SCSI Disk controller
 */
sdopen(sip)
	register struct saioreq *sip;
{
	register struct sdparam *sdp;
	register short *sp, sum;
	int count;

#ifdef SDBOOT
	sdp = (struct sdparam *)DEVDATA;
#else  SDBOOT
	/* FIXME: This won't work with two scsi boards both using unit 0 */
	sdp = &sdparam[sip->si_unit];
#endif SDBOOT
	bzero( (char *)sdp, (sizeof (struct sdparam)));
	sip->si_devdata = (char *)sdp;
	sdp->sd_target = sip->si_unit >> 2;
	sdp->sd_unit = sip->si_unit & 0x03;
	if (sip->si_ctlr < ((sizeof sdstd)/sizeof *sdstd) - 1)
		sdp->sd_har = 
		    (struct scsi_ha_reg *) sdstd[sip->si_ctlr];
	else
		sdp->sd_har = 
		    (struct scsi_ha_reg *) (MBMEM_BASE + sip->si_ctlr);
	sdp->sd_har->icr = 0;
#ifdef RESETIT
	sdp->sd_har->icr = ICR_RESET;
	DELAY(50);
	sdp->sd_har->icr = 0;
	DELAY(1000000);
#endif RESETIT
	label->dkl_magic = 0;
#ifdef SDBOOT 
	/* PROM has a helper routine for testing disk ready */
	switch (isspinning(sdspin, sdp, 0)) {

	case 0:
		/* isspinning has already printed "Giving up..." */
		return(-1);

	case 1:
		break;

	default:
		DELAY(1000000);  /* one second delay after spinup */
	}
#endif SDBOOT
	if (sdcmd(SC_READ, sdp, 0, SECSIZE, SDBUF, 1) == 0) {
		return(-1);
	}
	/* compute checksum */
	sum = 0;
	count = sizeof (struct dk_label) / sizeof (short);
	sp = (short *) label;
	while (count--) {
		sum ^= *sp++;
	}
	if (sum != 0) {
		printf("sd: bad label\n");
		return(-1);
	}
	sdp->sd_boff = (unsigned short)(label->dkl_map[sip->si_boff].dkl_cylno)
		* (unsigned short)(label->dkl_nhead * label->dkl_nsect);
	return(0);
}

sdstrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	return( sdcmd(rw == WRITE ? SC_WRITE : SC_READ,
		(struct sdparam *)sip->si_devdata,
		(int) sip->si_bn, sip->si_cc, sip->si_ma, 1));
}

sdcmd(cmd, sdp, blkno, acount, buf, errprint)
	int cmd;
	register struct sdparam *sdp;
	int blkno, acount;
	register char *buf;
	int errprint;
{
	struct scsi_cdb cdb, scdb;
	struct scsi_scb scb, sscb;
	int retry, r, i, count;

	if (cmd == SC_WRITE && buf != SDBUF) {
		bcopy(buf, SDBUF, (unsigned)acount);
	}
	/* set up cdb */
	bzero((char *) &cdb, sizeof cdb);
	cdb.cmd = cmd;
	cdb.lun = sdp->sd_unit;
	blkno += sdp->sd_boff;
	cdbaddr(&cdb, blkno);
	count = (acount + SECSIZE -1) & ~(SECSIZE-1);
	cdb.count = count / SECSIZE;
	retry = 0;
	do {
		r = scdoit(&cdb, &scb, count, SDBUF, sdp->sd_har,
			   1 << sdp->sd_target);
		if (scb.chk) {
			bzero((char *) &scdb, sizeof scdb);
			scdb.cmd = SC_REQUEST_SENSE;
			scdb.lun = sdp->sd_unit;
			scdb.count = sizeof (struct scsi_sense);
			i = scdoit(&scdb, &sscb, scdb.count, SDBUF, 
				sdp->sd_har, 1 << sdp->sd_target);
			if (i >= 4) {	/* all the sense Adaptec gives us */
#ifdef SDBOOT
				if (errprint) sd_pr_sense(SDBUF, i);
#endif SDBOOT
				continue;
			} else {	/* can't get sense, give up */
				if (errprint) {
					printf("sd: scsi %s\n",
#ifndef SDBOOT
					scerrmsg ? scerrmsg :
#endif SDBOOT
					"sense failed");
				}
				return(0);
			}
		} else if (scb.busy) {
			sc_error("disk busy");
			DELAY(100000);
			continue;
		}
		if (r != count) {
#ifdef SDBOOT
			if (errprint)
				printf("sd: dma count is %d wanted %d\n",
					r, count);
#endif SDBOOT
			continue;
		}
		if (cmd == SC_READ && buf != SDBUF) {
			bcopy(SDBUF, buf, (unsigned)acount);
		}
#ifndef SDBOOT
		scerrmsg = 0;
#endif SDBOOT
		return (count ? count : 1);
	} while (retry++ < 16);
#ifndef SDBOOT
	if (errprint) {
		if (scb.chk) {
			sd_pr_sense(SDBUF, i);
		} else if (scerrmsg) {
			printf("sd: scsi %s\n", scerrmsg);
		} else if (r != count) {
			printf("sd: dma count is %d wanted %d\n", r, count);
		} else {
			/* we should never get this far */
			printf("sd: retry count exceeded\n");
		}
	}
#endif SDBOOT
	return(0);
}

#ifdef SDERRORS
char	*class_00_errors[] = {
	"No sense",
	"No index signal",
	"No seek complete",
	"Write fault",
	"Drive not ready",
	"Drive not selected",
	"No track 00",
	"Multiple drives selected",
	"No address acknowledged",
	"Media not loaded",
	"Insufficient capacity",
};

char	*class_01_errors[] = {
	"I.D. CRC error",
	"Unrecoverable data error",
	"I.D. address mark not found",
	"Data address mark not found",
	"Record not found",
	"Seek error",
	"DMA timeout error",
	"Write protected",
	"Correctable data check",
	"Bad block found",
	"Interleave error",
	"Data transfer incomplete",
	"Unformatted or bad format on drive",
	"Self test failed",
	"Defective track (media errors)",
};

char	*class_02_errors[] = {
	"Invalid command",
	"Illegal block address",
	"Aborted",
	"Volume overflow",
};

char	**sc_errors[] = {
	class_00_errors,
	class_01_errors,
	class_02_errors,
	0, 0, 0, 0,
};

int	sc_errct[] = {
	sizeof class_00_errors / sizeof (char *),
	sizeof class_01_errors / sizeof (char *),
	sizeof class_02_errors / sizeof (char *),
	0, 0, 0, 0,
};

char	*sc_sense7_keys [] = {
	"No sense",
	"Recoverable error",
	"Not ready",
	"Media error",
	"Hardware error",
	"Illegal request",
	"Media change",
	"Write protect",
	"Diagnostic unique",
	"Vendor unique",
	"Power up failed",
	"Aborted command",
	"Equal",
	"Volume overflow",
};
#endif SDERRORS

/*
 * Print out sense info.
 */
sd_pr_sense(sp, len)
	unsigned char *sp;
	int len;
{

#ifndef SDERRORS
	printf("sd: error");
	while (--len >= 0) {
		printf(" %x", *sp++);
	}
	printf("\n");
#else  SDERRORS
	printf("sd: ");
	if (((struct scsi_sense *)sp)->class <= 6) {
		register struct scsi_sense *sense;

		sense = (struct scsi_sense *) sp;
		if (sense->code < sc_errct[sense->class]) {
			printf("%s", sc_errors[sense->class][sense->code]);
		} else {
			printf("error %x", *sp);
		}
		if (sense->adr_val) {
			printf(" block no. %d", (sense->high_addr << 16) |
				(sense->mid_addr << 8) | sense->low_addr);
		}
	} else {	/* Sense class 7: the standardized one */
		register struct scsi_sense7 *sense7;

		sense7 = (struct scsi_sense7 *) sp;
		if (sense7->fil_mk) {
			printf("file mark ");
		}
		if (sense7->eom) {
			printf("end of medium ");
		}
		if (sense7->key 
		    < sizeof (sc_sense7_keys) / sizeof (sc_sense7_keys[0])) {
			printf("%s", sc_sense7_keys[sense7->key]);
		} else {
			printf("sense key %x", sense7->key);
		}
		printf(" block no. %x", (sense7->info_1 << 24) |
			(sense7->info_2 << 16) | (sense7->info_3 << 8) |
			sense7->info_4);
	}
	printf("\n");
#endif SDERRORS
}
