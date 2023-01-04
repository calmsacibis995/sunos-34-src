#ifndef lint
static	char sccsid[] = "@(#)disk_usage.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef enum 	Boolean { false, true } Boolean;

/*
 * "percent" is the percent fudge for each optional software group.
 * "extra" is the extra added in for /usr so that /usr/tmp is reasonable.
 * "dirfudge" is the percent fudge for /usr.  This accounts for the 10%
 * that the file system takes and room for inodes, cylinder groups and
 * indirect blocks.
 */
#define DEF_PERCENT	10
#define DEF_EXTRA	10
#define DEF_DIRFUDGE	30
#define DEF_ARCHSTR	"010"

int	percent		= DEF_PERCENT;
int	extra		= DEF_EXTRA;
int	dirfudge	= DEF_DIRFUDGE;
char	*archstr	= DEF_ARCHSTR;
int	tot;
Boolean	total = false;
Boolean	script = false;
char	*cmdname;
char	*sname();

main(argc, argv)
int	argc;
char	*argv[];
{
	int	ch;
	char	*dir;
	char	pwd[256];

	cmdname = argv[0];
	argc--;
	argv++;
	while (argc > 0 && argv[0][0] == '-') {
		ch = argv[0][1];
		argc--;
		argv++;
		if (argc <= 0) {
			usage();
		}
		switch (ch) {
		case 'p':
			percent = atoi(argv[0]);
			argc--;
			argv++;
			break;
		case 'd':
			dirfudge = atoi(argv[0]);
			argc--;
			argv++;
			break;
		case 'e':
			extra = atoi(argv[0]);
			argc--;
			argv++;
			break;
		case 'a':
			archstr = argv[0];
			argc--;
			argv++;
			break;
		case 's':
			script = true;
			break;
		case 't':
			total = true;
			break;
		}
	}
	if (argc < 2) {
		usage();
	}
	get_pwd(pwd);
	dir = argv[0];
	argv++;
	argc--;
	while (argc > 0) {
		disk_usage(dir, argv[0], pwd);
		argv++;
		argc--;
	}
	if (total) {
		dir_usage(dir);
	}
}

/*
 * Find the amount of disk space used by an optional software group.
 * The file contains a list of files that comprise the software group.
 * "du" each file in the list and sum the total.
 */
disk_usage(dir, file, pwd)
char	*dir;
char	*file;
char	*pwd;
{
	FILE	*pfp;
	char	line[256];
	char	cmd[256];
	int	size;
	int	filesize;

	size = 0;
	sprintf(cmd, "(cd %s; du -s `cat %s/%s`)", dir, pwd, file);
	pfp = popen(cmd, "r");
	while (fgets(line, sizeof(line), pfp) != NULL) {
		sscanf(line, "%d", &filesize);
		size += filesize;
	}
	pclose(pfp);
	tot += size;
	size += (int) (size * (percent/100.0));
	if (script) {
		printf("1,$s:%s.%s:%d:\n", sname(file), archstr, size * 2);
	} else {
		printf("%s:\t%d Sectors, %d K bytes\n", sname(file), 
		    size * 2, size);
	}
}

/*
 * Determine the space needed for the directory as a whole.
 * Do a "du" on the directory and subtract off the amount
 * computed for the optional software groups.
 * Then apply some percents and fudge factors to take into
 * account the space used for inodes and cylinder groups.
 */
dir_usage(dir)
char	*dir;
{
	FILE	*pfp;
	char	cmd[256];
	int	dirsize;
	int	size;

	sprintf(cmd, "du -s %s", dir);
	pfp = popen(cmd, "r");
	fscanf(pfp, "%d", &dirsize);
	pclose(pfp);
	size = dirsize - tot;
	size += size * (dirfudge/100.0);
	size += extra;
	if (script) {
		printf("1,$s:%s.%s:%d:\n", sname(dir), archstr, size * 2);
	} else {
		printf("%s:\t%d Sectors, %d K bytes with %d percent fudge and %d K extra\n",
		   dir, size * 2, size, dirfudge, extra);
	}
}

/*
 * Return the last component of a path name (the simple name).
 */
char *
sname(dir)
char	*dir;
{
	char	*cp;
	char	*name;

	name = dir;
	for (cp = dir; *cp; cp++) {
		if (*cp == '/') {
			name = &cp[1];
		}
	}
	return(name);
}

get_pwd(buf)
char	*buf;
{
	FILE	*pfp;
	int	len;

	pfp = popen("pwd", "r");
	fgets(buf, 256, pfp);
	len = strlen(buf);
	buf[len - 1] = 0;
	pclose(pfp);
}

usage()
{
	fprintf(stderr, "%s: [-p percent] [-d dirfudge] [-e extra] [-a archstr] [-s] [-t] dir exclude_files ...\n", cmdname);
	fprintf(stderr, "\tdefaults:\tpercent %d%%\n", DEF_PERCENT);
	fprintf(stderr, "\t\t\t: extra %d K\n", DEF_PERCENT);
	fprintf(stderr, "\t\t\t: dirfudge %d%%\n", DEF_PERCENT);
	fprintf(stderr, "\t\t\t: archstr \"%s\"\n", DEF_ARCHSTR);
	exit(1);
}
