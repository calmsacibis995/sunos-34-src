#ifndef lint
static	char sccsid[] = "@(#)nd.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * /etc/nd  -  setup parameters for /dev/nd*
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <sun/ndio.h>
#include <stdio.h>
#include <netdb.h>

#define DEFAULT_MAXPACKS 6	/* old and arbitrary */

char 	line[256], line2[256];
char	*cvec[16];		/* arg strings */
int	ivec;			/* current index */

int	fdnd;
char	*pathnd = "/dev/rndl0";

main(argc, argv)
	char	**argv;
{
	int err = 0;

	if ((fdnd = open(pathnd, 0)) < 0) {
		printf("can't open %s\n", pathnd);
		exit(1);
	}
	if (argc <= 1 || strcmp(argv[1], "-") == 0) {
		while (fgets(line, sizeof line, stdin) != NULL) {
			line[strlen(line)-1] = 0;
			err |= doline();
		}
		exit(err);
	}
	for (argv++, argc--; argc; argv++, argc--) {
		strcat(line, *argv);
		strcat(line, " ");
	}
	exit(doline());
}

struct cmdtab {
	char	*cmd;
	int	tok;
} cmdtab[] = {
	"son",		NDIOCSON,
	"soff",		NDIOCSOFF,
	"serverat",	NDIOCSAT,
	"user",		NDIOCUSER,
	"clear",	NDIOCCLEAR,
	"version",	NDIOCVER,
	"ether",	NDIOCETHER,
	0,		0,
};

doline()
{
	register char *cp, *cpe;
	int i, fd = 0;
	struct hostent *hp;

	if (line[0] == '#')
		return (0);
	strcpy(line2, line);
	for (cp = line; *cp != 0; cp++)
		if (*cp == ' ' || *cp == '\t') 
			*cp = 0;
	cpe = cp;
	for (ivec = 0, cp = line; cp < cpe; cp++) {
		if (*cp == 0) 
			continue;
		cvec[ivec] = cp;
		ivec++;
		for (; *cp != 0; cp++);
	}

	for (i = 0; cmdtab[i].cmd; i++) 
		if (strcmp(cmdtab[i].cmd, cvec[0]) == 0)
			break;

	switch (cmdtab[i].tok) {

	case NDIOCSON:
		if (ivec != 1)
			goto bad;
		if (ioctl(fdnd, NDIOCSON, 0)) {
			perror("nd son ioctl NDIOCSON");
			goto bad;
		}
		break;

	case NDIOCSOFF:
		if (ivec != 1)
			goto bad;
		if (ioctl(fdnd, NDIOCSOFF, 0)) {
			perror("nd soff ioctl NDIOCSOFF");
			goto bad;
		}
		break;

	case NDIOCSAT:
		if (ivec != 2)
			goto bad;
		if ((hp = gethostbyname(cvec[1])) == NULL) {
			printf("nd: unknown hostname: %s\n", cvec[1]);
			goto bad;
		}
		if (ioctl(fdnd, NDIOCSAT, hp->h_addr)) {
			perror("nd serverat ioctl NDIOCSAT");
			goto bad;
		}
		break;

	case NDIOCUSER:
		if (ivec != 7 && ivec != 8) {
			printf("nd user: wrong arg count\n");
			goto bad;
		}
		if (douser(cvec))
			goto bad;
		break;

	case NDIOCCLEAR:
		if (ivec != 1)
			goto bad;
		if (ioctl(fdnd, NDIOCCLEAR, 0)) {
			perror("nd clear ioctl NDIOCCLEAR");
			goto bad;
		}
		break;

	case NDIOCVER:
		if (ivec != 2)
			goto bad;
		if ((i = atoi(cvec[1])) < 0 || i > 15) {
			printf("nd ver: bad version number\n");
			goto bad;
		}
		if (ioctl(fdnd, NDIOCVER, &i)) {
			perror("nd version ioctl NDIOCVER");
			goto bad;
		}
		break;
		
	case NDIOCETHER:
		printf("nd: \"%s\": obsolete command (ignored)\n", line2);
		break;

	default:
	bad:
		printf("nd: \"%s\": bad command (ignored)\n", line2);
		return (1);
	}
	return (0);
}

static long int fakeaddr = -1;

douser(cvec)
	char **cvec;
{
	struct ndiocuser nu;
	struct hostent *hp;
	struct ndiocether ne;
	int doether = 1;

	bzero(&nu, sizeof (nu));
	bzero(&ne, sizeof (ne));
	if (strcmp(cvec[1], "0") != 0) {
		if ((hp = gethostbyname(cvec[1])) == NULL) {
			printf("nd user: unknown host %s\n", cvec[1]);
			nu.nu_addr.s_addr = ne.ne_iaddr.s_addr = fakeaddr--;
			doether = 0;
		} else {
			nu.nu_addr=ne.ne_iaddr = *(struct in_addr *)hp->h_addr;
		}
		/*
		 * get 48 bit ether address
		 */
		ne.ne_maxpacks = DEFAULT_MAXPACKS;
		if (ether_hostton(cvec[1], &ne.ne_eaddr) != 0) {
			printf("nd user: unknown ether %s\n", cvec[1]);
			doether = 0;
		}
	} else {
		nu.nu_addr.s_addr = 0;		/* public */
		doether = 0;
	}
	if ((nu.nu_hisunit = atoi(cvec[2])) < 0) {
		printf("nd user: bad unit number\n");
		return (1);
	}
	if ((nu.nu_myoff = atoi(cvec[4])) < 0) {
		printf("nd user: bad offset\n");
		return (1);
	}
	if ((nu.nu_mysize = atoi(cvec[5])) < 0) {
		if (strcmp(cvec[5], "-1") != 0) {
			printf("nd user: bad size\n");
			return (1);
		}
	}
	if ((nu.nu_mylunit = atoi(cvec[6])) < 0) {
		if (strcmp(cvec[6], "-1") != 0) {
			printf("nd user: bad lunit number\n");
			return (1);
		}
	}
	if ((nu.nu_mydev = open(cvec[3], 0)) < 0) {
		printf("nd user: can't open %s\n", cvec[3]);
		return (1);
	}
	if (ivec == 8) {
		ne.ne_maxpacks = atoi(cvec[7]);
		if (ne.ne_maxpacks < 0
		    || ne.ne_maxpacks > 2 * DEFAULT_MAXPACKS) {
			printf("nd user: bad maxpacks\n");
			return (1);
		}
	}
	if (ioctl(fdnd, NDIOCUSER, (char *)&nu)) {
		perror("nd user ioctl NDIOCUSER");
		close(nu.nu_mydev);
		return (1);
	}
	if (doether && ioctl(fdnd, NDIOCETHER, (char *)&ne)) {
		perror("nd ether ioctl NDIOCETHER");
		return (1);
	}
	close(nu.nu_mydev);
	return (0);
}

