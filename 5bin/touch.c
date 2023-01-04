#ifndef lint
static	char sccsid[] = "@(#)touch.c 1.1 86/09/24 SMI"; /* from S5R2 1.5 */
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define	dysize(A) (((A)%4)? 365: 366)

struct	stat	stbuf;
int	status;
int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char	*cbp;
long	timbuf, time();
long	lseek();

gtime()
{
	register int i, y, t;
	int d, h, m;
	long nt;

	tzset();

	t = gpair();
	if(t<1 || t>12)
		return(1);
	d = gpair();
	if(d<1 || d>31)
		return(1);
	h = gpair();
	if(h == 24) {
		h = 0;
		d++;
	}
	m = gpair();
	if(m<0 || m>59)
		return(1);
	y = gpair();
	if (y<0) {
		(void) time(&nt);
		y = localtime(&nt)->tm_year;
	}
	if (*cbp == 'p')
		h += 12;
	if (h<0 || h>23)
		return(1);
	timbuf = 0;
	y += 1900;
	for(i=1970; i<y; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(y)==366 && t >= 3)
		timbuf += 1;
	while(--t)
		timbuf += dmsize[t-1];
	timbuf += (d-1);
	timbuf *= 24;
	timbuf += h;
	timbuf *= 60;
	timbuf += m;
	timbuf *= 60;
	return(0);
}

gpair()
{
	register int c, d;
	register char *cp;

	cp = cbp;
	if(*cp == 0)
		return(-1);
	c = (*cp++ - '0') * 10;
	if (c<0 || c>100)
		return(-1);
	if(*cp == 0)
		return(-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return(-1);
	cbp = cp;
	return (c+d);
}

main(argc, argv)
char *argv[];
{
	register c;
	register int created;
	struct utbuf { long actime, modtime; } times;

	int mflg=1, aflg=1, cflg=0, nflg=0, errflg=0, optc, fd;
	extern char *optarg;
	extern int optind;

	while ((optc=getopt(argc, argv, "amc")) != EOF)
		switch(optc) {
		case 'm':
			mflg++;
			aflg--;
			break;
		case 'a':
			aflg++;
			mflg--;
			break;
		case 'c':
			cflg++;
			break;
		case '?':
			errflg++;
		}

	if(((argc-optind) < 1) || errflg) {
		(void) fprintf(stderr, "usage: touch [-amc] [mmddhhmm[yy]] file ...\n");
		exit(2);
	}
	status = 0;
	if(!isnumber(argv[optind]))
		if((aflg <= 0) || (mflg <= 0))
			timbuf = time((long *) 0);
		else
			nflg++;
	else {
		cbp = (char *)argv[optind++];
		if(gtime()) {
			(void) fprintf(stderr,"date: bad conversion\n");
			exit(2);
		}
		timbuf += timezone;
		if (localtime(&timbuf)->tm_isdst)
			timbuf += -1*60*60;
	}
	for(c=optind; c<argc; c++) {
		created = 0;
		if(stat(argv[c], &stbuf)) {
			if(cflg) {
				status++;
				continue;
			}
			else if ((fd = creat (argv[c], 0666)) < 0) {
				(void) fprintf(stderr, "touch: cannot create ");
				perror(argv[c]);
				status++;
				continue;
			}
			else {
				(void) close(fd);
				if(stat(argv[c], &stbuf)) {
					(void) fprintf(stderr,"touch: cannot stat ");
					perror(argv[c]);
					status++;
					continue;
				}
			}
			created++;
		}

		if (!nflg) {
			times.actime = times.modtime = timbuf;
			if (mflg <= 0)
				times.modtime = stbuf.st_mtime;
			if (aflg <= 0)
				times.actime = stbuf.st_atime;

			if(utime(argv[c], &times)) {
				(void) fprintf(stderr,"touch: cannot change times on ");
				perror(argv[c]);
				status++;
			}
		} else {
			if (!created) {
				if (touch(argv[c], &stbuf))
					status++;
			}
		}
	}
	exit(status);
}

int
touch(filename, statp)
char *filename;
register struct stat *statp;
{
	int	fd;
	char	first;

	if ((statp->st_mode & S_IFMT) != S_IFREG) {
		(void) fprintf(stderr, "touch: %s: can only touch regular files\n",
		    filename);
		return(1);
	}

	if (statp->st_size) {
		if ((fd = open(filename, 2)) < 0)
			goto error;
		if (read(fd, &first, 1) != 1)
			goto closeerror;
		if (lseek(fd, 0L, 0) == -1)
			goto closeerror;
		if (write(fd, &first, 1) != 1)
			goto closeerror;
	} else {
		if ((fd = creat(filename, 0666)) < 0)
			goto error;
	}
	if (close(fd) < 0)
		goto error;
	return(0);

closeerror:
	(void) close(fd);
error:
	(void) fprintf(stderr, "touch: ");
	perror(filename);
	return(1);
}

isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}
