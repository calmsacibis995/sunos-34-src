#ifndef lint
static	char sccsid[] = "@(#)df.c 1.1 86/09/24 SMI"; /* from UCB 4.18 84/02/02 */
#endif
/*
 * df
 */
#include <sys/param.h>
#include <errno.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <stdio.h>
#include <mntent.h>

void	usage(), pheader();
char	*mpath();
int	iflag;
int	type;
char	*typestr;

struct mntent *getmntpt(), *mntdup();

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define sblock sb.iu_fs

int
main(argc, argv)
	int argc;
	char **argv;
{
	int i;
	struct stat statb;
	char tmpname[1024];

	/*
	 * Skip over command name, if present.
	 */
	if (argc > 0) {
		argv++;
		argc--;
	}

	while (argc > 0 && (*argv)[0]=='-') {
		switch ((*argv)[1]) {

		case 'i':
			iflag++;
			break;

		case 't':
			type++;
			argv++;
			argc--;
			if (argc <= 0)
				usage();
			typestr = *argv;
			break;

		default:
			usage();
		}
		argc--, argv++;
	}
	if (argc > 0 && type) {
		usage();
	}
	sync();
	if (argc <= 0) {
		register FILE *mtabp;
		register struct mntent *mnt;

		if ((mtabp = setmntent(MOUNTED, "r")) == NULL) {
			(void) fprintf(stderr, "df: ");
			perror(MOUNTED);
			exit(1);
		}
		pheader();
		while ((mnt = getmntent(mtabp)) != NULL) {
			if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
				continue;
			if (type && strcmp(typestr, mnt->mnt_type) != 0) {
				continue;
			}
			if (strcmp(mnt->mnt_type, MNTTYPE_42) == 0 &&
			    (stat(mnt->mnt_fsname, &statb) >= 0) &&
			   (((statb.st_mode & S_IFMT) == S_IFBLK) ||
			    ((statb.st_mode & S_IFMT) == S_IFCHR))) {
				(void) strcpy(tmpname, mnt->mnt_fsname);
				dfreedev(tmpname);
			} else {
				dfreemnt(mnt->mnt_dir, mnt);
			}
		}
		(void) endmntent(mtabp);
	} else {
		pheader();
		for (i = 0; i < argc; i++) {
			register struct mntent *mnt;

			if (stat(argv[i], &statb) < 0) {
				(void) fprintf(stderr, "df: ");
				perror(argv[i]);
			} else {
				if ((statb.st_mode & S_IFMT) == S_IFBLK ||
				    (statb.st_mode & S_IFMT) == S_IFCHR) {
					dfreedev(argv[i]);
				} else {
					if ((mnt = getmntpt(argv[i])) != NULL) {
						if (!type ||
						    strcmp(typestr, mnt->mnt_type) == 0) {
							dfreemnt(argv[i], mnt);
						}
					}
				}
			}
		}
	}
	exit(0);
	/*NOTREACHED*/
}

void
pheader()
{
	if (iflag)
		(void) printf("Filesystem             iused   ifree  %%iused");
	else
		(void) printf("Filesystem            kbytes    used   avail capacity");
	(void) printf("  Mounted on\n");
}

dfreedev(file)
	char *file;
{
	long totalblks, availblks, avail, free, used;
	int fi;

	fi = open(file, 0);
	if (fi < 0) {
		(void) fprintf(stderr, "df: ");
		perror(file);
		return;
	}
	if (bread(file, fi, SBLOCK, (char *)&sblock, SBSIZE) == 0) {
		(void) close(fi);
		return;
	}
	(void) printf("%-20.20s", file);
	if (iflag) {
		int inodes = sblock.fs_ncg * sblock.fs_ipg;
		used = inodes - sblock.fs_cstotal.cs_nifree;
		(void) printf("%8ld%8ld%6.0f%% ", used, sblock.fs_cstotal.cs_nifree,
		    inodes == 0 ? 0.0 : (double)used / (double)inodes * 100.0);
	} else {
		totalblks = sblock.fs_dsize;
		free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
		    sblock.fs_cstotal.cs_nffree;
		used = totalblks - free;
		availblks = totalblks * (100 - sblock.fs_minfree) / 100;
		avail = availblks > used ? availblks - used : 0;
		(void) printf("%8d%8d%8d",
		    totalblks * sblock.fs_fsize / 1024,
		    used * sblock.fs_fsize / 1024,
		    avail * sblock.fs_fsize / 1024);
		(void) printf("%6.0f%%",
		    availblks==0? 0.0: (double)used/(double)availblks * 100.0);
		(void) printf("  ");
	}
	(void) printf("  %s\n", mpath(file));
	(void) close(fi);
}

dfreemnt(file, mnt)
	char *file;
	struct mntent *mnt;
{
	struct statfs fs;

	if (statfs(file, &fs) < 0) {
		perror(file);
		return;
	}

	if (strlen(mnt->mnt_fsname) > 20) {
		(void) printf("%s\n", mnt->mnt_fsname);
		(void) printf("                    ");
	} else {
		(void) printf("%-20.20s", mnt->mnt_fsname);
	}
	if (iflag) {
		long files, used;

		files = fs.f_files;
		used = files - fs.f_ffree;
		(void) printf("%8ld%8ld%6.0f%% ", used, fs.f_ffree,
		    files == 0? 0.0: (double)used / (double)files * 100.0);
	} else {
		long totalblks, avail, free, used, reserved;

		totalblks = fs.f_blocks;
		free = fs.f_bfree;
		used = totalblks - free;
		avail = fs.f_bavail;
		reserved = free - avail;
		if (avail < 0)
			avail = 0;
		(void) printf("%8d%8d%8d", totalblks * fs.f_bsize / 1024,
		    used * fs.f_bsize / 1024, avail * fs.f_bsize / 1024);
		totalblks -= reserved;
		(void) printf("%6.0f%%",
		    totalblks==0? 0.0: (double)used/(double)totalblks * 100.0);
		(void) printf("  ");
	}
	(void) printf("  %s\n", mnt->mnt_dir);
}

/*
 * Given a name like /usr/src/etc/foo.c returns the mntent
 * structure for the file system it lives in.
 */
struct mntent *
getmntpt(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt, *mntsave;
	struct stat filestat, dirstat;

	if (stat(file, &filestat) < 0) {
		(void) fprintf(stderr, "df: ");
		perror(file);
		return(NULL);
	}

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		(void) fprintf(stderr, "df: ");
		perror(MOUNTED);
		exit(1);
	}

	mntsave = NULL;
	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		if ((stat(mnt->mnt_dir, &dirstat) >= 0) &&
		   (filestat.st_dev == dirstat.st_dev)) {
			mntsave = mntdup(mnt);
		}
	}
	(void) endmntent(mntp);
	if (mntsave) {
		return(mntsave);
	} else {
		(void) fprintf(stderr, "Couldn't find mount point for %s\n", file);
		exit(1);
	}
	/*NOTREACHED*/
}

/*
 * Given a name like /dev/rrp0h, returns the mounted path, like /usr.
 */
char *
mpath(file)
	char *file;
{
	FILE *mntp;
	register struct mntent *mnt;

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		(void) fprintf(stderr, "df: ");
		perror(MOUNTED);
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(file, mnt->mnt_fsname) == 0) {
			(void) endmntent(mntp);
			return (mnt->mnt_dir);
		}
	}
	(void) endmntent(mntp);
	return "";
}

long lseek();

int
bread(file, fi, bno, buf, cnt)
	char *file;
	int fi;
	daddr_t bno;
	char *buf;
	int cnt;
{
	register int n;
	extern int errno;

	(void) lseek(fi, (long)(bno * DEV_BSIZE), 0);
	if ((n = read(fi, buf, (unsigned) cnt)) < 0) {
		/* probably a dismounted disk if errno == EIO */
		if (errno != EIO) {
			(void) fprintf(stderr, "df: read error on ");
			perror(file);
			(void) fprintf(stderr, "bno = %ld\n", bno);
		} else {
			(void) fprintf(stderr, "df: premature EOF on %s\n",
			    file);
			(void) fprintf("bno = %ld expected = %d count = %d\n",
			    bno, cnt, n);
		}
		return (0);
	}
	return (1);
}

char *
xmalloc(size)
	unsigned int size;
{
	register char *ret;
	char *malloc();
	
	if ((ret = (char *)malloc(size)) == NULL) {
		(void) fprintf(stderr, "umount: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	register struct mntent *mnt;
{
	register struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	(void) strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	(void) strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	(void) strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	(void) strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

void
usage()
{

	(void) fprintf(stderr, "usage: df [ -i ] [ -t type | file... ]\n");
	exit(0);
}
