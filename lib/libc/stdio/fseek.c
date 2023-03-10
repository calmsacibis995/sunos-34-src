#ifndef lint
static	char sccsid[] = "@(#)fseek.c 1.1 86/09/24 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
/*
 * Seek for standard library.  Coordinates with buffering.
 */
#include <stdio.h>

extern long lseek();
extern int fflush();

int
fseek(iop, offset, ptrname)
register FILE *iop;
long	offset;
int	ptrname;
{
	register int resync, c;
	long	p = -1;			/* can't happen? */

	iop->_flag &= ~_IOEOF;
	if(iop->_flag & _IOREAD) {
		if(ptrname < 2 && iop->_base && !(iop->_flag&_IONBF)) {
			c = iop->_cnt;
			p = offset;
			if(ptrname == 0) {
				long curpos = lseek(fileno(iop), 0L, 1);
				if (curpos == -1)
					return (-1);
				p += c - curpos;
				resync = offset&01;
			} else {
				offset -= (long)c;
				resync = 0;
			}
			if(!(iop->_flag&_IORW) && c > 0 && p <= c &&
					p >= iop->_base - iop->_ptr) {
				iop->_ptr += (int)p;
				iop->_cnt -= (int)p;
				return(0);
			}
		} else 
			resync = 0;
		if(iop->_flag & _IORW) {
			iop->_ptr = iop->_base;
			iop->_flag &= ~_IOREAD;
			resync = 0;
		}
		p = lseek(fileno(iop), offset-resync, ptrname);
		iop->_cnt = 0;
		if (resync && p != -1)
			if (getc(iop) == EOF)
				p = -1;
	} else if(iop->_flag & (_IOWRT | _IORW)) {
		p = fflush(iop);
		iop->_cnt = 0;
		if(iop->_flag & _IORW) {
			iop->_flag &= ~_IOWRT;
			iop->_ptr = iop->_base;
		}
		return(lseek(fileno(iop), offset, ptrname) == -1 || p == EOF ?
		    -1 : 0);
	}
	return((p == -1)? -1: 0);
}
