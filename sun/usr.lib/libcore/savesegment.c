#ifndef lint
static char sccsid[] = "@(#)savesegment.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"
#include <sys/file.h>
#include <stdio.h>
#define NULL 0
#define OLDSEGMENT 0
#define NEWSEGMENT 1

/*
	These routines save and restore a SunCore segment on disk file.
*/

/* need this in case we restore an old segment */
typedef struct {		/* 1.1 Segment descriptor */
   int segname;
   short type;				/* noretain,retain,xlate,xform */
   segattr segats;
   int vsurfnum;			/* numbr of view surfs to display */
   viewsurf *vsurfptr[MAXVSURF];
   int pdfptr;				/* byte ptr into pseudo display file */
   int redraw;				/* true if segment needs redraw */
   int segsize;
   } oldsegstruc;

#define SEGBUFSZ	1024
/*-------------------------------------------------------*/
save_segment( segnum, filename) int segnum; char *filename;
{
    int segfid, i;
    segstruc *sptr;
    int errnum;
    char *funcname;
    char *buf;
    short *dummy;
	
    errnum = 0;
    if( (segfid = open( filename, O_WRONLY|O_CREAT|O_TRUNC, PMODE)) == -1) {
	fprintf(stderr, "save_segment: can't open %s\n",filename);
	return(1);
	}
    _core_critflag++;
							/* copy the segment */
    for (sptr = &_core_segment[0]; sptr < &_core_segment[SEGNUM]; sptr++) {
        if((sptr->type != DELETED) && (segnum == sptr->segname)) {
	    if (sptr == _core_openseg) {		/* seg must be closed */
		errnum = 1; break;
		}
	    write( segfid, "SEGM", 4);			/* write file id */
	    write( segfid, sptr, sizeof( *sptr));	/* write seg header */
	    buf = (char*)malloc( SEGBUFSZ);
	    _core_pdfseek( sptr->pdfptr, 0, &dummy);	/* write seg data */
	    for (i=sptr->segsize*2; i>=SEGBUFSZ; i-=SEGBUFSZ) {
		_core_pdfread( SEGBUFSZ, buf );
		write( segfid, buf, SEGBUFSZ );
	        }
	    _core_pdfread( i, buf);
	    write (segfid, buf, i);
	    free( buf);
	    break;
	    }
	if (sptr->type == EMPTY) {
	    funcname = "save_segment";  errnum = 29;
	    _core_errhand(funcname,errnum); break;
	    }
	}
    if (sptr == &_core_segment[SEGNUM])
	{
	funcname = "save_segment";
	errnum = 29;
	_core_errhand(funcname,errnum);
	}
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
    close( segfid);					/* close disk file */
    return(errnum);
}
/*-------------------------------------------------------*/
restore_segment( segname, filename) int segname; char *filename;
{
    int segfid, i;
    segstruc *sptr;
    oldsegstruc *dummyptr;
    viewsurf *surfp;
    char *buf, segfileid[4];
    char *funcname;
    int visible_flag;
    int errnum;
    int segment_type;
	
    errnum = 0;
    funcname = "restore_segment";
    if (!segname) {				/* seg number can't be 0 */
        _core_errhand(funcname, 29); return(1);
        }
    if (_core_osexists) {			/* a segment is already open */
        _core_errhand(funcname,8); return(1);
        }
	
    if( (segfid = open( filename, O_RDONLY, PMODE)) == -1) {
	fprintf(stderr, "restore_segment: can't open %s\n",filename);
	return(1);
	}
    read( segfid, segfileid, 4);			/* read file id */
    if (segfileid[0] == 'S' && segfileid[1] == 'E' &&
	segfileid[2] == 'G' && segfileid[3] == 'M') {
            segment_type=NEWSEGMENT;
        }
    else if (segfileid[0] == 'S' && segfileid[1] == 'U' &&
	     segfileid[2] == 'N' && segfileid[3] == 'C') {
                 segment_type=OLDSEGMENT;
        }
        else {
            fprintf(stderr, "restore_segment: %s is not a segment file\n",filename);
            close( segfid);
            return( 1);
            }

    /*
     * create the new segment from disk file 
     */

    for (sptr = &_core_segment[0]; sptr < &_core_segment[SEGNUM]; sptr++) {
        if (sptr->type == EMPTY) {
	    if (_core_openseg == NULL)
		_core_openseg = sptr;		/* find available segstruc */
	    break;
	    }
        else if (sptr->type == DELETED) {
	    if (_core_openseg == NULL)
		_core_openseg = sptr;		/* find available segstruc */
	    }
        else if (segname == sptr->segname) {
	    _core_errhand(funcname,31);
	    _core_openseg = NULL;
	    close( segfid); return(2);
	    }
        }
    if (_core_openseg == NULL)
	{
	_core_errhand(funcname, 16);
	return(16);
	}
    _core_critflag++;
    errnum = 33;			/* assume error */
    _core_openseg->vsurfnum = 0; 	/* zero number of selected view surfs */
    for(surfp = &_core_surface[0];surfp < &_core_surface[MAXVSURF];surfp++) {
        if (surfp->selected) {
	    errnum = 0;			/* no error because one is selected */
	    		/* Put pointers to selected viewsurfaces in segment. */
	    _core_openseg->vsurfptr[(_core_openseg->vsurfnum)++] = surfp;
	    }
        }
    if (errnum == 33) {			/* no surfaces are selected */
        _core_errhand(funcname,errnum);
	_core_openseg = NULL;
	close( segfid);
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	return(4);
        }
    _core_ndcset |= 1;
    _core_openseg->segname = segname;

    /*
     * SET SEGMENT DYNAMIC ATTRIBUTES FROM DISK FILE SEGMENT ATTRIBUTES 
     */
    buf = (char*)malloc( SEGBUFSZ);
    sptr = (segstruc*)buf;
    if (segment_type == NEWSEGMENT) {
        read( segfid, sptr, sizeof( *sptr));	/* read newseg header */
        }
    else {
        read( segfid, sptr, sizeof( *dummyptr ));	/* read oldseg header */
        }

    _core_openseg->type = sptr->type;
    _core_openseg->segats.visbilty = sptr->segats.visbilty;
    if (_core_openseg->segats.visbilty)
	visible_flag = TRUE;
    else
	visible_flag = FALSE;
    _core_openseg->segats.detectbl = sptr->segats.detectbl;
    _core_openseg->segats.highlght = sptr->segats.highlght;
    _core_openseg->segats.scale[0] = sptr->segats.scale[0];
    _core_openseg->segats.scale[1] = sptr->segats.scale[1];
    _core_openseg->segats.scale[2] = sptr->segats.scale[2];
    _core_openseg->segats.translat[0] = sptr->segats.translat[0];
    _core_openseg->segats.translat[1] = sptr->segats.translat[1];
    _core_openseg->segats.translat[2] = sptr->segats.translat[2];
    _core_openseg->segats.rotate[0] = sptr->segats.rotate[0];
    _core_openseg->segats.rotate[1] = sptr->segats.rotate[1];
    _core_openseg->segats.rotate[2] = sptr->segats.rotate[2];
    _core_setmatrix(_core_openseg);
    _core_openseg->pdfptr = _core_pdfwrite(0, buf);	/* next pdf position */
    _core_openseg->redraw = TRUE;
    _core_openseg->segsize = sptr->segsize;
    if( segment_type == NEWSEGMENT) { /* restore segment bndbox if newseg */
        _core_openseg->bndbox_min.x = sptr->bndbox_min.x;
        _core_openseg->bndbox_min.y = sptr->bndbox_min.y;
        _core_openseg->bndbox_min.z = sptr->bndbox_min.z;
        _core_openseg->bndbox_max.x = sptr->bndbox_max.x;
        _core_openseg->bndbox_max.y = sptr->bndbox_max.y;
        _core_openseg->bndbox_max.z = sptr->bndbox_max.z;
        }
    else { /* set up segment bndbox to be max dimensions if od type of seg */
        _core_openseg->bndbox_min.x = -MAX_NDC_COORD;
        _core_openseg->bndbox_min.y = -MAX_NDC_COORD;
        _core_openseg->bndbox_min.z = -MAX_NDC_COORD;
        _core_openseg->bndbox_max.x =  MAX_NDC_COORD;
        _core_openseg->bndbox_max.y =  MAX_NDC_COORD;
        _core_openseg->bndbox_max.z =  MAX_NDC_COORD;
        }
					/* transfer seg data */
    for (i=(_core_openseg->segsize-1)*2; i>=SEGBUFSZ; i-=SEGBUFSZ) {
	read( segfid, buf, SEGBUFSZ );
	_core_pdfwrite( SEGBUFSZ, buf );
	}
    read( segfid, buf, i);
    _core_pdfwrite( i, buf);		/* seg remainder without PDFENDSEG */
    free( buf);

    /*
     * SET SEGMENT STATE AND OUTPUT PRIMITIVE FLAGS
     */

    _core_prevseg =   TRUE;
    _core_osexists =  TRUE;
    _core_segnum++;
    close_retained_segment();

    if (visible_flag) {
	set_segment_visibility(segname,FALSE);
	set_segment_visibility(segname,TRUE);
    }

    close( segfid);				/* close the file */
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
    return(0);
}
