#ifndef lint
static	char sccsid[] = "@(#)touch.c 1.1 86/09/25 SMI"; /* from UCB 4.2 82/06/09 */
#endif

/*
 *	attempt to set the modify date of a file to the current date.
 *	if the file exists, read and write its first character.
 *	if the file doesn't exist, create it, unless -c option prevents it.
 *	if the file is read-only, -f forces chmod'ing and touch'ing.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int	dontcreate;	/* set if -c option */
int	force;		/* set if -f option */

char *whoami = "touch";

main(argc,argv)
	int	argc;
	char	**argv;
{
	char	*argp;
	register int	status;

	dontcreate = 0;
	force = 0;
	for (argv++; *argv && **argv == '-'; argv++) {
		for (argp = &(*argv)[1]; *argp; argp++) {
			switch (*argp) {
			case 'c':
				dontcreate = 1;
				break;
			case 'f':
				force = 1;
				break;
			default:
				fprintf(stderr, "%s: bad option -%c\n",
					whoami, *argp);
				exit(1);
			}
		}
	}
	status = 0;
	for (/*void*/; *argv; argv++) {
		if (touch(*argv) < 0)
			status++;
	}
	exit(status);
}

touch(filename)
	char	*filename;
{
	struct stat	statbuffer;
	register int	rwstatus;

	if (stat(filename,&statbuffer) == -1) {
		if (!dontcreate) {
			return(readwrite(filename,0));
		} else {
			fprintf(stderr, "%s: %s: does not exist\n",
				whoami, filename);
			return(-1);
		}
	}
	if ((statbuffer.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "%s: %s: can only touch regular files\n",
			whoami, filename);
		return(-1);
	}
	if (!access(filename,4|2))
		return(readwrite(filename,statbuffer.st_size));
	if (force) {
		if (chmod(filename,0666)) {
			fprintf(stderr, "%s: %s: couldn't chmod: ",
				whoami, filename);
			perror("");
			return(-1);
		}
		rwstatus = readwrite(filename,statbuffer.st_size);
		if (chmod(filename,statbuffer.st_mode)) {
			fprintf(stderr, "%s: %s: couldn't chmod back: ",
				whoami, filename);
			perror("");
			return(-1);
		}
		return(rwstatus);
	} else {
		fprintf(stderr, "%s: %s: cannot touch\n", whoami, filename);
		return(-1);
	}
}

readwrite(filename,size)
	char	*filename;
	int	size;
{
	int	filedescriptor;
	char	first;

	if (size) {
		filedescriptor = open(filename,2);
		if (filedescriptor == -1) {
error:
			fprintf(stderr, "%s: %s: ", whoami, filename);
			perror("");
			return(-1);
		}
		if (read(filedescriptor, &first, 1) != 1) {
			(void) close(filedescriptor);
			return(-1);
		}
		if (lseek(filedescriptor,0l,0) == -1) {
			(void) close(filedescriptor);
			return(-1);
		}
		if (write(filedescriptor, &first, 1) != 1) {
			(void) close(filedescriptor);
			return(-1);
		}
	} else {
		filedescriptor = creat(filename,0666);
		if (filedescriptor == -1) {
			return(-1);
		}
	}
	if (close(filedescriptor) == -1) {
		return(-1);
	}
	return(0);
}
