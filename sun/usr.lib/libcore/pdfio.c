#ifndef lint
static char sccsid[] = "@(#)pdfio.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <stdio.h>

#define	PDFCHUNK	32768		/* words in PDF chunk */

		/*** PDF VARIABLES ***/

static short *pdfstart, *pdfend;	/* pointers to start and end of PDF */
static int   pdfptr, pdfnext;		/* PDF read ptr and write ptr */

/*----------------------------------------------------------------------*/
int _core_pdfread( bytecnt, data) int bytecnt; register short *data;
{		/* read count words from the PDF */
    register short *ptr;
    register int i;

    bytecnt >>= 1;			/* convert byte count to words */
    ptr = &(pdfstart[pdfptr]);
    if ( pdfptr+bytecnt > pdfnext) {
	*data = PDFENDSEGMENT;
	return(pdfptr);
	}
    for (i=0; i<bytecnt; i++)  *data++ = *ptr++;
    pdfptr += bytecnt;
    return(pdfptr);
}
/*----------------------------------------------------------------------*/
int _core_pdfwrite( bytecnt, data) int bytecnt; register short *data;
{	/* append count words to the PDF, count must be <PDFCHUNK */

    register short *ptr;
    register int i;

    bytecnt >>= 1;			/* convert byte count to words */
    ptr = &(pdfstart[pdfnext]);
    if ( ptr+bytecnt >= pdfend) {
	short *newstart;
	int i, size;
	newstart = (short*)realloc( pdfstart,
		(pdfend-pdfstart+bytecnt+PDFCHUNK+1)<<1 );
	if ( newstart) {
		size = pdfend-pdfstart + bytecnt + PDFCHUNK;
		pdfstart = newstart;
		pdfend = pdfstart + size;
    		ptr = &(pdfstart[pdfnext]);
	} else {
		fprintf( stderr,
		"Display list overflow; delete segments before adding more!\n");
		return(1);
	}
    }
    for (i=0; i<bytecnt; i++)  *ptr++ = *data++;
    pdfnext += bytecnt;
    return(pdfnext);
}
/*----------------------------------------------------------------------*/
int _core_pdfskip( bytenum) int bytenum;
{
    pdfptr += (bytenum>>1);
    return(pdfptr);
}
/*----------------------------------------------------------------------*/
int _core_pdfseek( wordnum, mode, dataptr)  int wordnum, mode;  short **dataptr;
{		/* read data into buffers and return ptr into buffer    */
    if (mode) {
	*dataptr = &(pdfstart[pdfptr+wordnum]);
        pdfptr += wordnum;
	}
    else {
	*dataptr = &(pdfstart[wordnum]);
	pdfptr = wordnum;
	}
    return(pdfptr);
}
/*----------------------------------------------------------------------*/
int _core_pdfmarkend()
{   		/* mark temporary end of segment and backup */
    short ptype = PDFENDSEGMENT;

    _core_pdfwrite (SHORT, &ptype);
    pdfnext--;
}
/*----------------------------------------------------------------------*/
_core_PDFinit()
{
    int i, words;
    pdfptr = 0;
    pdfnext = 0;
    words = PDFCHUNK;
    for (i=0; i<3; i++) {
	pdfstart = (short*)malloc( words*sizeof(short)); /* word  array */
	pdfend = pdfstart + words;
	if ( pdfstart) break;
	else {
	    fprintf( stderr,
	    "Insufficient disk pages for %D word virtual display list.\n",
	    words);
	    words >>= 1;
	    }
	}
    return((int)pdfstart);
}
/*--------------------------------------------------------------------*/
_core_PDFclose()
{
    if (pdfstart) free( pdfstart);
}
/*--------------------------------------------------------------------*/
_core_PDFcompress(segptr)				/* compress the PDF */
segstruc *segptr;
	{
	int i;
	short *ptr1, *ptr2;
	segstruc *sptr;

	if (segptr == (segstruc *) 0)
		{
		pdfnext = 0;
		return(0);
		}
	ptr1 = &(pdfstart[segptr->pdfptr]);
	ptr2 = ptr1 + segptr->segsize;
	for (i=0; i<(pdfnext-(segptr->pdfptr+segptr->segsize)); i++)
		*ptr1++ = *ptr2++;
	pdfnext -= segptr->segsize;
        for (sptr = &_core_segment[0]; sptr < &_core_segment[SEGNUM]; sptr++) {
		if (sptr->type == EMPTY) break;
		if (sptr->pdfptr > segptr->pdfptr)
		sptr->pdfptr -= segptr->segsize;
		}
	return(0);
	}
