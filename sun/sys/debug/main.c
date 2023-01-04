#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/reboot.h"
#include "../stand/saio.h"
#include "../machine/reg.h"
#include "../debug/debugger.h"
#include <a.out.h>

char vmunix[50] = "vmunix";		/* and room to patch to whatever */
char myname[50];
char aline[LINEBUFSZ];
extern int debugcmd();
extern char *malloc();
extern char estack[];

#define	MALLOC_PAD	0x1000		/* malloc pad */

static interactive = 0;			/* true if -d flag passed in */

main()
{
	func_t go2, load_it();
	char *arg;
	struct regs fakeregs;
	struct stkfmt fakestkfmt;
	int fakedfc, fakesfc;

	ttysync();
#ifdef sun3
#define PAD	(DEBUGEND - DEBUGSTART)
	/*
	 * Initialize so that the standalone code doesn't
	 * use the same pages the debugger is in.  PAD is
	 * for the pages to be malloc'ed for symbol table info.
	 */
	reset_alloc(*romp->v_memoryavail - PAD);
#endif sun3

	spawn((int *)estack, debugcmd);
	while ((go2 = load_it(&arg)) == (func_t)-1)
		continue;
	if (go2 == (func_t)-2)
		return;
	free(malloc(MALLOC_PAD));
	printf("%s loaded - 0x%x bytes used\n",
	    arg, ctob(pagesused));

	if (interactive) {
		bzero((caddr_t)&fakeregs, sizeof (fakeregs));
		bzero((caddr_t)&fakestkfmt, sizeof (fakestkfmt));
		(void) cmd(fakedfc, fakesfc, fakeregs, fakestkfmt);
		if (dotrace) {
			scbstop = 1;
			dotrace = 0;
		}
	}
	nobrk = 1;		/* no more sbrk's allowed */

	(*go2)(arg);
	/*NOTREACHED*/
}

#define LOAD	0x4000

/*
 * Read in a Unix executable file and return its entry point.
 * Handle the various a.out formats correctly.
 * "Io" is the standalone file descriptor to read from.
 * Print informative little messages if "print" is on.
 * Returns -1 for errors.
 */
func_t
readfile(io, print, name)
	register int io;
	int print;
	char *name;
{
	struct exec x;
	register int i;
	register char *addr;
	register int shared = 0;
	register int loadaddr;
	register int segsiz;
	register int datasize;

	i = read(io, (char *)&x, sizeof x);
	if (x.a_magic == ZMAGIC || x.a_magic == NMAGIC)
		shared = 1;
	if (i != sizeof x || (!shared && x.a_magic != OMAGIC)) {
		printf("Not executable\n");
		return ((func_t)-1);
	}
	if (print)
		printf("Size: %d", x.a_text);
	datasize = x.a_data;
	if (!shared) {
	        x.a_text = x.a_text + x.a_data;
	        x.a_data = 0;
	}
	if (lseek(io, N_TXTOFF(x), 0) == -1)
		goto shread;
	if (read(io, (char *)LOAD, (int)x.a_text) < x.a_text)
		goto shread;
	addr = (char *)(x.a_text + LOAD);
	if (x.a_machtype == M_OLDSUN2)
	        segsiz = OLD_SEGSIZ;
	else
	        segsiz = SEGSIZ;
	if (shared)
		while ((int)addr & (segsiz-1))
			*addr++ = 0;
	if (print)
		printf("+%d", datasize);
	if (read(io, addr, (int)x.a_data) < x.a_data)
		goto shread;
	if (print)
		printf("+%d", x.a_bss);
	addr += x.a_data;
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	if (print)
		printf(" bytes\n");
	if (x.a_machtype != M_OLDSUN2 && x.a_magic == ZMAGIC)
	        loadaddr = LOAD + sizeof (struct exec);
	else
	        loadaddr = LOAD;
	debuginit(io, &x, name);
	return ((func_t)loadaddr);

shread:
	printf("Truncated file\n");
	return ((func_t)-1);
}

/*
 * Prompt for name of file to be read into memory for debugging.
 */
func_t
load_it(arg)
	register char **arg;
{
	register struct bootparam *bp;
	register int howto;
	register int io;
	func_t go2;
	extern char myname_default[];

	if (romp->v_bootparam == 0)
		bp = (struct bootparam *)0x2000;	/* Old Sun-1 address */
	else
		bp = *(romp->v_bootparam);		/* S-2: via romvec */

	*arg = bp->bp_argv[0];
	if (*arg == NULL)
		*arg = "";
	if (myname[0] == '\0') {
		register char *s, *p;

		for (s = *arg; *s && *s++ != ')';)
			;
		if (*s == '\0')
			s = myname_default;
		for (p = myname; *s;)
			*p++ = *s++;
	} else if (interactive == 0) {
		/*
		 * 2nd time thru and we are not interactive,
		 * return fatal error code back to caller.
		 */
		return ((func_t)-2);
	}

	howto = bootflags(bp->bp_argv[1]);
	if (howto & RB_DEBUG)
		interactive = 1;

	/*
	 * Now we have to ask for the name of the program to load
	 * if we are interactive.
	 */
	*aline = '\0';
	if (interactive) {
		printf("%s: ", myname);
		gets(aline);
	}

	/*
	 * If no file name given (or we are non-interactive),
	 * default to patchable string in vmunix[]
	 */
	if (*aline == '\0') {
		register char *s, *p;

		for (s = *arg, p = aline; *s; s++, p++)
			*p = *s;

		for (s = aline; *s && *s++ != ')';)
			;
		for (p = vmunix; *p;)
			*s++ = *p++;
		*s = '\0';
		printf("%s: %s\n", myname, aline);
	}
	parseparam(aline, howto, bp);
	io = open(aline, 0);
	if (io >= 0) {
		go2 = readfile(io, 1, aline);
	} else {
		printf("boot failed\n");
		go2 = (func_t)-1;
	}
	close(io);		/* Done with it. */
	return (go2);
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

	while (*line) {
		if (narg == 1)		/* terminate line for open */
			*line++ = 0;
		while (*line == ' ')
			line++;
		if (*line)
			bp->bp_argv[narg++] = cp;
		while (*line != ' ' && *line != 0)
			*cp++ = *line++;
		if (narg == 2)
			*cp++ = 'd';	/* or in debug flag in argv[1] */
		*cp++ = 0;
	};
	bp->bp_argv[narg] = 0;

	/*
	 * Stuff in default switches if user didn't respecify
	 */
	defaults |= RB_DEBUG;		/* or in debug flag */
	if (narg == 1) {
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
