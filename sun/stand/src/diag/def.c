#ifndef lint
static        char sccsid[] = "@(#)def.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>
#include <sun/dkio.h>
#include <sys/dkbad.h>
#include "def.h"
#include "xyxd.h"

#define MAPTRK(x)	x >> 8
#define MAPSEC(x)	x & 8


struct defect_list defect[NDEFCT];

extern struct dkbad dkbad;
extern struct specs *specs[];
extern int hsect;
extern int silent;

#ifdef DEBUG
extern int startcyl;
extern int range;
#endif

int gotrunt;

/*
 * This routine build the defect list and save in the first 2 tracks
 * of the second alternate cylinder. It returns the number of defects
 * found on disk.
 */
int
rddeflist()
{
    struct tdef trkdef;
    int count = 0;
    register struct bt_bad *b = dkbad.bt_bad;

    if (rdtrkdef(0, 0, &trkdef) == FALSE) {
	/*
	 * from a formatted disk with the defect list saved in the
	 * first and second tracks of the second alternate cylinder.
	 */
	if ( (count = rddeflab()) != 0) {
#ifdef DEBUG
printf("FOR DEBUG ONLY: read disk with defect label\n");
#endif
	    ftlast2c(OLDISK, count);
	    putdef(count);
	    return(count);
	}

	/*
	 * from a formatted disk without the defect list saved.
	 */
#ifdef DEBUG
printf("FOR DEBUG ONLY: read slipped disk without the new label\n");
#endif
	count = rdslipsect();	/* get all slipped sectors */
	silent = 1;		/* disable errors messages in case we read new disk
				   without fujitsu encoded defect info */
	getbad(0);		/* get mapped sectors entries */
	while(b->bt_cyl != 0xffff) {
	    updatedef((int) b->bt_cyl, (int) (b->bt_trksec >> 8),
			(int) (b->bt_trksec & 0xff), -1, -1, &defect[count++]);
	    b++;
	}
	initdkbad();
	putbad();			/* mark map table empty */
	silent = 0;
	ftlast2c(OLDISK, count);	/* format the last 2 cylinders */
	putdef(count);
	return(count);
    } else {
#ifdef DEBUG
printf("FOR DEBUG ONLY: read new disk\n");
#endif
	count = rdnewdisk();	/* from a virgin disk. */
	ftlast2c(NEWDISK, count);
	putdef(count);
	return(count);
    }
}

/*
 * 	This routine build the defect list by reading the defect info
 * from the disk manufacturer. It return number of defects found.
 */
int
rdnewdisk()
{
    int cyl;
    int head;
    int i;
    int j;
    struct defect_list *dl = &defect[0];
    struct tdef trkdef;
    struct specs *sp;
    int printonce = 0;
    int count = 0;
    
    sp = &specs[controller][drive];

    printf("Reading defect from disk: \n");
#ifdef DEBUG
    for (cyl = startcyl; cyl < ncyl+acyl; cyl++) {
	if (cyl == startcyl + range)
		cyl = ncyl-1;
#else
    for (cyl = 0; cyl < ncyl+acyl; cyl++) {
#endif
	for (head = 0; head < nhead; head++) {
	    printf("\rcyl %d head %d ", cyl, head);
	    if ( rdtrkdef(cyl, head, &trkdef) == FALSE ) {
	       if (!printonce) {
		    printf("\nWarning! disk isn't completely encoded with def info\n");
		    printonce = 1;
	       }
	    } else 
		for (i = 0; i < 4; i++)	 /* update defect array */
		    if (trkdef.length[i] != 0) {
			j = trkdef.bfi[i]/sp->special;
			updatedef(cyl, head, j, (int) trkdef.bfi[i], (int) trkdef.length[i], dl++);
			count++;
			if (span(trkdef.bfi[i], trkdef.length[i],sp->special)) {
			    updatedef(cyl, head, j+1, (int) trkdef.bfi[i], (int) trkdef.length[i], dl++);
			    count++;
			}
		    }
	}
    }

    if (controller == C_XY450) {		/* adaptive format */
	/* 
	 * format cyl 0, trk 1 so we can find out if there is 
	 * any runt sector on this disk. use trk 1 so if someone
	 * do a control c after this, the disk wont be looked at 
	 * as if it has being formatted.
	 */
	(void) devcmd(FORMAT, 0, 1, 0, nsect);
	runtsect(0, 1);
	for (i = 0; i < count; i++)
	    defect[i].sect = adaptive(XSECT(i), XHEAD(i),
					(gotrunt ? hsect - 1 : hsect));
    }
    /*
     * correct the logical sector for track that have more than one defect
     * that are on seperate physical sector.
     */
    for (i = 1; i < count; i++)
	if (XCYL(i) == XCYL(i-1) && XHEAD(i) == XHEAD(i-1) && 
		XSECT(i) != XSECT(i-1) && defect[i].bfi != defect[i-1].bfi)
	    defect[i].sect = defect[i].sect - 1;

    return(count);
}

/*
 * 	This routine build the defect list by reading all track 
 * headers for slipped sectors.
 */
int
rdslipsect()
{
    int cyl;
    int head;
    int slipped[2];
    struct defect_list *dl = &defect[0];
    int count = 0;

    /*
     * read each track header to get slipped sectors.
     */
    if (doesithave(0, 0, HDR_SPARE))	/* disk is not set for slipping */
	return (0);
	
    printf("Looking for slipped sectors from disk: \n");
#ifdef DEBUG
    for (cyl = startcyl; cyl < ncyl+acyl; cyl++) {
	if (cyl == startcyl + range)
		cyl = ncyl-1;
#else
    for (cyl = 0; cyl < ncyl; cyl++) {
#endif
	for (head = 0; head < nhead; head++) {
	    printf("\rcyl %d head %d ", cyl, head);
	    slipped[0] = -1;
	    if (devcmd(READ_SLIP, cyl, head, slipped) != 0)
		return(0);
	    if (slipped[0] != -1) {
		updatedef(cyl, head, slipped[0], -1, -1, dl++);
		count++;
	    }
	}
    }
    return(count);
}

updatedef(cyl, head, sect, bfi, nbits, def)
    struct defect_list *def;
{
    def->cyl = (short) cyl;
    def->head = (short) head;
    def->sect = (short) sect;
    def->bfi = bfi;
    def->nbits = (short) nbits;
}

ftlast2c(disk, count)
{
    int slipped[40];
    int cyl;
    int head;
    int i;
    int *sp;

    switch (disk) {
	case OLDISK:
	    for (cyl = ncyl; cyl < ncyl+acyl; cyl++)
		for (head = 0; head < nhead; head++) {
		    slipped[0] = -1;
		    /*
		     * disable errors messages in case we read a disk
		     * formatted by a different controller.
		     */
		    silent = 1;
		    (void) devcmd(READ_SLIP, cyl, head, slipped);
		    silent = 0;

		    if (devcmd(FORMAT, cyl, head, 0, nsect) != 0) {
reverify:
			if ( (i = longverf(cyl, head)) == -1 )
			    printf("track %d error but can't find sector error in alternate cyl %d!!\n", head, cyl);
			else if (i == -2) {
			    printf("intermittent error on cyl %d, trk %d\n",
					cyl, head);
			} else {
			    if (devcmd(SLIP, cyl, head, i) != 0) {
				printf("slip failed in alternate cyl %d, head %d\n", cyl, head);
				printf("WARNING!! Disk is unusable!!!\n");
			    } else
				goto reverify;
			}
		    }
		    for (sp = slipped; *sp != -1; sp++) {
#ifdef DEBUG
			printf("reslipping %d/%d/%d ", cyl, head, *sp);
#endif
			if (devcmd(SLIP, cyl, head, *sp) == 0)
			    printf("\n");
			else
			    printf("[failed]\n");
		    }
		}
	    break;

	case NEWDISK:
	    /*
	     * format the last 2 cylinders. slipped the first defect 
	     * spot on any track. We take a calculated risk here that
	     * it is a rare case that you have 2 defects on one track
	     * and even if we do only 10% of defect spots from the
	     * disk manufacturer show up being "really defective sectors".
	     */
	    for (cyl = ncyl; cyl < ncyl+acyl; cyl++)
		for (head = 0; head < nhead; head++) 
		    (void) devcmd(FORMAT, cyl, head, 0, nsect);
	    
	    for (i = 0; i < count; i++)
		if (defect[i].cyl >= ncyl)
		    (void) devcmd(SLIP, XCYL(i), XHEAD(i), XSECT(i));
	    break;

	default:
	    break;
    }
}

/*
 *  This routine printed the defect list.
 */
prdef(pflag, count)
{
    int i = 0;
    int j = 0;
    int odef = 0;			/* original defect count */

    printf("\n");
    for (i = 0; i < count; i++) {
	if (pflag) {
	    printf("cyl %d, head %d, sect %d, ",
		    defect[i].cyl, defect[i].head, defect[i].sect);
	    if (defect[i].bfi == -1)
		printf("bfi unknown, length unknown\n");
	    else
		printf("bfi %d, length %d\n", defect[i].bfi, defect[i].nbits);
	}
	if (defect[i].bfi != -1) {
	    if ( !(defect[i+1].cyl == defect[i].cyl &&
		    defect[i+1].head == defect[i].head &&
		    defect[i+1].nbits == defect[i].nbits &&
		    defect[i+1].bfi == defect[i].bfi) )
		odef++;
	}

	if (pflag) {
	    j++;
	    if (j == 20) {
		j = 0;
		printf("stop? ");
		if (confirm())
		    pflag = 0;
	    }
	}
    }
    printf("total defects %d with %d original defects from disk manufacturer\n", i, odef);
}

/*
 * This routine find out if there is a runt sector on disk.
 * This is needed for defect calculation because the 450/451
 * included runt sector in the total number of h/w they reported
 * in a read drive status command. 
 */
runtsect(cyl, head)
{
    struct tdef trkdef;

    /*
     * can only do this for formatted track. 
     */
    if (rdtrkdef(cyl, head, &trkdef) == FALSE) 
	if (doesithave(cyl, head, HDR_RUNT) == 0)
	    gotrunt = 1;
}
