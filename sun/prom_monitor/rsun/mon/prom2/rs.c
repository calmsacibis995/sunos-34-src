/*
 * @(#)rs.c 1.2 84/04/12 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Serial Line Loader -- invoked by "b ra"  or "b rb".
 *
 * Remainder of command line is the line to be sent to the host system.
 * 
 * We then accept "S-records" and respond with record counts and
 * 1-letter acknowledgements, until we receive any non-S command, which
 * terminates the transfer.
 */

#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/sasun.h"
#include "../h/bootparam.h"

extern unsigned char getone();
char getrecord();
int gethexbyte();

int
rsboot(bp)
	register struct bootparam *bp;
{
	/*
	Serial-line (RS232) boot.

	Sends the rest of the line to Uart A or B, receives Motorola style
	"S-records".  To avoid a "shouting match" with
	the host, we don't echo until we see a CSTLD ("start
	load" character -- a backslash).

	The computer at the other end of the serial line must be running
	a special program.  Just dumping S-records to us will not work.

	(We might want to make it work as an option.  Does it already?)
	(We might want to put in a CR after the response to help out IBM
	 and other stupid systems.)

	*/

	char InSave, OutSave, EchoSave;
	unsigned int userpc;
	int recount;
	register char **p;

	InSave = gp->g_insource;  /* Save for later "u" cmd */
	OutSave = gp->g_outsink;
	EchoSave = gp->g_echo;

	/* Determine which line to interact with. */
	if ('a' == bp->bp_dev[1])	gp->g_insource = INUARTA;
	else				gp->g_insource = INUARTB;
	gp->g_outsink = gp->g_insource;	/* Output to same place too */
	gp->g_echo = 0;

	putchar ('\r');		/* Poke the host */

	for (p = &bp->bp_argv[0]; 0 != *p; p++) {
		printf("%s ", *p);
	}
	putchar ('\r');		/* Terminate the command line */

	recount = 0;
	while (getchar() != '\\');  /* backslash */
	gp->g_echo = 1;

	while (putchar('>'), getline(), getone()&UPCASE != 'S') {

		/* let user know that good records are coming */
		if (!(recount&0x3)) {
			gp->g_outsink = OutSave;
			putchar('.');
			gp->g_outsink = gp->g_insource;
		}
		putchar(getrecord(&userpc, &recount));
	}

	/* Be done with it. */
	gp->g_insource = InSave;
	gp->g_outsink = OutSave;
	gp->g_echo = EchoSave;
	return userpc;

}

#define SREC_DATA	'2'	/* Data-carrying S-record */
#define	SREC_TRAILER	'8'	/* Trailer S-record */


/*
 * get an S-record.  Returns character used for handshaking.
 * pcadx is set to address part of S-record for non-data records.
 */
char getrecord(pcadx, pcount)
long *pcadx;
int *pcount;
{
	register int bytecount;
	register int x;
	register long adx;
	char rtype;
	int w;
	short checksum;
	unsigned char *savptr;

	printhex(*pcount, 2);

	/*
	 * Format of 'S' record:
	 *	S<type><count><address><datum>...<datum><checksum>
	 *	  digit  byte   3bytes  byte      byte    byte
 	 */

	rtype = getone();
	if ( (rtype != SREC_DATA) && (rtype != SREC_TRAILER))
	    return('T');

	bytecount = gethexbyte();
	/*
	 * checksum = 0; checksum += bytecount
	 */
	checksum = bytecount;	

	/* check to see that the bytecount agrees with line length */
	if ( (bytecount < 3) ||	(gp->g_linesize != (bytecount<<1)+4))
	    return('L');

	/* accumulate address - 3 bytes */
	adx = 0;
	for (w = 0; w++ < 3; ) {
		 adx = (adx<<8)|(x = gethexbyte());
		 checksum += x;
		 bytecount--;
	}

	/* verify checksum */
	savptr = gp->g_lineptr;	/* save line ptr for rescanning */
	for (x = bytecount; x > 0 ; x--)
		checksum += gethexbyte();

	if ((++checksum)&0xFF)
	    return('K');

	/* checksum worked - now rescan input and read data */
	if (rtype == SREC_DATA) {
		gp->g_lineptr = savptr;
		while (--bytecount) {	/* predecrement to ignore checksum */
			*(char *)adx++ = (char)gethexbyte();
		}
	} else *pcadx = adx;

	(*pcount)++;

	return('Y');
}

/*
 * gethexbyte.c
 *
 * common subroutines for Sun ROM monitor: read a byte in hex format
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 */

/* get one hex-coded byte */
int
gethexbyte()
{
	register int v = 0;
	
	v = ishex(getone())<<4;
	return(v|ishex(getone()));
}
