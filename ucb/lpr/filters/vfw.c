#ifndef lint
static	char sccsid[] = "@(#)vfw.c 1.1 86/09/25 SMI"; /* from UCB 4.1 83/04/29 */
#endif

/*
 * Quick hack to see the values in a troff width table.
 */

#include <stdio.h>

main(argc,argv)
char **argv;
{
	FILE *f;
	int c;
	int i;

	if (argc != 2) {
		printf("usage: vfw ftX\n");
		exit(1);
	}
	f = fopen(argv[1], "r");
	if (f == NULL) {
		printf("Can't open %s\n", argv[1]);
		exit(1);
	}
	fseek(f, 32L, 0);
	for (i=0; !feof(f); i++) {
		c = getc(f);
		printf("%d\t%d\n", i, c&255);
	}
	exit(0);
}
