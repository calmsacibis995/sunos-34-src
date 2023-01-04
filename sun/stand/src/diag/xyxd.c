#ifndef lint
static	char sccsid[] = "@(#)xyxd.c 1.1 86/09/25";
#endif

#include "diag.h"
#include <sys/types.h>
#include <sundev/xyreg.h>
#include <sundev/xdreg.h>
#include "xyxd.h"

/*
 * Total # of sectors including runts and spares (hardware controlled).
 * This value is initialized when a STATUS command is issued.
 * (for smd disks)
 */
int hsect;

static u_char rev;
extern u_char xyrev;

C_rhdr()
{

	printf("read track header\nstarting ");
	if (controller == C_XY450 || controller == C_XY751)
	    (void) do_rhdr(1);
	else
	    printf("\rMust have Xylogics 450/451 controller!\n");
}

int
do_rhdr(loop)
	register int loop;
{
	register int cyl, head, bn;

	for (;;) {
		bn = pgetbn("track (dd/dd): ") - nsect;
		if ((bn % nsect) == 0)
			break;
		printf("Must be on track boundary!\n");
	}

	do {
		bn += nsect;

		cyl = bn / (nhead * nsect);
		head = (bn % (nhead * nsect)) / nsect;

		if (do_readhdr(cyl, head))
			return -1;

		hdr_print((int *)DBUF_VA, cyl, head);
		if (loop)
			printf("stop? ");
	} while (loop && !confirm());
	return (bn);
}

/*
 * This routine simply reads a track headers and adjusts for the
 * differences in various PROM sets, leaving the headers in physical
 * index form.  Uses DBUF area for doing its work.
 */
int
do_readhdr(cyl, head)
	int cyl, head;
{
	register int *dp = (int *)DBUF_VA;
	register i, lsec;

	if (hsect < nsect) {
		printf("\nSorry, cannot execute this command without\n");
		printf("first successfully executing a status command.\n");
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}

	bzero((char *)dp, hsect * sizeof (*dp));
	if (controller == C_XY450) {
	    rev = xyrev;
	    if (XYexec(XY_READHDR, cyl, head, 0, hsect, DBUF_PA))
		    return -1;
	} else if (controller == C_XY751) {
	    if (XDexec(XD_REXT, cyl, head, 0, hsect, DBUF_PA, XD_THEAD))
		    return -1;
	    return(0);
	}

	/*
	 * Rev C and beyond will all use physical index
	 * form and require no fix ups (at least that's
	 * what Xylogics's told me).  If the head is 0
	 * then pseudo index form same as physical index
	 * form, nothing to worry about.
	 */
	if (rev >= 'C' || head == 0)
		return 0;

	/*
	 * Rev A and B are known to use pseudo index form,
	 * so we must put in physical index form.
	 */
	if (rev == 'A' || rev == 'B') {
		to_physical(dp, head);
		return 0;
	}

	/*
	 * Unknown version, find out the hard way what's going on.
	 * Xylogics special PROM sets will be Rev `@'.  Too bad we
	 * can't tell if it's a B+ or a C+ PROM from this.
	 */
	for (lsec = i = 0; i < hsect; i++) {
		if (dp[i] == HDR_RUNT || dp[i] == HDR_SLIP)
			continue;
		if (dp[i] == HDR_ZAP || dp[i] == HDR_SPARE) {
			lsec++;
			continue;
		}
		if (lsec == XY_GET_SEC((struct xyhdr *)&dp[i])) {
			/*
			 * Oops, some old version (like B12) which reads
			 * using pseudo index form.  Convert it.
			 */
			to_physical(dp, head);
		}
		return 0;
	}
	return -1;		/* We're in trouble if we get here! */
}

hdr_print(dp, cyl, head)
register int *dp, cyl, head;
{
	register i;

	printf("%d/%d", cyl, head);
	for (i = 0; i < hsect; i++) {
		if ((i % 8) == 0)
			printf("\nsec %d:\t", i);
		printf("%x ", dp[i]);
	}
	putchar('\n');
}

int
read_slip(cyl, head, res)
	register int *res;
{
	register int *dp = (int *)DBUF_VA;
	register int i, logical;

	if (do_readhdr(cyl, head))
		return -1;

	/*
	 * Put array into pseudo index form
	 */
	if (controller == C_XY450)
	    to_pseudo(dp, head);

	/*
	 * Look through the physical sector headers for slips.
	 * `logical' contains the logical sector number which
	 * should be at the current physical location
	 * (taking into account any previous slips).
	 */
	for (i = logical = 0; i < hsect; i++) {
		if (dp[i] == HDR_RUNT) {
			continue;		/* onto the next */
		} else if (dp[i] == HDR_SLIP) {
			*res++ = logical;	/* add to the list */
		} else if (++logical >= nsect) {
			break;			/* end of sectors to look at */
		}
	}

	*res = -1;				/* terminate the list */
	return 0;
}

#define SSLIP		0
#define SSPARE		1
#define SZAP		2
#define SRUNT		3
#define BADHDR		4
#define GOODHDR		5

/*
 * We insist on an interleave of 1 to perform a slip sector
 * because of the simple mapping algorithm used.
 */
int
slipzap(cyl, head, sec, hdr)
{
	register int *dp = (int *)DBUF_VA;
	register int i, nsec, *ip;
	int o = -1;
	int orig[256];
	int gotit = 0;

	if (do_readhdr(cyl, head))
		return -1;

	if (interleave != 1)
		return -1;
 
	for (i = 0; i < hsect; i++) {
		orig[i] = dp[i];
		if (orig[i] == HDR_SPARE)
			o = i;
	}

	if (o == -1)
	    if (hdr == HDR_SLIP)
		return -1;		/* No spare available */
	/*
	 * Put the orig array to put in psuedo index form
	 * to make construction of the new array in pseudo
	 * index form easier.
	 */
	if (controller == C_XY450)
	    to_pseudo(orig, head);

	/*
	 * Now construct the new track headers in pseudo index form
	 * (putting logical sector 0 in the first available location).
	 * If we find a previously slipped (HDR_SLIP) or the logical
	 * sector we are looking for we mark the physical slot as slipped.
	 * Else if we find that we have a runt sector (HDR_RUNT) we mark
	 * the slot as a runt.  Else if we find a sector that was zapped
	 * (HDR_ZAP), we leave it zapped but pretend that we used the
	 * next logical sector.  Otherwise the sector is usable.  If we
	 * used up all the logical sectors then the slot is marked as a
	 * spare (HDR_SPARE).  Otherwise we build the next logical sector
	 * entry in place and increment the logical sector count.
	 */
	bzero((char *)dp, hsect * sizeof (*dp));
	for (nsec = 0, ip = dp; ip < &dp[hsect]; ip++) {
		o = orig[ip - dp];
		switch (i = analyse(o)) {
		    case SSLIP:
			*ip = HDR_SLIP;
			break;

		    case SRUNT:
			*ip = HDR_RUNT;
			break;

		    case SZAP:
			*ip = HDR_ZAP;
			nsec++;
			break;

		    default:
			if (sec == nsec && !gotit) {
			    *ip = hdr;
			    if (hdr == HDR_ZAP)
				nsec++;
			    gotit = 1;
			} else if (nsec >= nsect) {
			    if (sec == nsec)
				*ip = HDR_ZAP;
			    else
				*ip = HDR_SPARE;
			    nsec++;
			} else {
			    if (i == BADHDR)	/* helpless case here */
				printf("Warning! unrecognized sector header\n");
			    
			    if (controller == C_XY450)
				xyhdrfill((struct xyhdr *) ip, cyl, head, nsec);
			    if (controller == C_XY751)
				xdhdrfill((struct xdhdr *) ip, cyl, head, nsec);
			    nsec++;
			}
			break;
		}
	}

	/*
	 * Now we convert back to physical index form since the WRITEHDR
	 * command starts at the physical index, not the pseudo index.
	 */
	if (controller == C_XY450)
	    to_physical(dp, head);

	/* Check to make sure we were able to map everything in ok */
	if (nsec < (nsect -1))
		return -1;
	else {
		if (slipmsgs) {
			to_physical(orig, head);
			printf("original track headers for ");
			hdr_print(orig, cyl, head);
			printf("new track headers for ");
			hdr_print(dp, cyl, head);
		}
	}

	if (controller == C_XY450)
	    return XYexec(XY_WRITEHDR, cyl, head, 0, hsect, DBUF_PA);
	else		/*  controller == C_XY751 */
	    return XDexec(XD_WEXT, cyl, head, 0, hsect, DBUF_PA, XD_THEAD);
}

/*
 * this routine looked for slipped, mapped and runt header
 */
int
analyse(hdr)
    int hdr;
{
    u_int i = 0;

    /*
     * we have the following byte mapping between the 2 headers:
     * 		xy byte 0 -> xd byte 1, xy byte 1 -> xd byte 0, 
     *		xy byte 2 -> xd byte 3, xy byte 3 -> xd byte 2
     */
    if (controller == C_XY751) {
	i = i | ((hdr >> 8) & 0xff00ff);
	i = i | ((hdr << 8) & 0xff00ff00);
	hdr = i;
    }
	

    switch (hdr) {
	case HDR_SLIP:
	    return(SSLIP);
	case HDR_RUNT:
	    return(SRUNT);
	case HDR_SPARE:
	    return(SSPARE);
	case HDR_ZAP:
	    return(SZAP);
	default:
	    switch (hdr & 0xff) {
		case 0xfe:
		    return(SSLIP);
		case 0xdd:
		    return(SSPARE);
		case 0xff:
		    return(SZAP);
		case 0xee:
		    return(SRUNT);
		default:
		    switch ( (hdr & 0xff000000) >> 24 ) {
			case 0xfe:
			    return(SSLIP);
			case 0xdd:
			    return(SSPARE);
			case 0xff:
			    return(SZAP);
			case 0xee:
			    return(SRUNT);
			default:
			    if (hdr & 0x38000000) {
				switch ( (hdr & 0xffff00) >> 8 ) {
				    case 0xfefe:
					return(SSLIP);
				    case 0xdddd:
					return(SSPARE);
				    case 0xffff:
					return(SZAP);
				    case 0xeeee:
					return(SRUNT);
				    default:
					return(BADHDR);
				}
			    } else
				return(GOODHDR);
		    }
	    }
    }
}
