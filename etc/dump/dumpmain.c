#ifndef lint
static	char sccsid[] = "@(#)dumpmain.c 1.1 86/09/24 SMI"; /* from UCB 1.14 83/06/09 */
#endif

#include "dump.h"

int	notify = 0;	/* notify operator flag */
int	blockswritten = 0;	/* number of blocks written on current tape */
int	tapeno = 0;	/* current tape number */
int	density = 0;	/* density in bytes/0.1" */
int	tenthsperirg;	/* inter-record-gap in 0.1"'s */
int	ntrec = NTREC;	/* # tape blocks in each tape record */
int	cartridge = 0;	/* Assume non-cartridge tape */
char	*host;
char	*index(), *rindex();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		*arg;
	int		i;
	float		fetapes;
	register	struct	fstab	*dt;

	time(&(spcl.c_date));

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	if (arg = rindex(argv[0], '/'))
		arg++;
	else
		arg = argv[0];
	if (*arg == 'r')
		tape = RTAPE;
	else
		tape = TAPE;
	disk = NULL;
	increm = NINCREM;
	temp = TEMP;
	if (TP_BSIZE / DEV_BSIZE == 0 || TP_BSIZE % DEV_BSIZE != 0) {
		msg("TP_BSIZE must be a multiple of DEV_BSIZE\n");
		dumpabort();
	}
	incno = '9';
	uflag = 0;
	arg = "u";
	if (argc > 1) {
		argv++;
		argc--;
		arg = *argv;
		if (*arg == '-')
			argc++;
	}
	while(*arg)
	switch (*arg++) {
	case 'w':
		lastdump('w');		/* tell us only what has to be done */
		exit(0);
		break;
	case 'W':			/* what to do */
		lastdump('W');		/* tell us the current state of what has been done */
		exit(0);		/* do nothing else */
		break;

	case 'f':			/* output file */
		if (argc > 1) {
			argv++;
			argc--;
			tape = *argv;
		}
		break;

	case 'd':			/* density, in bits per inch */
		if (argc > 1) {
			argv++;
			argc--;
			density = atoi(*argv) / 10;
		}
		break;

	case 's':			/* tape size, feet */
		if (argc > 1) {
			argv++;
			argc--;
			tsize = atol(*argv);
			tsize *= 12L*10L;
		}
		break;

	case 'b':			/* blocks per tape write */
		if (argc > 1) {
			argv++;
			argc--;
			ntrec = atol(*argv);
			if (ntrec <= 0 || (ntrec&1)) {
				msg("Block size must be a positive, even integer\n");
				dumpabort();
			}
			ntrec /= 2;
		}
		break;

	case 'c':			/* Tape is cart. not 9-track */
		cartridge++;
		break;

	case '0':			/* dump level */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		incno = arg[-1];
		break;

	case 'u':			/* update /etc/dumpdates */
		uflag++;
		break;

	case 'n':			/* notify operators */
		notify++;
		break;

	default:
		fprintf(stderr, "dump: bad key '%c%'\n", arg[-1]);
		Exit(X_ABORT);
	}
	if (argc > 1) {
		argv++;
		argc--;
		disk = *argv;
	}
	if (strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}
	if (disk == NULL) {
		fprintf(stderr,
		   "Usage: dump [0123456789fusdWwncb [argument]] filesystem\n");
		Exit(X_ABORT);
	}

	/*
	 * Determine how to default tape size and density
	 *
	 *         	density				tape size
	 * 9-track	1600 bpi (160 bytes/.1")	2300 ft.
	 * 9-track	6250 bpi (625 bytes/.1")	2300 ft.
	 * cartridge	8000 bpi (100 bytes/.1")	1700 ft. (450*4 - slop)
	 */
	if (density == 0)
		density = cartridge ? 100 : 160;
	if (tsize == 0)
		tsize = cartridge ? 1700L*120L : 2300L*120L;
	if (index(tape, ':')) {
		host = tape;
		tape = index(host, ':');
		*tape++ = 0;
		if (rmthost(host) == 0)
			exit(X_ABORT);
		tf = &remotetape;
	} else
		tf = &localtape;
	if (signal(SIGHUP, sighup) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTRAP, sigtrap) == SIG_IGN)
		signal(SIGTRAP, SIG_IGN);
	if (signal(SIGFPE, sigfpe) == SIG_IGN)
		signal(SIGFPE, SIG_IGN);
	if (signal(SIGBUS, sigbus) == SIG_IGN)
		signal(SIGBUS, SIG_IGN);
	if (signal(SIGSEGV, sigsegv) == SIG_IGN)
		signal(SIGSEGV, SIG_IGN);
	if (signal(SIGTERM, sigterm) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	

	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	set_operators();	/* /etc/group snarfed */
	getfstab();		/* /etc/fstab snarfed */
	/*
	 *	disk can be either the full special file name,
	 *	the suffix of the special file name,
	 *	the special name missing the leading '/',
	 *	the file system name with or without the leading '/'.
	 */
	dt = fstabsearch(disk);
	if (dt != 0)
		disk = rawname(dt->fs_spec);
	getitime();		/* /etc/dumpdates snarfed */

	msg("Date of this level %c dump: %s\n", incno, prdate(spcl.c_date));
 	msg("Date of last level %c dump: %s\n",
		lastincno, prdate(spcl.c_ddate));
	msg("Dumping %s ", disk);
	if (dt != 0)
		msgtail("(%s) ", dt->fs_file);
	msgtail("to %s", tape);
	if (host)
		msgtail(" on host %s", host);
	msgtail("\n");

	fi = open(disk, 0);
	if (fi < 0) {
		msg("Cannot open %s\n", disk);
		Exit(X_ABORT);
	}
	esize = 0;
	sblock = (struct fs *)buf;
	sync();
	bread(SBLOCK, sblock, SBSIZE);
	if (sblock->fs_magic != FS_MAGIC) {
		msg("bad sblock magic number\n");
		dumpabort();
	}
	msiz = roundup(howmany(sblock->fs_ipg * sblock->fs_ncg, NBBY),
		TP_BSIZE);
	clrmap = (char *)calloc(msiz, sizeof(char));
	dirmap = (char *)calloc(msiz, sizeof(char));
	nodmap = (char *)calloc(msiz, sizeof(char));

	msg("mapping (Pass I) [regular files]\n");
	pass(mark, (char *)NULL);		/* mark updates esize */

	do {
		msg("mapping (Pass II) [directories]\n");
		nadded = 0;
		pass(add, dirmap);
	} while(nadded);

	bmapest(clrmap);
	bmapest(nodmap);

	if (cartridge) {
		/*
		 * Estimate number of tapes, assuming streaming stops at
		 * the end of each block written, and not in mid-block.
		 * Assume no erroneous blocks; this can be compensated for
		 * with an artificially low tape size.
		 */
		tenthsperirg = 16;	/* actually 15.48, says Archive */
		fetapes = 
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes/block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* streaming-stops per block */
			* tenthsperirg	/* 0.1" / streaming-stop */
		) * (1.0 / tsize );	/* tape / 0.1" */
	} else {
		/* Estimate number of tapes, for old fashioned 9-track tape */
#ifdef sun
		/* sun has long irg's */
		tenthsperirg = (density == 625) ? 6 : 12;
#else
		tenthsperirg = (density == 625) ? 3 : 7;
#endif
		fetapes =
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes / block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* IRG's / block */
			* tenthsperirg	/* 0.1" / IRG */
		) * (1.0 / tsize );	/* tape / 0.1" */
	}
	etapes = fetapes;		/* truncating assignment */
	etapes++;
	/* count the nodemap on each additional tape */
	for (i = 1; i < etapes; i++)
		bmapest(nodmap);
	esize += i + 10;	/* headers + 10 trailer blocks */
	msg("estimated %ld tape blocks on %3.2f tape(s).\n", esize, fetapes);

	alloctape();			/* Allocate tape buffer */

	otape();			/* bitmap is the first to tape write */
	time(&(tstart_writing));
	bitmap(clrmap, TS_CLRI);

	msg("dumping (Pass III) [directories]\n");
	pass(dump, dirmap);

	msg("dumping (Pass IV) [regular files]\n");
	pass(dump, nodmap);

	spcl.c_type = TS_END;
	if (host == 0) {
		for(i=0; i<ntrec; i++)
			spclrec();
	}
	msg("DUMP: %ld tape blocks on %d tape(s)\n",spcl.c_tapea,spcl.c_volume);
	msg("DUMP IS DONE\n");

	putitime();
	if (host == 0) {
		if (!pipeout) {
			close(to);
			rewind();
		}
	} else {
		tflush(1);
		rewind();
	}
	broadcast("DUMP IS DONE!\7\7\n");
	Exit(X_FINOK);
}

int	sighup(){	msg("SIGHUP()  try rewriting\n"); sigAbort();}
int	sigtrap(){	msg("SIGTRAP()  try rewriting\n"); sigAbort();}
int	sigfpe(){	msg("SIGFPE()  try rewriting\n"); sigAbort();}
int	sigbus(){	msg("SIGBUS()  try rewriting\n"); sigAbort();}
int	sigsegv(){	msg("SIGSEGV()  ABORTING!\n"); abort();}
int	sigalrm(){	msg("SIGALRM()  try rewriting\n"); sigAbort();}
int	sigterm(){	msg("SIGTERM()  try rewriting\n"); sigAbort();}

sigAbort()
{
	if (pipeout) {
		msg("Unknown signal, cannot recover\n");
		dumpabort();
	}
	msg("Rewriting attempted as response to unknown signal.\n");
	fflush(stderr);
	fflush(stdout);
	close_rewind();
	exit(X_REWRITE);
}

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	strcpy(rawbuf, cp);
	*dp = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp+1);
	return (rawbuf);
}

dumpabort()
{
	msg("The ENTIRE dump is aborted.\n");
	Exit(X_ABORT);
}

Exit(status)
{
#ifdef TDEBUG
	msg("pid = %d exits with status %d\n", getpid(), status);
#endif TDEBUG
	exit(status);
}
