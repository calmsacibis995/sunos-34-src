#ifndef lint
static	char *sccsid = "@(#)newfs.c 1.1 86/09/24 SMI"; /* from UCB 5.2 9/11/85 */
#endif

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <ufs/fs.h>
#include <sys/dir.h>

#include <stdio.h>
#include <disktab.h>

#ifdef sun
#include <sys/ioctl.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>

struct disktab *getdiskbydev();

#endif sun

#define	BOOTDIR	"/usr/mdec"	/* directory for boot blocks */

int	Nflag;			/* run mkfs without writing file system */
int	verbose;		/* show mkfs line before exec */
int	noboot;			/* do not fill boot blocks */
int	fssize;			/* file system size */
int	fsize;			/* fragment size */
int	bsize;			/* block size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	cpg;			/* cylinders/cylinder group */
int	minfree = -1;		/* free space threshold */
int	opt;			/* optimization preference (space or time) */
int	rpm;			/* revolutions/minute of drive */
int	density;		/* number of bytes per inode */
int	apc;			/* alternates per cylinder */

char	device[MAXPATHLEN];
char	cmd[BUFSIZ];

char	*index();
char	*rindex();
char	*sprintf();

main(argc, argv)
	char *argv[];
{
	char *cp, *special;
	register struct disktab *dp;
	register struct partition *pp;
	struct stat st;
	register int i;
	int status;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'v':
				verbose++;
				break;

			case 'N':
				Nflag++;
				/* fall through to */

			case 'n':
				noboot++;
				break;

			case 's':
				if (argc < 1)
					fatal("-s: missing file system size");
				argc--, argv++;
				fssize = atoi(*argv);
				if (fssize < 0)
					fatal("%s: bad file system size",
						*argv);
				goto next;

			case 't':
				if (argc < 1)
					fatal("-t: missing track total");
				argc--, argv++;
				ntracks = atoi(*argv);
				if (ntracks < 0)
					fatal("%s: bad total tracks", *argv);
				goto next;

			case 'o':
				if (argc < 1)
					fatal("-o: missing optimization preference");
				argc--, argv++;
				if (strcmp(*argv, "space") == 0)
					opt = FS_OPTSPACE;
				else if (strcmp(*argv, "time") == 0)
					opt = FS_OPTTIME;
				else
					fatal("%s: bad optimization preference %s",
					    *argv,
					    "(options are `space' or `time')");
				goto next;

			case 'a':
				if (argc < 1)
					fatal(
					 "-a: missing # of alternates per cyl");
				argc--, argv++;
				apc = atoi(*argv);
				if (apc < 0)
					fatal("%s: bad altenates per cyl ",
						*argv);
				goto next;

			case 'b':
				if (argc < 1)
					fatal("-b: missing block size");
				argc--, argv++;
				bsize = atoi(*argv);
				if (bsize < 0 || bsize < MINBSIZE)
					fatal("%s: bad block size", *argv);
				goto next;

			case 'f':
				if (argc < 1)
					fatal("-f: missing frag size");
				argc--, argv++;
				fsize = atoi(*argv);
				if (fsize < 0)
					fatal("%s: bad frag size", *argv);
				goto next;

			case 'c':
				if (argc < 1)
					fatal("-c: missing cylinders/group");
				argc--, argv++;
				cpg = atoi(*argv);
				if (cpg < 0)
					fatal("%s: bad cylinders/group", *argv);
				goto next;

			case 'm':
				if (argc < 1)
					fatal("-m: missing free space %%\n");
				argc--, argv++;
				minfree = atoi(*argv);
				if (minfree < 0 || minfree > 99)
					fatal("%s: bad free space %%\n",
						*argv);
				goto next;

			case 'r':
				if (argc < 1)
					fatal("-r: missing revs/minute\n");
				argc--, argv++;
				rpm = atoi(*argv);
				if (rpm < 0)
					fatal("%s: bad revs/minute\n", *argv);
				goto next;

			case 'i':
				if (argc < 1)
					fatal("-i: missing bytes per inode\n");
				argc--, argv++;
				density = atoi(*argv);
				if (density < 0)
					fatal("%s: bad bytes per inode\n",
						*argv);
				goto next;

			default:
				fatal("-%c: unknown flag", cp);
			}
next:
		argc--, argv++;
	}
#ifdef vax
	if (argc < 2) {
		fprintf(stderr, "usage: newfs [ -v ] [ mkfs-options ] %s\n",
			"special-device device-type");
#endif vax
#ifdef sun
	if (argc < 1) {
		fprintf(stderr, "usage: newfs [ -v ] [ mkfs-options ] %s\n",
			"raw-special-device");
#endif sun
		fprintf(stderr, "where mkfs-options are:\n");
		fprintf(stderr, "\t-N do not create file system, %s\n",
			"just print out parameters");
		fprintf(stderr, "\t-s file system size (sectors)\n");
		fprintf(stderr, "\t-b block size\n");
		fprintf(stderr, "\t-f frag size\n");
		fprintf(stderr, "\t-t tracks/cylinder\n");
		fprintf(stderr, "\t-c cylinders/group\n");
		fprintf(stderr, "\t-m minimum free space %%\n");
		fprintf(stderr, "\t-o optimization preference %s\n",
			"(`space' or `time')");
		fprintf(stderr, "\t-r revolutions/minute\n");
		fprintf(stderr, "\t-i number of bytes per inode\n");
		fprintf(stderr, "\t-a number of alternates per cylinder\n");
		exit(1);
	}
	special = argv[0];
again:
	if (stat(special, &st) < 0) {
		if (*special != '/') {
			if (*special == 'r')
				special++;
			special = sprintf(device, "/dev/r%s", special);
			goto again;
		}
		fprintf(stderr, "newfs: "); perror(special);
		exit(2);
	}
#ifndef sun
	if ((st.st_mode & S_IFMT) != S_IFBLK &&
	    (st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a block or character device", special);
	dp = getdiskbyname(argv[1]);
	dp->d_apc = 0;		/* just in case getdiskbyname does not zero */
#else sun
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a raw disk device", special);
	dp = getdiskbydev(special);
#endif sun
	if (dp == 0)
#ifndef sun
		fatal("%s: unknown disk type", argv[1]);
#else sun
		fatal("%s: unknown disk information", argv[1]);
#endif sun
	cp = index(argv[0], '\0') - 1;
	if (cp == 0 || *cp < 'a' || *cp > 'h')
		fatal("%s: can't figure out file system partition", argv[0]);
	pp = &dp->d_partitions[*cp - 'a'];
	if (fssize == 0) {
		fssize = pp->p_size;
		if (fssize < 0)
			fatal("%s: no default size for `%c' partition",
				argv[1], *cp);
	}
	if (nsectors == 0) {
		nsectors = dp->d_nsectors;
		if (nsectors < 0)
			fatal("%s: no default #sectors/track", argv[1]);
	}
	if (ntracks == 0) {
		ntracks = dp->d_ntracks;
		if (ntracks < 0)
			fatal("%s: no default #tracks", argv[1]);
	}
	if (bsize == 0) {
		bsize = pp->p_bsize;
		if (bsize < 0)
			fatal("%s: no default block size for `%c' partition",
				argv[1], *cp);
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize < 0)
			fatal("%s: no default frag size for `%c' partition",
				argv[1], *cp);
	}
	if (rpm == 0) {
		rpm = dp->d_rpm;
		if (rpm < 0)
			fatal("%s: no default revolutions/minute value",
				argv[1]);
	}
	if (density <= 0)
		density = 2048;
	if (cpg == 0)
		cpg = 16;
	if (minfree < 0)
		minfree = 10;
	if (minfree < 10 && opt != FS_OPTSPACE) {
		fprintf(stderr, "setting optimization for space ");
		fprintf(stderr, "with minfree less than 10%\n");
		opt = FS_OPTSPACE;
	}
	sprintf(cmd, "/etc/mkfs %s%s %d %d %d %d %d %d %d %d %d %s %d",
		Nflag ? "-N " : "", special,
		fssize, nsectors, ntracks, bsize, fsize, cpg, minfree, rpm/60,
		density, opt == FS_OPTSPACE ? "s" : "t", apc);
	if (verbose)
		printf("%s\n", cmd);
	if (status = system(cmd))
		exit(status >> 8);
	sprintf(cmd, "/etc/fsirand %s\n", special);
	if (status = system(cmd))
		printf("%s: failed, status = %d\n", cmd, status);
	if (*cp == 'a' && !noboot) {
		char type[3];
		struct stat sb;

		cp = rindex(special, '/');
		if (cp == NULL)
			fatal("%s: can't figure out disk type from name",
				special);
		if (stat(special, &sb) >= 0 && (sb.st_mode & S_IFMT) == S_IFCHR)
			cp++;
		type[0] = *++cp;
		type[1] = *++cp;
		type[2] = '\0';
		installboot(special, type);
	}
	exit(0);
}

installboot(dev, type)
	char *dev, *type;
{
	int fd;
#ifdef vax
	char bootblock[MAXPATHLEN];
	char boot0image[DEV_BSIZE];
#endif vax
	char standalonecode[MAXPATHLEN];
	char boot1image[BBSIZE - DEV_BSIZE];

#ifdef vax
	sprintf(bootblock, "%s/%sboot", BOOTDIR, type);
#endif vax
	sprintf(standalonecode, "%s/boot%s", BOOTDIR, type);
	if (verbose) {
		printf("installing boot code\n");
#ifdef vax
		printf("sector 0 boot = %s\n", bootblock);
#endif vax
		printf("1st level boot = %s\n", standalonecode);
	}
#ifdef vax
	fd = open(bootblock, 0);
	if (fd < 0) {
		fprintf(stderr, "newfs: "); perror(bootblock);
		exit(1);
	}
	if (read(fd, boot0image, sizeof boot0image) < 0) {
		fprintf(stderr, "newfs: "); perror(bootblock);
		exit(2);
	}
	close(fd);
#endif vax
	fd = open(standalonecode, 0);
	if (fd < 0) {
		fprintf(stderr, "newfs: "); perror(standalonecode);
		exit(1);
	}
	if (read(fd, boot1image, sizeof boot1image) < 0) {
		fprintf(stderr, "newfs: "); perror(standalonecode);
		exit(2);
	}
	close(fd);
	fd = open(dev, 1);
	if (fd < 0) {
		fprintf(stderr, "newfs: "); perror(dev);
		exit(1);
	}
#ifdef vax
	if (write(fd, boot0image, sizeof boot0image) != sizeof boot0image) {
		fprintf(stderr, "newfs: "); perror(dev);
		exit(2);
	}
#endif vax
	lseek(fd, (long)DEV_BSIZE, 0);
	if (write(fd, boot1image, sizeof boot1image) != sizeof boot1image) {
		fprintf(stderr, "newfs: "); perror(dev);
		exit(2);
	}
	close(fd);
}

/*VARARGS*/
fatal(fmt, arg1, arg2)
	char *fmt;
{

	fprintf(stderr, "newfs: ");
	fprintf(stderr, fmt, arg1, arg2);
	putc('\n', stderr);
	exit(10);
}

#ifdef sun
struct disktab *
getdiskbydev(disk)
	char *disk;
{
	static struct disktab d;
	struct dk_geom g;
	struct dk_map m;
	int fd, part;

	part = disk[strlen(disk)-1] - 'a';
	if ((fd = open(disk, 0)) < 0) {
		perror(disk);
		exit(1);
	}
	if (ioctl(fd, DKIOCGGEOM, &g) < 0) {
		perror("geom ioctl");
		exit(1);
	}
	d.d_secsize = 512;
	d.d_ntracks = g.dkg_nhead;
	d.d_nsectors = g.dkg_nsect;
	d.d_ncylinders = g.dkg_ncyl;
	d.d_rpm = 3600;
	d.d_apc = g.dkg_apc;
	if (ioctl(fd, DKIOCGPART, &m) < 0) {
		perror("part ioctl");
		exit(1);
	}
	close(fd);
	d.d_partitions[part].p_size = m.dkl_nblk;
	d.d_partitions[part].p_bsize = 8192;
	d.d_partitions[part].p_fsize = 1024;
	return (&d);
}
#endif sun
