#ifndef lint
static	char sccsid[] = "@(#)copy.c	1.1 86/09/25	SMI"; /* From ancient history */
#endif

#include "../mon/sunromvec.h"

#define	BUFSIZ	(3*10*1024)	/* must be multiple of 10K, < 32K */

/*
 * Copy from to in large units.
 * Intended for use in bootstrap procedure.
 */
main()
{
	int from, to;
	char fbuf[50], tbuf[50];
	static char buffer[BUFSIZ];
	register int i;
	register int count = 0;

	printf("Standalone Copy\n");
	from = getdev("From", fbuf, 0);
	to = getdev("To", tbuf, 1);
	for (;;) {
		i = read(from, buffer, sizeof (buffer));
		if (i <= 0)
			break;
		if (write(to, buffer, i) != i) {
			printf("Write error\n");
			break;
		}
		count += i;
	}
	if (i < 0)
		printf("Read error\n");
	printf("Copy completed - %d bytes\n", count);
}

getdev(prompt, buf, mode)
	char *prompt, *buf;
	int mode;
{
	register int i;

	do {
		printf("%s: ", prompt);
		gets(buf);
		i = open(buf, mode);
	} while (i <= 0);
	return (i);
}
