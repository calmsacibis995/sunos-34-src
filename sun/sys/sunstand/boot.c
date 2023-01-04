#ifndef lint
static	char sccsid[] = "@(#)boot.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "saio.h"
#include "../h/reboot.h"
#include "../mon/sunromvec.h"

/*
 * Boot program... argument from monitor determines
 * whether boot stops to ask for system name and which device
 * boot comes from.
 */

int (*readfile())();

char aline[100];

char vmunix[50] = "vmunix";	/* and room to patch to whatever */

main()
{
	register struct bootparam *bp;
	register char *arg;
	register int howto;
	register int io;
	int (*go2)();

	if (romp->v_bootparam == 0)
		bp = (struct bootparam *)0x2000;	/* Old Sun-1 address */
	else
		bp = *(romp->v_bootparam);		/* S-2: via romvec */

#ifdef JUSTASK
	howto = RB_ASKNAME|RB_SINGLE;
#else
	arg = bp->bp_argv[0];
	howto = bootflags(bp->bp_argv[1]);
	if ((howto&RB_ASKNAME)==0) {
		register char *s, *p;

		for (s = arg, p = aline; *s; s++, p++)
			*p = *s;
		
		/*
		 * If no file name given, default to "vmunix"
	  	 * We only do this if we aren't asking for input
		 */
		for (s = aline; *s;)
			if (*s++ == ')')
				break;
		if (*s == '\0') {
			for (p = vmunix; *p;)
				*s++ = *p++;
			*s = '\0';
		}
	}
#endif
	for (;;) {
		if (howto & RB_ASKNAME) {
			printf("Boot: ");
			gets(aline);
			if (*aline == 0)
				continue;	/* Null input - try again */
			parseparam(aline, howto, bp);
		} else
			printf("Boot: %s\n", aline);
		io = open(aline, 0);
		if (io >= 0) {
			go2 = readfile(io, 1);
			if (!(howto & RB_ASKNAME))  {
				_exitto(go2);
				/*NOTREACHED*/ ;
			}
			if ((int)go2 != -1)
				(*go2)(arg);
		} else {
			printf("boot failed\n");
			if (!(howto & RB_ASKNAME))
				_stop((char *) 0);
		}
	}
}


struct bootf {
	char	let;
	short	bit;
} bootf[] = {
	'a',	RB_ASKNAME,
	's',	RB_SINGLE,
	'i',	0,
	'h',	RB_HALT,
	'b',	RB_NOBOOTRC,
	'd',	RB_DEBUG,
	0,	0,
};

#ifndef JUSTASK
/*
 * Parse the boot line to determine boot flags 
 */
bootflags(cp)
	register char *cp;
{
	register int i, boothowto = 0;

	if (*cp++ == '-')
	do {
		for (i = 0; bootf[i].let; i++) {
			if (*cp == bootf[i].let) {
				boothowto |= bootf[i].bit;
				break;
			}
		}
		cp++;
	} while (bootf[i].let && *cp);
	return (boothowto);
}
#endif
 
/*
 * Parse the boot line and put it in bootparam.  Stuff in -a -s or -as if
 * s/he only typed one argument and if they were in effect before.
 */
parseparam(line, defaults, bp)
	register char *line;
	int defaults;
	register struct bootparam *bp;
{
	register int narg = 0, i;
	register char *cp = bp->bp_strings;

	while(*line) {
		if (narg == 1)		/* terminate line for open */
			*line++ = 0;
		while (*line == ' ')
			line++;
		if (*line)
			bp->bp_argv[narg++] = cp;
		while (*line != ' ' && *line != 0)
			*cp++ = *line++;
		*cp++ = 0;
	};
	bp->bp_argv[narg] = 0;
	/*
	 * Stuff in default switches
	 */
	if (narg == 1 && defaults) {
		bp->bp_argv[narg++] = cp;
		*cp++ = '-';
		for (i = 0; bootf[i].let; i++) {
			if (defaults & bootf[i].bit)
				*cp++ = bootf[i].let;
		}
		*cp = 0;
		bp->bp_argv[narg] = 0;
	}
}
