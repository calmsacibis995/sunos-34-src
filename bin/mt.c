#ifndef lint
static	char sccsid[] = "@(#)mt.c 1.1 86/09/24 SMI"; /* from UCB 4.8 83/05/08 */
#endif
/*
 * mt --
 *   magnetic tape manipulation program
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>

#define	equal(s1,s2)	(strcmp(s1, s2) == 0)

struct commands {
	char *c_name;
	int c_code;
	int c_ronly;
	int c_usecnt;
} com[] = {
	{ "weof",	MTWEOF,	0,	1 },
	{ "eof",	MTWEOF,	0,	1 },
	{ "fsf",	MTFSF,	1,	1 },
	{ "bsf",	MTBSF,	1,	1 },
	{ "fsr",	MTFSR,	1,	1 },
	{ "bsr",	MTBSR,	1,	1 },
	{ "rewind",	MTREW,	1,	0 },
	{ "offline",	MTOFFL,	1,	0 },
	{ "rewoffl",	MTOFFL,	1,	0 },
	{ "status",	MTNOP,	1,	0 },
	{ "retension",	MTRETEN,1,	0 },
	{ "erase",	MTERASE,0,	0 },
	{ 0 }
};

int mtfd;
struct mtop mt_com;
struct mtget mt_status;
char *tape;

main(argc, argv)
	char **argv;
{
	char line[80], *getenv();
	register char *cp;
	register struct commands *comp;

	if (argc > 2 && (equal(argv[1], "-t") || equal(argv[1], "-f"))) {
		argc -= 2;
		tape = argv[2];
		argv += 2;
	} else
		if ((tape = getenv("TAPE")) == NULL)
			tape = DEFTAPE;
	if (argc < 2) {
		fprintf(stderr, "usage: mt [ -f device ] command [ count ]\n");
		exit(1);
	}
	cp = argv[1];
	for (comp = com; comp->c_name != NULL; comp++)
		if (strncmp(cp, comp->c_name, strlen(cp)) == 0)
			break;
	if (comp->c_name == NULL) {
		fprintf(stderr, "mt: unknown command: %s\n", cp);
		exit(1);
	}
	if ((mtfd = open(tape, comp->c_ronly ? 0 : 2)) < 0) {
		perror(tape);
		exit(1);
	}
	if (comp->c_code != MTNOP) {
		int usecnt;

		mt_com.mt_op = comp->c_code;
		usecnt = argc > 2 && comp->c_usecnt;
		mt_com.mt_count = (usecnt ? atoi(argv[2]) : 1);
		if (mt_com.mt_count < 0) {
			fprintf(stderr, "mt: negative repeat count\n");
			exit(1);
		}
		if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
			fprintf(stderr, "%s %s %d ", tape, comp->c_name,
				mt_com.mt_count);
			perror("failed");
			exit(2);
		}
	} else {
		if (ioctl(mtfd, MTIOCGET, (char *)&mt_status) < 0) {
			perror("mt");
			exit(2);
		}
		status(&mt_status);
	}
	exit(0);
}

#ifdef vax
#include <vaxmba/mtreg.h>
#include <vaxmba/htreg.h>

#include <vaxuba/utreg.h>
#include <vaxuba/tmreg.h>
#undef b_repcnt		/* argh */
#include <vaxuba/tsreg.h>
#endif

#ifdef sun
#include <sundev/tmreg.h>
#include <sundev/xtreg.h>
#include <sundev/arreg.h>
#endif

struct tape_desc {
	short	t_type;		/* type of magtape device */
	char	*t_name;	/* printing name */
	char	*t_dsbits;	/* "drive status" register */
	char	*t_erbits;	/* "error" register */
} tapes[] = {
#ifdef vax
	{ MT_ISTS,	"ts11",		0,		TSXS0_BITS },
	{ MT_ISHT,	"tm03",		HTDS_BITS,	HTER_BITS },
	{ MT_ISTM,	"tm11",		0,		TMER_BITS },
	{ MT_ISMT,	"tu78",		MTDS_BITS,	0 },
	{ MT_ISUT,	"tu45",		UTDS_BITS,	UTER_BITS },
#endif
#ifdef sun
	{ MT_ISCPC,	"TapeMaster",	TMS_BITS,	0 },
	{ MT_ISAR,	"Archive",	ARCH_CTRL_BITS,	ARCH_BITS },
	{ MT_ISSC,	"SCSI Archive",	0,		ARCH_BITS },
	{ MT_ISXY,	"Xylogics 472",	XTS_BITS,	0 },
#endif
	{ 0 }
};

/*
 * Interpret the status buffer returned
 */
status(bp)
	register struct mtget *bp;
{
	register struct tape_desc *mt;

	for (mt = tapes; mt->t_type; mt++)
		if (mt->t_type == bp->mt_type)
			break;
	if (mt->t_type == 0) {
		printf("unknown tape drive type (%d)\n", bp->mt_type);
		return;
	}
	printf("%s tape drive, residual=%d\n", mt->t_name, bp->mt_resid);
	printreg("ds", bp->mt_dsreg, mt->t_dsbits);
	printreg("\ner", bp->mt_erreg, mt->t_erbits);
	if (mt->t_type == MT_ISSC) {	/* kluge */
		printf(" retries=%d underruns=%d", bp->mt_fileno,
			bp->mt_blkno);
	}
	putchar('\n');
}

/*
 * Print a register a la the %b format of the kernel's printf
 */
printreg(s, v, bits)
	char *s;
	register char *bits;
	register unsigned short v;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (v && bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}
