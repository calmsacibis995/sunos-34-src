#ifndef lint
static	char sccsid[] = "@(#)nft.c 1.1 86/09/25";
#endif

#include "diag.h"
#include <sys/types.h>
#include <sun/dkio.h>
#include <sys/dkbad.h>
#include "def.h"
#include "xyxd.h"

#define PATTERNS 5
#define DELETE 0
#define CHANGE 1
#define ADD    2

extern int hsect;
extern struct defect_list defect[];
extern struct dkbad dkbad;
extern struct specs *specs[];
extern int gotrunt;

int scandata[] = {
	0xc6dec6de,
	0x6db6db6d,
	0x00000000,
	0xffffffff,
	0xaaaaaaaa,
/*
	0xc6dec6de,
	0x6db6db6d,
	0xaaaaaaaa,
	0x92492492,
	0x24924924,
*/
};

char *msg1 = "MUST DO FORMAT COMMAND FOR THESE ENTRIES TO BE USED\n";

struct fmtcmd {
	char *cmdname;
} fmtsel[] = {
	"list defect list saved on disk",
	"format only with original disk defects",
	"manual entry for defect list",
	"delete manual defect entry",
	"change manual defect entry",
	"add manual defect entry",
	"exit sformat",
	0
};

#ifdef DEBUG
int startcyl = 0;
int range;
#endif

/* 
 * this print out a selection of smd disk format options
 * for the Xylogics controller family.
 */
C_sformat()
{
    int i, j, k;
    int sb;
    int eb;
    int bn;
    int cyl;
    int head;
    int cmd;
    struct tdef trkdef;
    register int *dp = (int *)DBUF_VA;
    register int nw = nsect * SECSIZE / sizeof (int);

    if (controller != C_XY450 && controller != C_XY751) {
	printf(" This cmd is only available for the Xylogics 450/451\n");
	return;
    }

    for (;;) {
    do {
	    printf("\n\n");
	    printf("specify format:\n");
	    for (i = 0; fmtsel[i].cmdname != 0; i++)
		    printf("\t%d - %s\n", i, fmtsel[i].cmdname);
	    j = pgetn("which one? ");
	    if (j >= 89 && j <= 93)
		break;
    } while (j < 0 || j >= i);

    switch (j) {
	case 0:
	    if ( (i = rddeflab()) != 0) 
		prdef(1, i);
	    else
		printf(" No defect list is found on this disk! \n");
	    break;
	case 1:
	    printf("WARNING! defect list will be truncated to original manufacturer's list\n");
	    printf("are you sure? ");
	    if (!confirm())
		break;
	    originaldef();
	    fmtdk(1);
	    break;
	case 2:
	    if ((i = manualdef()) != 0) {
		prdef(1, i);
		printf("\n%s", msg1);
		ftlast2c(NEWDISK, i);
		putdef(i);
	    }
	    break;
	case 3:
	    cmd = DELETE;
	    if ((i = rddeflab()) != 0) {
		if (chgdef(cmd, i) != -1) { 
		    i--;
		    putdef(i);
		}
	    } else
		printf(" There is no existing defect list to delete\n");
	    break;
	case 4:
	    cmd = CHANGE;
	    if ((i = rddeflab()) != 0) {
		if (chgdef(cmd, i) != -1) 
		    putdef(i);
	    } else
		printf(" There is no existing defect list to change\n");
	    break;
	case 5:
	    cmd = ADD;
	    if ((i = rddeflab()) != 0) {
		if ((k = chgdef(cmd, i)) != -1) 
		    putdef(i + k);
	    } else
		printf(" There is no existing defect list to add\n");
	    break;
	case 6:
	    return;
	case 89:			/* zap defect list */
	    printf(" zap defect list\n");
	    initbuf(dp, nw, 0x12345678);
	    (void) devcmd(WRITE, ncyl+1, 0, 0, nsect, DBUF_PA); 
	    (void) devcmd(WRITE, ncyl+1, 1, 0, nsect, DBUF_PA); 
	    break;
	case 90:			/* zap map list */
	    printf(" zap map table\n");
	    initbuf(dp, nw, 0x12345678);
	    (void) devcmd(WRITE, (ncyl+acyl-1), nhead-1, 0, nsect, DBUF_PA); 
	    break;
	case 91:	 /* format from cyl/trk to cyl/trk (without slip/map) */
	    sb = pgetbn("starting track? ");
	    eb = pgetbn("ending track? ");
	    for (bn = sb; bn < eb; bn += nsect) {
		cyl = bn / (nhead*nsect);
		head = (bn % (nhead*nsect)) / nsect;
		printf("\rcyl %d head %d  ", cyl, head);
		(void) devcmd(FORMAT, cyl, head, 0, nsect);
	    }
	    break;
	case 92: /* format disk ignoring all previously slipped and mapped sect */
	    ftlast2c(OLDISK, 0);
	    putdef(0);
	    fmtdk(0);
	    break;
	case 93:	 /* read defect from a brand new drive */
	    if (rdtrkdef(0, 0, &trkdef) != FALSE) {
		i = rdnewdisk();
		prdef(1, i);
	    } else
		printf("can't read defect from this drive\n");
	    break;

	default:
	    break;
    }
    }
}

/*
 * this routine return -1 if the sector in question is a mapped one
 * else it return a 0.
 */
int
isitmap(cyl, head, sect)
{
    int i;

    for (i = 0; i < NBAD; i++) {
	if (dkbad.bt_bad[i].bt_cyl == 0xffff)
	    return (0);
	if (dkbad.bt_bad[i].bt_cyl == cyl
		&& (dkbad.bt_bad[i].bt_trksec >> 8) == head
		&& (dkbad.bt_bad[i].bt_trksec & 0xff) == sect)
	    return (-1);
    }
    return (0);
}

/*
 * given an index into the "defect" array it return the logical sector
 * number based on the bfi information.
 */
short
lsect(ind)
    int ind;
{
    struct specs *sp = &specs[controller][drive];
    short i;

    i = defect[ind].bfi/sp->special;
    if (controller == C_XY450)		/* adaptive format */
	i = adaptive(i, XHEAD(ind), (gotrunt ? hsect - 1 : hsect));
    return(i);
}

/*
 * This is the ultimate smd format routine.
 */
fmtdk(flag)
{
    int i;
    int ind = 0;		/* index into the mapped sectors table */
    int cyl;
    int head;
    int moredef = 0;
    int count;			/* count of defects */
    jmp_buf save_jmp;

    fmtinit();			

    if (flag)
	count = rddeflist();	/* get all defects info */
    else
	count = 0;

    /*
     * format entire disk.
     */
    printf("\n Formatting \n");
#ifdef DEBUG
    for (cyl = startcyl; cyl < ncyl; cyl++) {
	if (cyl == startcyl + range)
		cyl = ncyl-1;
#else
    for (cyl = 0; cyl < ncyl; cyl++) {
#endif
	for (head = 0; head < nhead; head++) {
	    printf("\r cyl %d head %d ", cyl, head);
	    (void) devcmd(FORMAT, cyl, head, 0, nsect);
	}
    }
    printf("\n");

    /*
     * slipped/mapped defective sectors.
     */
    for (i = 0; i < count; i++) {
	if (XCYL(i) >= cyl)
	    break;
	if (i != 0)
	    if ( XCYL(i) == XCYL(i-1) && XHEAD(i) == XHEAD(i-1) &&
		     XSECT(i) == XSECT(i-1) && lsect(i) == XSECT(i-1))
		if (isitmap((int) XCYL(i), (int) XHEAD(i), (int) XSECT(i)))
		     continue; 
	if (devcmd(SLIP, XCYL(i), XHEAD(i), XSECT(i)) != 0) {
	    printf("mapped cyl %d, head %d, sect %d\n",
				XCYL(i), XHEAD(i), XSECT(i));
	    (void) mapsector(XCYL(i), XHEAD(i), XSECT(i), ind++);
	} else
	    printf("slipped cyl %d, head %d, sect %d\n", XCYL(i), XHEAD(i), XSECT(i));
    }
    for (i = 0; i < sizeof save_jmp/sizeof save_jmp[0]; i++)
	    save_jmp[i] = abort_jmp[i];

    if (_setjmp(abort_jmp)) 
	     goto ftdone;
    

    /*
     * verify by doing a read, map if found error.
     * short verify for previously slipped/mapped disk and a long
     * verify for unslipped/unmapped disk.
     */
    printf("\n Verifying \n");
#ifdef DEBUG
    for (cyl = startcyl; cyl < ncyl; cyl++) {
	if (cyl == startcyl + range)
		cyl = ncyl-1;
#else
    for (cyl = 0; cyl < ncyl; cyl++) {
#endif
	for (head = 0; head < nhead; head++) {
	    printf("\r cyl %d head %d ", cyl, head);

	    switch ( (i = longverf(cyl, head)) ) {
		case -2:
		    break;
		case -1:
		    printf("track error but can't find sector error!!\n");
		    break;
		default:
		    if (isitmap(cyl, head, i))
			continue;
		    moredef++;
		    printf("found bad sector %d,", i);
		    if (devcmd(SLIP, cyl, head, i) != 0) {
			printf(" mapped it\n");
			(void) mapsector(cyl, head, i, ind++);
		    } else
			printf(" slipped it\n");
		    updatedef(cyl, head, i, -1, -1, &defect[count++]);
		    break;
	    }
	}
    }
    printf("\n");

    /*
     * update defect list if more defect(s) are found during verify 
     */
    if (moredef)
	putdef(count);
ftdone:
    for (i = 0; i < 128 && dkbad.bt_bad[i].bt_cyl != INVALID; i++) {
	printf("mapping cyl %d, head %d, sect %d\n",
	    dkbad.bt_bad[i].bt_cyl, dkbad.bt_bad[i].bt_trksec >> 8,
	    dkbad.bt_bad[i].bt_trksec & 0xff);
	(void) devcmd(ZAP, dkbad.bt_bad[i].bt_cyl,
			dkbad.bt_bad[i].bt_trksec >> 8,
			dkbad.bt_bad[i].bt_trksec & 0xff, 1);
    }
    
    if (dkbad.bt_bad[0].bt_cyl != INVALID)
	putbad();		/* write map table */
    if (count)
	prdef(0, count);
    for (i = 0; i < sizeof save_jmp/sizeof save_jmp[0]; i++)
	    abort_jmp[i] = save_jmp[i];
}

/*
 * This routine does read/write scan using data patterns that are
 * believed to stress the disk circuitry. It returned: 
 * 	a) -1 for error resulted on a whole track read/write but
 * not on single sector read/write.
 * 	b) -2 for good status
 * 	c) the bad sector.
 */
int
longverf(cyl, head)
{
    int pass;
    int data;
    int sec;
    register int *dp = (int *)DBUF_VA;
    register int nw = nsect * SECSIZE / sizeof (int);

    for (pass = 0; pass < PATTERNS; pass++) {
	data = scandata[pass];
	initbuf(dp, nw, data);
	if (devcmd(WRITE, cyl, head, 0, nsect, DBUF_PA) != 0 ||
		devcmd(READ, cyl, head, 0, nsect, DBUF_PA) != 0 ||
		checkdata(dp, nw, data) != 0) {
	    for (sec = 0; sec < nsect; sec++) 
		if (devcmd(WRITE, cyl, head, sec, 1, DBUF_PA) != 0 ||
		    devcmd(READ, cyl, head, sec, 1, DBUF_PA) != 0 ||
		    checkdata(dp, nw/nsect, data) != 0) {
		    return(sec);
		}
	    return(-1);
	}
    }
    return(-2);
}

initdkbad()
{
    u_short *sp;
    int ns;

    sp = (u_short *)&dkbad;
    ns = sizeof dkbad/sizeof (u_short);
    while (ns--)
    	*sp++ = 0xffff;
    dkbad.bt_csn = 0;
    dkbad.bt_mbz = 0;
    dkbad.bt_flag = 0;
}

fmtinit()
{
    int i;

    for (i = 0; i < NDEFCT; i++) {
	defect[i].cyl = INVALID;	/* initialize defect array */
	defect[i].nbits = INVALID;
	defect[i].bfi = INVALID;
    }
    initdkbad();
}

/*
 *  This function return 0 if cyl/track has sector of type 'header' else
 * it return a -1.
 */
int 
doesithave(cyl, head, header)
{
    int i;
    register int *dp = (int *)DBUF_VA;

    (void) do_readhdr(cyl, head);
    for (i = 0; i < hsect; i++) {
	if (*dp++ == header)
	    return (0);
    }
    return -1;
}

#define chknbits(x) { if (x > 4096) \
	printf("WARNING! defect length of %d is abnormally long\n", x); }

/*
 * this routine get manually entered defects
 * return the number of defect entered.
 */
int
manualdef()
{
    int cyl, head, bfi, nbits, sect, i;
    struct defect_list *dl = &defect[0];
    struct specs *sp = &specs[controller][drive];
    int count = 0;

    printf("WARNING! previous defect list will be destroyed\n");
    printf("Use this command only with slip-sectored disk\n");
    printf("are you sure ? ");
    if (!confirm())
	return(0);

    for(;;) {
	cyl = pgetn( "cylinder? (Enter -1 if you want to stop)          ");
	if (cyl == -1)
	    break;
	if (cyl > ncyl+acyl) {
	    printf(" illegal cylinder \n");
	    continue;
	}
	do {
	    head = pgetn("head?                                             ");
	    if (head >= nhead) 
		printf(" illegal head \n");
	    else
		break;
	} while(1);
	bfi = pgetn("bytes from index?                                 ");
	do {
	    nbits = pgetn("length? (Enter -1 if you want to redo this entry) ");
	    if (nbits == 0) 
		printf(" length has to be greater than 0 \n");
	    else
		break;
	} while(1);
	chknbits(nbits);
	printf("\n\n");
	if (nbits == -1)
	    continue;

	sect = bfi/sp->special;
	updatedef(cyl, head, sect, bfi, nbits, dl++);
	count++;
	if (span(bfi, nbits, sp->special))  {
	    updatedef(cyl, head, sect+1, bfi, nbits, dl++);
	    count++;
	}
    }
    if (controller == C_XY450)		/* adaptive format */
	for (i = 0; i < count; i++)
	    defect[i].sect = adaptive(XSECT(i), XHEAD(i),
					(gotrunt ? hsect - 1 : hsect));
    for (i = 1; i < count; i++)
	if (XCYL(i) == XCYL(i-1) && XHEAD(i) == XHEAD(i-1) && 
		XSECT(i) != XSECT(i-1) && defect[i].bfi != defect[i-1].bfi)
		defect[i].sect = defect[i].sect - 1;
    return(count);
}

/*
 * this routine delete/change a defect entry in the defect array. It return
 * 0 if successful else -1 for delete and change. For add it will return 
 * the number of defective sectors for the defect entry.
 */
int
chgdef(flag, count)
{
    struct defect_list inp[2];
    struct specs *sp = &specs[controller][drive];
    int i;

    if (flag == CHANGE) {
	inp[0].cyl = pgetn("old cylinder?         ");
	inp[0].head = pgetn("old head?             ");
	inp[0].sect = pgetn("old sect?             ");
	inp[0].bfi = pgetn("old bytes from index? ");
	inp[0].nbits = pgetn("old length?           ");
    } else {
	inp[0].cyl = pgetn("cylinder?         ");
	inp[0].head = pgetn("head?             ");
	if (flag != ADD)
	    inp[0].sect = pgetn("sect?             ");
	inp[0].bfi = pgetn("bytes from index? ");
	inp[0].nbits = pgetn("length?           ");
    }

    if (flag == ADD) {
	printf("are you sure? ");
	if (!confirm())
		return(-1);
	i = 1;
	XCYL(count) = inp[0].cyl;
	XHEAD(count) = inp[0].head;
	XSECT(count) = inp[0].bfi/sp->special;
	defect[count].nbits = inp[0].nbits;
	defect[count].bfi = inp[0].bfi;
	if (controller == C_XY450)		/* adaptive format */
	    XSECT(count) = adaptive(XSECT(count), XHEAD(count),
					(gotrunt ? hsect - 1 : hsect));
	count++;
	if (span(inp[0].bfi, inp[0].nbits, sp->special)) {
	    XSECT(count) = XSECT(count-1) + 1;
	    i++;
	}
	printf("%s", msg1);
	return(i);
    }
    
    if (flag == CHANGE) {
	printf("\n\n");
	inp[1].cyl = pgetn("new cylinder?         ");
	inp[1].head = pgetn("new head?             ");
	inp[1].bfi = pgetn("new bytes from index? ");
	inp[1].nbits = pgetn("new length?           ");
	chknbits(inp[1].nbits);
	inp[1].sect = inp[1].bfi/sp->special;
	if (controller == C_XY450)		/* adaptive format */
	    inp[1].sect = adaptive(inp[1].sect, inp[1].head,
					(gotrunt ? hsect - 1 : hsect));
    }
    printf("are you sure? ");
    if (!confirm())
	    return(-1);

    for (i = 0; i < count; i++) {
	if ( XCYL(i) == inp[0].cyl && XHEAD(i) == inp[0].head &&
		XSECT(i) == inp[0].sect && XBFI(i) == inp[0].bfi &&
		XNBITS(i) == inp[0].nbits )
	    switch (flag) {
		case CHANGE:
		    XCYL(i) = inp[1].cyl;
		    XHEAD(i) = inp[1].head;
		    XSECT(i) = inp[1].sect;
		    XBFI(i) = inp[1].bfi;
		    XNBITS(i) = inp[1].nbits;
		    printf("%s", msg1);
		    return(0);
		case DELETE:
		    for ( i = i; i < count; i++ ) 
			defect[i] = defect[i+1];
		    printf("%s", msg1);
		    return(0);
		default:
		    printf("unsupported option in deldef\n");
		    doexit();
	    }
    }

    printf(" can't find cyl %d head %d sect %d bfi %d length %d ", inp[0].cyl,
		inp[0].head, inp[0].sect, inp[0].bfi, inp[0].nbits);
    switch (flag) {
	case CHANGE:
	    printf("to change\n");
	    break;
	case DELETE:
	    printf("to delete\n");
	    break;
    }
    return(-1);
}

