/*	sacat.c	1.1	86/09/25	*/

main()
{
	int c, i;
	char buf[500];

	do {
		printf("File: ");
		gets(buf);
		i = open(buf, 0);
		if (i < 0) printf("Error\n");
	} while (i < 0);

	while ((c = getc(i)) >= 0)
		putchar(c);
	exit(0);
}
