#ifndef lint
static	char sccsid[] = "@(#)sum.c 1.1 86/09/25 SMI"; /* from UCB 4.1 10/01/80 */
#endif
/*
 * Sum bytes in file mod 2^16
 */

#include <stdio.h>

main(argc,argv)
char **argv;
{
	register unsigned sum;
	register i, c;
	register FILE *f;
	register long nbytes;
	int errflg = 0;

	i = 1;
	do {
		if(i < argc) {
			if ((f = fopen(argv[i], "r")) == NULL) {
				fprintf(stderr, "sum: Can't open %s\n", argv[i]);
				errflg += 10;
				continue;
			}
		} else
			f = stdin;
		sum = 0;
		nbytes = 0;
		while ((c = getc(f)) != EOF) {
			nbytes++;
			if (sum&01)
				sum = (sum>>1) + 0x8000;
			else
				sum >>= 1;
			sum += c;
			sum &= 0xFFFF;
		}
		if (ferror(f)) {
			errflg++;
			fprintf(stderr, "sum: read error on %s\n", argc>1?argv[i]:"-");
		}
		printf("%05u%6ld", sum, (nbytes+BUFSIZ-1)/BUFSIZ);
		if(argc > 2)
			printf(" %s", argv[i]);
		printf("\n");
		fclose(f);
	} while(++i < argc);
	exit(errflg);
}
