#ifndef lint
static	char sccsid[] = "@(#)cat.c 1.1 86/09/24 SMI"; /* from S5R2 1.6 */
#endif

/*
**	Concatenate files.
*/

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>

int	silent = 0;
int	vflag = 0;
int	tflag = 0;
int	eflag = 0;

int	ibsize;
int	obsize;

main(argc, argv)
int    argc;
char **argv;
{
	register FILE *fi;
	register char *filenm;
	register int c;
	extern	int optind;
	int	errflg = 0;
	int	stdinflg = 0;
	int	status = 0;
	int	dev, ino = -1;
	struct	stat	statb;

#ifdef STANDALONE
	if (argv[0][0] == '\0')
		argc = getargv ("cat", &argv, 0);
#endif
	while( (c=getopt(argc,argv,"usvte")) != EOF ) {
		switch(c) {
		case 'u':
#ifndef	STANDALONE
			setbuf(stdout, (char *)NULL);
#endif
			continue;
		case 's':
			silent++;
			continue;
		case 'v':
			vflag++;
			continue;
		case 't':
			tflag++;
			continue;
		case 'e':
			eflag++;
			continue;
		case '?':
			errflg++;
			break;
		}
		break;
	}

	if (errflg) {
		if (!silent)
			fprintf(stderr,"usage: cat -usvte [-|file] ...\n");
		exit(2);
	}
	if(fstat(fileno(stdout), &statb) < 0) {
		if(!silent) {
			fprintf(stderr, "cat: cannot stat standard output: ");
			perror("");
		}
		exit(2);
	}
	statb.st_mode &= S_IFMT;
	if (statb.st_mode!=S_IFCHR && statb.st_mode!=S_IFBLK) {
		dev = statb.st_dev;
		ino = statb.st_ino;
	}
	obsize = statb.st_blksize;
	if (optind == argc) {
		argc++;
		stdinflg++;
	}
	for (argv = &argv[optind];
	     optind < argc && !ferror(stdout); optind++, argv++) {
		filenm = *argv;
		if (stdinflg || (filenm[0]=='-' && filenm[1]=='\0')) {
			filenm = "standard input";
			fi = stdin;
		} else {
			if ((fi = fopen(filenm, "r")) == NULL) {
				if (!silent) {
				   fprintf(stderr, "cat: cannot open %s: ",
								filenm);
				   perror("");
				}
				status = 2;
				continue;
			}
		}
		if(fstat(fileno(fi), &statb) < 0) {
			if(!silent) {
			   fprintf(stderr, "cat: cannot stat %s: ", filenm);
			   perror("");
			}
			status = 2;
			continue;
		}
		if (statb.st_dev==dev && statb.st_ino==ino) {
			if(!silent)
			   fprintf(stderr, "cat: input %s is output\n",
						stdinflg?"-": *argv);
			if (fclose(fi) != 0 ) {
				fprintf(stderr, "cat: close error: ");
				perror("");
			}
			status = 2;
			continue;
		}
		ibsize = statb.st_blksize;

		if (vflag)
			status = vcat(fi, filenm);
		else
			status = cat(fi, filenm);

		if (fi!=stdin)
			fflush(stdout);
		if (fclose(fi) != 0) 
			if (!silent) {
				fprintf(stderr, "cat: close error: ");
				perror("");
			}
	}
	fflush(stdout);
	if (ferror(stdout)) {
		if (!silent) {
			fprintf (stderr, "cat: output error: ");
			perror("");
		}
		status = 2;
	}
	exit(status);
}

int
cat(fi, filenm)
	FILE *fi;
	char *filenm;
{
	register int nitems;
	register int nwritten;
	register int offset;
	register int fi_desc;
	register int buffsize;
	register char *bufferp;
	extern char *malloc();

	if (obsize)
		buffsize = obsize;	/* common case, use output blksize */
	else if (ibsize)
		buffsize = ibsize;
	else
		buffsize = BUFSIZ;

	if ((bufferp = malloc(buffsize)) == NULL) {
		perror("cat: no memory");
		return (1);
	}

	fi_desc = fileno(fi);

	for (;;) {
		nitems=read(fi_desc,bufferp,buffsize);
		if (nitems <= 0) break;
		offset = 0;
		/*
		 * Note that on some systems (V7), very large writes to a pipe
		 * return less than the requested size of the write.
		 * In this case, multiple writes are required.
		 */
		do {
			nwritten = write(1,bufferp+offset,(unsigned)nitems);
			if (nwritten < 0) {
				if (!silent) {
					fprintf(stderr, "cat: output error: ");
					perror("");
				}
				free(bufferp);
				return(2);
			}
			offset += nwritten;
		} while ((nitems -= nwritten) > 0);
	}
	free(bufferp);
	if (nitems < 0) {
		fprintf(stderr, "cat: input error on %s: ", filenm);
		perror("");
	}

	return(0);
}


/* character type table */

#define PLAIN	0
#define CONTRL	1
#define TAB	2
#define BACKSP	3
#define NEWLIN	4

char ctype[128] = {
	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,
	CONTRL, TAB,	NEWLIN,	CONTRL,	TAB,	CONTRL,	CONTRL,	CONTRL,
	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,
	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,	CONTRL,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	CONTRL,
};

int
vcat(fi, filenm)
	FILE *fi;
	char *filenm;
{
	register int c;

	while ((c = getc(fi)) != EOF) {
		if (c & 0200) {
			putchar('M');
			putchar('-');
			c-= 0200;
		}

		switch(ctype[c]) {
			case PLAIN:
				putchar(c);
				break;
			case TAB:
				if (!tflag)
					putchar(c);
				else {
					putchar('^');
					putchar (c^0100);
				}
				break;
			case CONTRL:
				putchar('^');
				putchar(c^0100);
				break;
			case NEWLIN:
				if (eflag)
					putchar('$');
				putchar(c);
				break;
			}
		}
	if (ferror(fi)) {
		fprintf(stderr, "cat: input error on %s: ", filenm);
		perror("");
	}
	return(0);
}
