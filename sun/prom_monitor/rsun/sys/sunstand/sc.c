#if (!defined(SDBOOT)) & !defined(STBOOT)
#ifndef lint
static	char sccsid[] = "@(#)sc.c	1.1 84/12/21	Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/types.h"
#include "../sun/param.h"
#include "../sundev/screg.h"

/*
 * Low-level routines common to all devices on the SCSI bus
 */

char seqerr_msg[] = "sequence error";	/* Saves dup copy of string */


/*
 * Record an error message from scsi
 */
#if (!defined(SDBOOT)) & !defined(STBOOT)

/* Running in RAM */
#define sc_error(msg)	scerrmsg = msg
char *scerrmsg;

#else

/* Running in PROM */

sc_error(msg)
	char *msg;
{
	printf("scsi: %s\n", msg);
}

#endif


/*
 * Write a command to the SCSI bus.
 */
int
scdoit(cdb, scb, len, dmaddr, har, target)
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	int len;
	char *dmaddr;
	register struct scsi_ha_reg *har;
	int target;
{
	register char *cp;
	register int i, b;

	/* select controller */
	har->data = target;
	i = 100000;
	for(;;) {
		if ((har->icr & ICR_BUSY) == 0) {
			break;
		}
		if (i-- <= 0) {
			har->icr = ICR_RESET;
			DELAY(50);
			har->icr = 0;
			sc_error("bus continuously busy");
			return(-1);
		}
	}
	har->icr = ICR_SELECT;
	if (sc_wait(har, ICR_BUSY) == 0) {
		/* FIXME: shouldn't we at least drop our SELECT strobe,
		 * and maybe reset the bus?  -- gnu 8Feb84
		 */
		sc_error("cannot select");
		return(-1);
	}
	/* pass command */
	har->icr = ICR_WORD_MODE | ICR_DMA_ENABLE;
	har->dma_addr = (int)dmaddr & 0xFFFFF;	/* strip DVMA high bits */
	har->dma_count = ~len;
	cp = (char *) cdb;
	for (i = 0; i < sizeof (struct scsi_cdb); i++) {
		if (sc_putbyte(har, ICR_COMMAND, *cp++) == 0) {
			return(-1);
		}
	}
	/* wait for command completion */
	if (sc_wait(har, ICR_INTERRUPT_REQUEST) == 0) {
		return(-1);
	}
	/* get status */
	cp = (char *) scb;
	i = 0;
	for (;;) {
		b = sc_getbyte(har, ICR_STATUS);
		if (b == -1) {
			break;
		}
		if (i < STATUS_LEN) {
			cp[i++] = b;
		}
	}
	b = sc_getbyte(har, ICR_MESSAGE_IN);
	if (b != SC_COMMAND_COMPLETE) {
		if (b >= 0) {	/* if not, sc_getbyte already printed msg */
			sc_error("invalid message");
		}
		return(-1);
	}
	return(len - ~har->dma_count);
}

/*
 * Wait for a condition on the scsi bus.
 */
int
sc_wait(har, cond)
	register struct scsi_ha_reg *har;
{
	register int icr, count;

	if (cond == ICR_INTERRUPT_REQUEST) {
		count = 10000000;
	} else {
		count = 100000;
	}
	while (((icr = har->icr) & cond) != cond) {
		if (--count <= 0) {
			sc_error("timeout");
			return(0);
		}
		if (icr & ICR_BUS_ERROR) {
			sc_error("bus error");
			return(0);
		}
		DELAY(100);
	}
	return(1);
}


/*
 * Put a byte into the scsi command register.
 */
int
sc_putbyte(har, bits, c)
	register struct scsi_ha_reg *har;
{
	if (sc_wait(har, ICR_REQUEST) == 0) {
		return(0);
	}
	if ((har->icr & ICR_BITS) != bits) {
		sc_error(seqerr_msg);
		return(0);
	}
	har->cmd_stat = c;
	return(1);
}

/*
 * Get a byte from the scsi command/status register.
 * <bits> defines the bits we want to see on in the ICR.
 * If <bits> is wrong, we print a message -- unless <bits> is ICR_STATUS.
 * This hack is because scdoit keeps calling getbyte until it sees a non-
 * status byte; this is not an error in sequence as long as the next byte
 * has MESSAGE_IN tags.  FIXME.
 */
int
sc_getbyte(har, bits)
	struct scsi_ha_reg *har;
{
	if (sc_wait(har, ICR_REQUEST) == 0) {
		return(-1);
	}
	if ((har->icr & ICR_BITS) != bits) {
		if (bits != ICR_STATUS) sc_error(seqerr_msg);	/* Hack hack */
		return(-1);
	}
	return(har->cmd_stat);
}

