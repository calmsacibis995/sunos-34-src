
#ifndef lint
static	char sccsid[] = "@(#)defutil.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>
#include "def.h"

struct shift left[4] = {
    0x01, 7,
    0x02, 5,
    0x04, 3,
    0x08, 1
};
struct shift right[4] = {
    0x10, 1,
    0x20, 3,
    0x40, 5,
    0x80, 7
};

extern int hsect;
extern int silent;
extern struct defect_list defect[];

/*
 * 	This routine reads the track defect info encoded by the drive
 * manufacturer. return FALSE if it can't find it.
 */
int
rdtrkdef(cyl, head, def)
    int cyl;
    int head;
    struct tdef *def;
{
    if (devcmd(READ_DEFECT, cyl, head, 0, -1, DBUF_PA))
	return(FALSE);
    else
	return(extract_def(def));
}

/*
 * This routine extract bad spot of each track  from the 19 bytes 
 * returned by the READ_DEFECT cmd. If the encode defect header is
 * valid, it returned the defect entries.
 */ 
int
extract_def(def)
    struct tdef *def;
{
    int bytefi;
    int length;
    int i;
    struct dformat *defptr;

    defptr = (struct dformat *) DBUF_VA;
    if (defptr->byt0 != SYNC_BYTE && defptr->byt21 != LAST_BYTE)
	return(FALSE);
    
    for (i = 0; i < 4; i++) {
	bytefi = 0;
	length = 0;
	switch (i) {
	case 0:
	    bytefi = BYTEFI(defptr->byt5, defptr->byt6);
	    length = DLENGHT(defptr->byt7, defptr->byt8);
	    break;

	case 1:
	    bytefi = BYTEFI(defptr->byt9, defptr->byt10);
	    length = DLENGHT(defptr->byt11, defptr->byt12);
	    break;

	case 2:
	    bytefi = BYTEFI(defptr->byt13, defptr->byt14);
	    length = DLENGHT(defptr->byt15, defptr->byt16);
	    break;

	case 3:
	    bytefi = BYTEFI(defptr->byt17, defptr->byt18);
	    length = DLENGHT(defptr->byt19, defptr->byt20);
	    break;

	default:
	    break;
	}
	def->bfi[i] = bytefi;
	def->length[i] = length;
	/*
	if (bytefi) {
	    printf("\n bfi %d with length %d\n\n", bytefi, length);
	}
	*/
    }
    return(TRUE);
}

/*
 * This routine reversed the bits of the defect info read
 * from the drive. Courtesy of Xylogics hardware.
 */
reversed()
{
    int i;
    u_short j;
    u_char x;
    u_char *cptr;
    u_short *ptr;

    cptr = (u_char *) DBUF_VA;
    for (j = 0; j < HDR_BYTES; j++) {
	x = 0;
	for (i = 0; i < 8; i++)
	    x = x | ( (*cptr & right[i].mask) >> right[i].bits );
	for (i = 0; i < 4; i++)
	    x = x | ( (*cptr & left[i].mask) << left[i].bits );
	*cptr++ = x;
    }

    ptr = (u_short *) DBUF_VA;
    for (i = 0; i < (HDR_BYTES/2); i++) {
	j = *ptr;		
	*ptr++ = ( ((j & 0xff00) >> 8) | ((j & 0xff) << 8) );
    }
}

/*
 *	This routine read back the saved defect entries that has being slipped.
 * it return the defect count.
 */
#define XD_WPAR		0x500	/* write params */
#define XD_DRIVE	0x80	/* drive parameters */
int
rddeflab()
{
    struct defhdr *sdef = (struct defhdr *)DBUF_VA;
    int count;
    int checksum;
    int i;
    int j;
    struct defect_list *dl;

    /*
     * read the control block at ncyl/0/0.
     */
    silent = 1;
    for (i = 0; i < 2; i++)
	if ( devcmd(READ, ncyl+1, i, 0, 1, DBUF_PA) == 0 ) {
	    if (sdef->magicno == DISKMAGIC) {
		checksum = sdef->chksum;
		if ( (count = sdef->count) == 0) {
		   /* printf("Warning!! count in defect label is 0\n"); */
		    silent = 0;
		    return(0);
		} else
		    if (devcmd(READ,ncyl+1, i, 1,DEFSECT(count),DBUF_PA) == 0) {
			if (checksum != chksum((int *)DBUF_VA, count*3)) {
			    printf("Warning! checksum checking in defect list failed..\n");
			    continue;
			}
			dl = (struct defect_list *)DBUF_VA;
			for (j = 0; j < count; j++)
			    defect[j] = *dl++;
		        silent = 0;
			return(count);
		    }
	    }
	} else {
	    /* 
	     * if controller is 751, then try 450 compatible mode
	     */
	    if (controller == C_XY751) {
		if ( nsect >= 64 )
		    (void) XDexec(XD_WPAR, ncyl+acyl-1, nhead-1, 63, 6, 0, XD_DRIVE);
		else
		    (void) XDexec(XD_WPAR, ncyl+acyl-1, nhead-1, nsect-1, 6, 0, XD_DRIVE);
		if ( devcmd(READ, ncyl+1, i, 0, 1, DBUF_PA) == 0 ) {
		    if (sdef->magicno == DISKMAGIC) {
			checksum = sdef->chksum;
			if ( (count = sdef->count) == 0) {
			    silent = 0;
			    (void) XDexec(XD_WPAR, ncyl+acyl-1, nhead-1, nsect-1, 0, 0, XD_DRIVE);
			    return(0);
			} else
			    if (devcmd(READ,ncyl+1, i, 1,DEFSECT(count),DBUF_PA) == 0) {
				(void) XDexec(XD_WPAR, ncyl+acyl-1, nhead-1, nsect-1, 0, 0, XD_DRIVE);
				if (checksum != chksum((int *)DBUF_VA, count*3)) {
				    printf("Warning! checksum checking in defect list failed..\n");
				    continue;
				}
				dl = (struct defect_list *)DBUF_VA;
				for (j = 0; j < count; j++) {
				    defect[j] = *dl++;
				    defect[j].sect = (defect[j].sect + defect[j].head) % nsect;
				}
				silent = 0;
				return(count);
			    }
		    }
		}
		(void) XDexec(XD_WPAR, ncyl+acyl-1, nhead-1, nsect-1, 0, 0, XD_DRIVE);
	    }
	}
    silent = 0;
    return(0);
}

/* 
 * this routine write out 2 copies of defect list in the first 2 tracks
 * of the second alternate cylinders.
 */
putdef(count)
{
    struct defhdr *s;
    struct defect_list *to;
    struct defect_list *from;
    int i;
    int head;

    s = (struct defhdr *) DBUF_VA;

    bzero((char *)s, sizeof (*s));
    for (head = 0; head < 2; head++) {
	s->count = count;
	s->magicno = DISKMAGIC;
	s->chksum = chksum((int *) &defect[0], count*3 ); /* 12 byt/defect */

	from = &defect[0];
	to = (struct defect_list *) (DBUF_VA + sizeof(*s));
	for (i = 0; i < count; i++) 
	    *to++ = *from++;

	if (devcmd(WRITE, ncyl+1, head, 0, DEFSECT(count)+1, DBUF_PA) != 0) {
	    s->magicno = -1;	/* -1 for defective defect label */
	    (void) devcmd(WRITE, ncyl+1, head, 0, 1, DBUF_PA);
	}
    }
}

int
chksum(l, count)
    int *l;
    int count;
{
    int *sp, sum = 0;

    sp = l;
    while (count--) 
	sum ^= *sp++;
    return(sum);
}

/*
 *     This routine build the original defect list from the disk by
 * getting rid of all entries that have length of -1. It assumes that
 * that the defect array start with original defects.
 */
originaldef()
{
    int i;
    int j = 0;
    int count;

    if ((count = i = rddeflab()) != 0) {
	while ( j < i ) {
	    if (defect[j].nbits == -1) 
		count--;
	    j++;
	}
	putdef(count);
    }
}

/*
 * this routine add one entry to the defect list stored 
 * at the first and second track of the second alternate cylinder.
 */
addonedef(cyl, head, sect)
{
    int i;

    if ((i = rddeflab()) != 0) {
	defect[i].cyl = cyl;
	defect[i].head = head;
	defect[i].sect = sect;
	defect[i].nbits = -1;
	defect[i].bfi = -1;
	putdef(i+1);
    }
}
