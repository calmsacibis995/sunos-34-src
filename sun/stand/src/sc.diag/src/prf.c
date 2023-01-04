/*
 *----------------------------------------------------------------------------
 *      These coded statements, instructions, and computer programs
 *      contain unpublished proprietary information and are protected
 *      by Federal copyright law.  They may not be disclosed to third
 *      parties or copied or duplicated in any form without the prior
 *      written consent of Lucasfilm Ltd.
 *
 *      Modifications to Unix V7 by John Seamons, Lucasfilm Ltd.
 *----------------------------------------------------------------------------
	@(#)prf.c 1.1 9/25/86 Copyright Sun Microsystems, Inc.
 */


/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %D are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much
 * suspended.
 * Printf should not be used for chit-chat.
 */
/* VARARGS */
printf(fmt, x1)
register char *fmt;
unsigned x1;
{
        register c;
        register unsigned int *adx;
        char *s;

        adx = &x1;
loop:
        while((c = *fmt++) != '%') {
                if(c == '\0') return;
                putchar(c);
        }
        c = *fmt++;
        if(c == 'd' || c == 'u' || c == 'o' || c == 'x')
                printn((unsigned long)*adx, c=='o'? 8: (c=='x'? 16:10));
        else if(c == 's') {
                s = (char *)*adx;
                while(c = *s++)
                        putchar(c);
        } else if (c == 'D') {
                printn(*(unsigned long *)adx, 10);
                adx += (sizeof(long) / sizeof(int)) - 1;
        } else if (c == 'X') {
                printn(*(unsigned long *)adx, 16);
                adx += (sizeof(long) / sizeof(int)) - 1;
        } else if (c == 'O') {
                printn(*(unsigned long *)adx, 8);
                adx += (sizeof(long) / sizeof(int)) - 1;
	} else if (c == 'c') {
		s = (char *)*adx;
		c = *s++;
		putchar(c);
        }
        adx++;
        goto loop;
}

/*
 * Print an unsigned integer in base b.
 */
printn(n, b)
unsigned long n;
{
        register unsigned long a;

        if (n<0) {      /* shouldn't happen */
                putchar('-');
                n = -n;
        }
        if(a = n/b)
                printn(a, b);
        putchar("0123456789ABCDEF"[(int)(n%b)]);
}

gets(buf)
        char *buf;
{
        register char *lp;
        register c;

        lp = buf;
        for (;;) {
                c = getchar() & 0177;

                switch(c) {
                case '\n':
                case '\r':
                        c = '\n';
                        *lp++ = '\0';
                        return;
                case '\b':
                case '#':
                        lp--;
                        if (lp < buf)
                                lp = buf;
                        continue;
                case '@':
                case 'u'&037:
                        lp = buf;
                        putchar('\n');
                        continue;
                default:
                        *lp++ = c;
                }
        }
}

getn() {
        register int num = 0;
        register char c;
	register short neg;

	c = getchar();
	if (c == '-') {
		neg = 1;
		c = getchar();
	} else {
		neg = 0;
	}

	while ((c>='0')&&(c<='9')) {
		num = num*10 + (c - '0');
		c = getchar();
	}

	if (neg) num = -num;
	if (c == '/') return (-1);

        return (num);
}


getnh() {
        register int num;
        register char c;

	num = 0;
        while (c = getchar(), ((c>='0') && (c<='9'))||((c>='A')&&(c<='F'))||
			      ((c>='a')&&(c<='f')) ) {
                num = (num<<4);
		if ((c>='A')&&(c<='F')) {
		   num += (c - '7');
		} else if ((c>='a')&&(c<='f')) {
		   num += (c - 'W');
		} else {
		   num +=  (c-'0');
		}
	}

	if (c == '/') return (-1);

        return (num);
}
