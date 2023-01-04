#ifndef lint
static	char sccsid[] = "@(#)rdwr.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif
 
#include <stdio.h>

fread(ptr, size, count, iop)
        unsigned size, count;
        register char *ptr;
        register FILE *iop;
{
        register int c;
        unsigned ndone, s;
        register bytes= size*count;

        if (bytes <= 0) return(count);
        if (bytes <= iop->_cnt) {
                bcopy(iop->_ptr, ptr, bytes);   /* just move stuff to buffer */
                iop->_ptr+= bytes;
                iop->_cnt-= bytes;
                return(count);};
        for (ndone = 0; ndone<count; ndone++) {
                s = size;
                do {
                        if ((c = getc(iop)) >= 0)
                                *ptr++ = c;
                        else
                                return(ndone);
                } while (--s);
        }
        return(ndone);
}

fwrite(ptr, size, count, iop)
        unsigned size, count;
        register char *ptr;
        register FILE *iop;
{
        register unsigned s;
        unsigned ndone;
        register bytes= size*count;

        if (bytes <= 0) return(count);
        if (bytes <= iop->_cnt) { 
                bcopy(ptr, iop->_ptr, bytes);   /* just move stuff to buffer if it fits */
                iop->_ptr+= bytes;
                iop->_cnt-= bytes;
                return(count);};
        for (ndone = 0; ndone<count; ndone++) {
                s = size;
                do {
                        putc(*ptr++, iop);
                } while (--s);
                if (ferror(iop))
                        break;
        }
        return(ndone);
}
