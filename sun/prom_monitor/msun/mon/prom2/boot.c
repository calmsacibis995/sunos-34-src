/*
 * @(#)boot.c 1.15 84/06/07 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifdef MAKE_DEPEND
#include "Makefile"
#endif

#include "../h/bootparam.h"
#include "../h/sunromvec.h"

/*
 * no_probe is used as the probe routine address if the device is
 * not to be booted by default.
 */
int	no_probe();

#ifdef XYBOOT
int	xyprobe(), xyboot();
#endif

#ifdef DDBOOT
int	ddprobe(), ddboot();
#endif

#ifdef IPBOOT
int	ipprobe(), ipboot();
#endif

#ifdef ECBOOT
int	xxboot();
int	ecprobe(), ecopen(), ndstrategy();
#endif

#ifdef IEBOOT
int	xxboot();
int	ieprobe(), ieopen(), ndstrategy(), ieclose();
#endif

#ifdef MTBOOT
int	mtboot();
#endif

#ifdef ARBOOT
int	arboot();
#endif

#ifdef SDBOOT
int	xxboot();
int	sdprobe(), sdopen(), sdstrategy();
#endif

#ifdef STBOOT
int	ttboot();
int	stopen(), ststrategy(), stclose();
#endif

#ifdef RSBOOT
int	rsboot();
#endif 

#ifdef XTBOOT
int	ttboot();
int	xtopen(), xtstrategy(), xtclose();
#endif

#define NONE	no_probe	/* Filler for nonexistent routines */

struct boottab boottab[] = {
#ifdef XYBOOT
	"xy",	xyprobe,	xyboot,	NONE,	NONE,		NONE,
	"xy: Xylogics disk",
#endif
#ifdef DDBOOT
	"dd",	ddprobe,	ddboot,	NONE,	NONE,		NONE,
	"dd: DSD 5-1/4 inch disk",
#endif
#ifdef IPBOOT
	"ip",	ipprobe,	ipboot,	NONE,	NONE,		NONE,
	"ip: Interphase disk",
#endif
#ifdef SDBOOT
	"sd",	sdprobe,	xxboot,	sdopen,	NONE,		sdstrategy,
	"sd: SCSI 5-1/4 disk",
#endif
#ifdef IEBOOT
	"ie",	ieprobe,	xxboot,	ieopen,	ieclose,	ndstrategy,
	"ie: Sun/Intel network disk",
#endif
#ifdef ECBOOT
	"ec",	ecprobe,	xxboot,	ecopen,	NONE,		ndstrategy,
	"ec: 3Com network disk",
#endif
#ifdef MTBOOT
	"mt",	no_probe,	mtboot,	NONE,	NONE,		NONE,
	"mt: 9-track tape",
#endif
#ifdef XTBOOT
	"xt",	no_probe,	ttboot,	xtopen,	xtclose,	xtstrategy,
	"xt: Xylogics 472 tape",
#endif
#ifdef STBOOT
	"st",	no_probe,	ttboot,	stopen,	stclose,	ststrategy,
	"st: SCSI tape",
#endif
#ifdef ARBOOT
	"ar",	no_probe,	arboot,	NONE,	NONE,		NONE,
	"ar: 1/4 inch tape",
#endif
#ifdef RSBOOT
	"ra",	no_probe,	rsboot,	NONE,	NONE,		NONE,
	"ra: RS232 port a",
	"rb",	no_probe,	rsboot,	NONE,	NONE,		NONE,
	"rb: RS232 port b",
#endif
	{0,0},	NONE,		NONE,	NONE,	NONE,		NONE,
	0,
};

#define	skipblank(p)	{ while (*(p) == ' ') (p)++; }

boot(cmd)
	char *cmd;
{
	register struct boottab *tp;
	register struct bootparam *bp = *romp->v_bootparam;
	register char *dev = 0, *name;
	register char *p, *q;
	char *gethex(), *puthex();
	register int i;

	if (*cmd == '?') goto syntax;
	for (p = cmd; *p != 0 && *p != ' ' && *p != '('; p++)
		;
	q = p;
	skipblank(p);
	if (*p == '(') {	/* device specified */
		p++;
		if (q > cmd+2)
			goto syntax;
		*q = 0;
		for (tp = boottab; tp->b_dev[0]; tp++) {
			if (cmd[0] == tp->b_dev[0] && cmd[1] == tp->b_dev[1]) {
				dev = cmd;
				break;
			}
		}
		if (dev == 0) goto syntax;
		p = gethex(p, &bp->bp_ctlr);
		p = gethex(p, &bp->bp_unit);
		p = gethex(p, &bp->bp_part);
		if (*p != 0 && *p != ')')
			goto syntax;
	} else {		/* default boot */
		p = cmd;
		for (tp = boottab; tp->b_dev[0]; tp++) {
			bp->bp_ctlr = (*tp->b_probe)();
			if (bp->bp_ctlr != -1) {
				dev = tp->b_dev;
				break;
			}
		}
		if (dev == 0) {
			printf("No default boot devices\n");
			return (-1);
		}
		bp->bp_unit = 0;
		bp->bp_part = 0;
	}
	if (*p == ')')
		p++;
	skipblank(p);
	if (*p == 0 || *p == '-') {
		if (tp->b_probe != no_probe)	/* default boot */
			name = "vmunix";
		else
			name = "";
	} else {
		name = p;
		while (*p != 0 && *p != ' ')
			p++;
		if (*p == ' ') {
			*p = 0;
			p++;
			skipblank(p);
		}
	}
	printf ("Boot: %c%c(%x,%x,%x)%s %s\n",
	    dev[0], dev[1], bp->bp_ctlr, bp->bp_unit, bp->bp_part, name, p);

	/* Put in bootparam */
	bp->bp_dev[0] = dev[0];
	bp->bp_dev[1] = dev[1];
	bp->bp_boottab = tp;
	q = bp->bp_strings;
	*q++ = dev[0];
	*q++ = dev[1];
	*q++ = '(';
	q = puthex(q, bp->bp_ctlr);
	*q++ = ',';
	q = puthex(q, bp->bp_unit);
	*q++ = ',';
	q = puthex(q, bp->bp_part);
	*q++ = ')';
	bp->bp_name = q;
	while (*q++ = *name++)
		;
	bp->bp_argv[0] = bp->bp_strings;
	for (i = 1; i < (sizeof bp->bp_argv/sizeof bp->bp_argv[0])-1;) {
		skipblank(p);
		if (*p == '\0')
			break;
		bp->bp_argv[i++] = q;
		while (*p != '\0' && *p != ' ')
			*q++ = *p++;
		/* SHOULD CHECK RANGE OF q */
		*q++ = '\0';
	}
	bp->bp_argv[i] = (char *)0;
	return ((*tp->b_boot)(bp));

syntax:
	printf ("Boot syntax: b [!][dev(ctlr,unit,part)] name [options]\n\
Possible boot devices:\n");
	for (tp = boottab; tp->b_dev[0]; tp++)
		printf("  %s\n", tp->b_desc);
	return (-1);
}

char *
gethex(p, ip)
	register char *p;
	int *ip;
{
	register int ac = 0;

	skipblank(p);
	while (*p) {
		if (*p >= '0' && *p <= '9')
			ac = (ac<<4) + (*p - '0');
		else if (*p >= 'a' && *p <= 'f')
			ac = (ac<<4) + (*p - 'a' + 10);
		else if (*p >= 'A' && *p <= 'F')
			ac = (ac<<4) + (*p - 'A' + 10);
		else
			break;
		p++;
	}
	skipblank(p);
	if (*p == ',')
		p++;
	skipblank(p);
	*ip = ac;
	return (p);
}

char *
puthex(p, n)
	register char *p;
	register int n;
{
	register int a;

	if (a = ((unsigned)n >> 4))
		p = puthex(p, a);
	*p++ = "0123456789abcdef"[n & 0xF];
	return (p);
}
 
no_probe()
{
	return (-1);
}
