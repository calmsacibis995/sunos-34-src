/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_comp.c 1.1 86/09/25 SMI"; /* from UCB 6.7 3/11/86 */
#endif LIBC_SCCS and not lint

#include <sys/types.h>
#include <stdio.h>
#include <arpa/nameser.h>


/*
 * Expand compressed domain name 'comp_dn' to full domain name.
 * 'msg' is a pointer to the begining of the message,
 * 'eomorig' points to the first location after the message,
 * 'exp_dn' is a pointer to a buffer of size 'length' for the result.
 * Return size of compressed name or -1 if there was an error.
 */
dn_expand(msg, eomorig, comp_dn, exp_dn, length)
	char *msg, *eomorig, *comp_dn, *exp_dn;
	int length;
{
	register char *cp, *dn;
	register int n, c;
	char *eom;
	int len = -1;

	dn = exp_dn;
	cp = comp_dn;
	eom = exp_dn + length - 1;
	/*
	 * fetch next label in domain name
	 */
	while (n = *cp++) {
		/*
		 * Check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:
			if (dn != exp_dn) {
				if (dn >= eom)
					return (-1);
				*dn++ = '.';
			}
			if (dn+n >= eom)
				return (-1);
			while (--n >= 0) {
				if ((c = *cp++) == '.') {
					if (dn+n+1 >= eom)
						return (-1);
					*dn++ = '\\';
				}
				*dn++ = c;
				if (cp >= eomorig)	/* out of range */
					return(-1);
			}
			break;

		case INDIR_MASK:
			if (len < 0)
				len = cp - comp_dn + 1;
			cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			if (cp < msg || cp >= eomorig)	/* out of range */
				return(-1);
			break;

		default:
			return (-1);			/* flag error */
		}
	}
	*dn = '\0';
	if (len < 0)
		len = cp - comp_dn;
	return (len);
}

/*
 * Compress domain name 'exp_dn' into 'comp_dn'.
 * Return the size of the compressed name or -1.
 * 'length' is the size of the array pointed to by 'comp_dn'.
 * 'dnptrs' is a list of pointers to previous compressed names. dnptrs[0]
 * is a pointer to the beginning of the message. The list ends with NULL.
 * 'lastdnptr' is a pointer to the end of the arrary pointed to
 * by 'dnptrs'. Side effect is to update the list of pointers for
 * labels inserted into the message as we compress the name.
 * If 'dnptr' is NULL, we don't try to compress names. If 'lastdnptr'
 * is NULL, we don't update the list.
 */
dn_comp(exp_dn, comp_dn, length, dnptrs, lastdnptr)
	char *exp_dn, *comp_dn;
	int length;
	char **dnptrs, **lastdnptr;
{
	register char *cp, *dn;
	register int c, l;
	char **cpp, **lpp, *sp, *eob;
	char *msg;

	dn = exp_dn;
	cp = comp_dn;
	eob = cp + length;
	if (dnptrs != NULL) {
		if ((msg = *dnptrs++) != NULL) {
			for (cpp = dnptrs; *cpp != NULL; cpp++)
				;
			lpp = cpp;	/* end of list to search */
		}
	} else
		msg = NULL;
	for (c = *dn++; c != '\0'; ) {
		/* look to see if we can use pointers */
		if (msg != NULL) {
			if ((l = dn_find(dn-1, msg, dnptrs, lpp)) >= 0) {
				if (cp+1 >= eob)
					return (-1);
				*cp++ = (l >> 8) | INDIR_MASK;
				*cp++ = l % 256;
				return (cp - comp_dn);
			}
			/* not found, save it */
			if (lastdnptr != NULL && cpp < lastdnptr-1) {
				*cpp++ = cp;
				*cpp = NULL;
			}
		}
		sp = cp++;	/* save ptr to length byte */
		do {
			if (c == '.') {
				c = *dn++;
				break;
			}
			if (c == '\\') {
				if ((c = *dn++) == '\0')
					break;
			}
			if (cp >= eob)
				return (-1);
			*cp++ = c;
		} while ((c = *dn++) != '\0');
		/* catch trailing '.'s but not '..' */
		if ((l = cp - sp - 1) == 0 && c == '\0') {
			cp--;
			break;
		}
		if (l <= 0 || l > MAXLABEL)
			return (-1);
		*sp = l;
	}
	if (cp >= eob)
		return (-1);
	*cp++ = '\0';
	return (cp - comp_dn);
}

/*
 * Skip over a compressed domain name. Return the size or -1.
 */
dn_skip(comp_dn)
	char *comp_dn;
{
	register char *cp;
	register int n;

	cp = comp_dn;
	while (n = *cp++) {
		/*
		 * check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:		/* normal case, n == len */
			cp += n;
			continue;
		default:	/* illegal type */
			return (-1);
		case INDIR_MASK:	/* indirection */
			cp++;
		}
		break;
	}
	return (cp - comp_dn);
}

/*
 * Search for expanded name from a list of previously compressed names.
 * Return the offset from msg if found or -1.
 */
dn_find(exp_dn, msg, dnptrs, lastdnptr)
	char *exp_dn, *msg;
	char **dnptrs, **lastdnptr;
{
	register char *dn, *cp, **cpp;
	register int n;
	char *sp;

	for (cpp = dnptrs + 1; cpp < lastdnptr; cpp++) {
		dn = exp_dn;
		sp = cp = *cpp;
		while (n = *cp++) {
			/*
			 * check for indirection
			 */
			switch (n & INDIR_MASK) {
			case 0:		/* normal case, n == len */
				while (--n >= 0) {
					if (*dn == '\\')
						dn++;
					if (*dn++ != *cp++)
						goto next;
				}
				if ((n = *dn++) == '\0' && *cp == '\0')
					return (sp - msg);
				if (n == '.')
					continue;
				goto next;

			default:	/* illegal type */
				return (-1);

			case INDIR_MASK:	/* indirection */
				cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			}
		}
		if (*dn == '\0')
			return (sp - msg);
	next:	;
	}
	return (-1);
}

/*
 * Routines to insert/extract short/long's. Must account for byte
 * order and non-alignment problems. This code at least has the
 * advantage of being portable.
 */

u_short
getshort(msgp)
	char *msgp;
{
	register u_char *p = (u_char *) msgp;
#ifdef vax
	/*
	 * vax compiler doesn't put shorts in registers
	 */
	register u_long u;
#else
	register u_short u;
#endif

	u = *p++ << 8;
	return ((u_short)(u | *p));
}

u_long
getlong(msgp)
	char *msgp;
{
	register u_char *p = (u_char *) msgp;
	register u_long u;

	u = *p++; u <<= 8;
	u |= *p++; u <<= 8;
	u |= *p++; u <<= 8;
	return (u | *p);
}


putshort(s, msgp)
	register u_short s;
	register char *msgp;
{

	msgp[1] = s;
	msgp[0] = s >> 8;
}

putlong(l, msgp)
	register u_long l;
	register char *msgp;
{

	msgp[3] = l;
	msgp[2] = (l >>= 8);
	msgp[1] = (l >>= 8);
	msgp[0] = l >> 8;
}
