#ifndef lint
static	char sccsid[] = "@(#)diag.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>
#include <sun/dkio.h>
#include <sys/dkbad.h>

/*
 *----------------------------------------------------------------------------
 *      disk diagnostic
 *----------------------------------------------------------------------------
 */

int infomsgs = 0, errors = 0, timing = 0, slipmsgs = 0, mapcheck = 1;
int formatmsgs = 0, max_retries, doabort, map_read;

int IPcmd();	/* Interphase SMD-2180 */
int XYcmd();	/* Xylogics 440, 450 */
int XDcmd();	/* Xylogics 751 */
int SCcmd();	/* Adaptec 4000, Adaptec 4520, Emulex MD21(S2) */

/* note order is defined in diag.h */
struct {
	int	(*c_cmd)();
	char	*c_name;
	int	c_flags;
} ctlrtab[] = {
	IPcmd,	"Interphase SMD-2180",		DKI_FMTTRK+DKI_MAPTRK,
	XYcmd,	"Xylogics 440 (prom set 926)",	DKI_BAD144,
	XYcmd,	"Xylogics 450/451",		DKI_BAD144,
	SCcmd,	"Adaptec ACB 4000 - SCSI/ST506",DKI_FMTVOL,
	SCcmd,	"Emulex MD21 - SCSI/ESDI",	DKI_FMTVOL,
#ifdef notdef
	XDcmd,	"Xylogics 751",			DKI_BAD144,
#endif
	0
};

/*
 * Tables of # of sectors per track, gaps and interleave factors
 * indexed by controller type.
 *
 * This is because different controllers use differing per-sector
 * overhead data on the disk. (e.g., CRC vs ECC).
 *
 * "X" means controller dosen't care about parameter.
 */

#define	X	-1
#define	NONE	0,0,0,0
#define NPARAMS	6		/* # of entries in each param array */

struct params 
		/* SMD-2180 */	/* XY-440 */	/* XY-450 */
		/* Adaptec */	/* CCS */	/* XY-751 */
s_norm[]   = {	{34,X,X,7},	{32,X,X,1},	{32,X,X,1},
		{NONE},		{NONE},		{32,X,X,1},
},
s_huge[]   = {	{48,X,X,7},	{NONE},		{46,X,X,1},
		{NONE},		{NONE},		{46,X,X,1},
},
s_super[]   = {	{NONE},		{NONE},		{67,X,X,1},
		{NONE},		{NONE},		{67,X,X,1},
},
s_cdc[]   = {	{NONE},		{NONE},		{48,X,X,1},
		{NONE},		{NONE},		{48,X,X,1},
},
s_lark[]   = {	{32,X,X,7},	{32,X,X,1},	{32,X,X,1},
		{NONE},		{NONE},		{NONE},
},
s_st506[]  = {	{NONE},		{NONE},		{NONE},
		{17,X,X,1},	{NONE},		{NONE},
},
s_ask[]    = {	{0,X,X,7},	{NONE},		{NONE},
		{NONE},		{NONE},		{NONE},
		{0,X,X,7},	{NONE},		{NONE},
		{NONE},		{NONE},		{NONE},
		{0,X,X,7},	{NONE},		{NONE},
		{NONE},		{NONE},		{NONE},
		{0,X,X,7},	{NONE},		{NONE},
		{NONE},		{NONE},		{NONE},
},
s_ccs[]	   = {	{NONE},		{NONE},		{NONE},
		{NONE},		{34,X,X,1},	{NONE},
};

/* track type defs for XY440 and XY450 in XY440 compat */
#define th(t,h)	(((t)<<6)+(h))
#define heads(th) (th & 077)
#define type(th) (th >> 6)

struct specs smd[] = {
	587,	2,	0,	0, th(1,7),  s_norm, 621, "Fujitsu-M2312K",
	821,	2,	0,	0, th(2,10), s_norm, 621, "Fujitsu-M2284/M2322",
	840,	2,	0,	0, th(0,20), s_huge, 595, "Fujitsu-M2351 Eagle",
	821,	2,	0,	0, th(3,10), s_super, 600, "Fujitsu-M2333",
	840,	2,	0,	0, th(3,20), s_super, 600,"Fujitsu-M2361 Eagle",
	1147,	2,	0,	0, th(1,10), s_cdc, 613,"CDC EMD 9720",
#ifdef notdef
	1022,	2,	0,	0, th(3,16), s_norm, 0, "Fujitsu-M2294",
	200,	2,	0,	1, 2,	  s_lark, 0, "CDC-Lark-cartridge",
	200,	2,	2,	0, 2,	  s_lark, 0, "CDC-Lark-fixed",
	612,	12,	0,	1, 2,	  s_lark, 0, "CDC-LarkII-cartridge",
	612,	12,	2,	0, 2,	  s_lark, 0, "CDC-LarkII-fixed",
	820,	3,	0,	0, 19,	  s_norm, 0, "CDC-9766",
	821,	2,	0,	0, th(2,10), s_norm, 0, "CDC-9730-160",
	820,	3,	0,	0, 5,	  s_norm, 0, "CDC-9730-80",
	318,	2,	0,	0, 4,	  s_norm, 0, "CDC-9730-24",
	318,	2,	0,	0, 2,	  s_norm, 0, "CDC-9730-12",
	820,	3,	0,	0, 5,	  s_norm, 0, "Ampex-Scorpio",
	1022,	2,	0,	0, th(3,16), s_norm, 0, "Ampex-Capricorn",
#endif notdef
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	0
};

struct specs smd440a[] = {
	587,	2,	0,	0, th(1,7),  s_norm, 0, "Fujitsu-M2312K",
	821,	2,	0,	0, th(2,10), s_norm, 621, "Fujitsu-M2284/M2322",
	1022,	2,	0,	0, th(3,16), s_norm, 0, "Fujitsu-M2294",
#ifdef notdef
	1022,	2,	0,	0, th(3,16), s_norm, 0, "Ampex-Capricorn",
	821,	2,	0,	0, th(2,10), s_norm, 0, "CDC-9730-160",
#endif notdef
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	-1,	0,	0,	0, 0,	0, 0,	"Other",
	0,
};

struct specs st506[] = {
/*	ncyl  acyl   buf sk  wr prcmp  nhead  */
	825,	5,	2,	400,	6,	s_st506, 0, "Micropolis 1304",
	1022,	2,	2,	1024,	8,	s_st506, 0, "Micropolis 1325",
	1020,	4,	2,	1024,	5,	s_st506, 0, "Maxtor XT-1050",
	752,	2,	2,	754,	11,	s_st506, 0, "Fujitsu M2243AS",
	1163,	3,	2,	1166,	7,	s_st506, 0, "Vertex V185",
#ifdef notdef
	984,	3,	2,	987,	5,	s_st506, 0, "Vertex V150",
	576,	2,	2,	0,	5,	s_st506, 0, "Tandon TM 703",
	300,	6,	2,	0,	6,	s_st506, 0, "Tandon TM 503",
	640,	5,	2,	0,	5,	s_st506, 0, "Atasi 3033",
	640,	5,	2,	0,	7,	s_st506, 0, "Atasi 3046",
	300,	6,	2,	0,	6,	s_st506, 0, "Seagate ST419",
	914,	4,	2,	918,	15,	s_st506, 0, "Maxtor XT-1140",
#endif notdef
	-1,	0,	0,	0,	0,	0, 0,	"Other",
	-1,	0,	0,	0,	0,	0, 0,	"Other",
	-1,	0,	0,	0,	0,	0, 0,	"Other",
	-1,	0,	0,	0,	0,	0, 0,	"Other",
	0
};

struct specs ccs[] = {
	1018,	2,	0,	0,	8,	s_ccs,	0, "Micropolis 1355",
	815,	2,	0,	0,	10,	s_ccs,	0, "Toshiba MK 156F",
#ifdef notdef
	905,	2,	0,	0,	9,	s_ccs,	0, "CDC 94166-182",
	815,	2,	0,	0,	10,	s_ccs,	0, "Fujitsu M2246E",
	815,	2,	0,	0,	10,	s_ccs,	0, "Hitachi DK512-17",
	815,	2,	0,	0,	10,	s_ccs,	0, "NEC D5652",
#endif notdef
	-1,	0,	0,	0,	0,	0,	0, "Other",
	-1,	0,	0,	0,	0,	0,	0, "Other",
	-1,	0,	0,	0,	0,	0,	0, "Other",
	-1,	0,	0,	0,	0,	0,	0, "Other",
	0
};

struct specs *specs[] = {
	smd,		/* 2180 */
	smd440a,	/* XY 440 */
	smd,		/* XY 450 */
	st506,		/* ACB 4000 */
	ccs,		/* CCS (and ESDI) */
	smd,		/* XY 751 */
};

int C_diag(), C_info(), C_errors(), C_clear(), C_time();
int C_partition(), C_label(), C_verify();
int C_format(), C_fix(), C_map(), C_seek(), C_write(), C_read();
int C_test(), C_position(), C_status(), C_help(), C_add(), C_sub();
int C_dmatest(), C_abortdma(), C_version();
int C_rhdr(), C_whdr(), C_translate(), C_slip(), C_scan(), C_slipmsgs();
int C_mapcheck(), C_formatmsgs(), C_sformat();

struct cmd {
	char *c_name;
	int c_len;
	int (*c_func)();
} commands[] = {
	"formatmsgs",	7,	C_formatmsgs,
	"abortdma",	6,	C_abortdma,
	"slipmsgs",	5,	C_slipmsgs,
	"version",	4,	C_version,
	"mapcheck",	4,	C_mapcheck,
	"dmatest",	2,	C_dmatest,
	"fix",		2,	C_fix,
	"partition",	2,	C_partition,
	"position",	2,	C_position,
	"rhdr",		2,	C_rhdr,
	"scan",		2,	C_scan,
	"slip",		2,	C_slip,
	"status",	2,	C_status,
	"test",		2,	C_test,
	"time",		2,	C_time,
	"whdr",		2,	C_whdr,
	"+",		1,	C_add,
	"-",		1,	C_sub,
	"?",		1,	C_help,
	"clear",	1,	C_clear,
	"diagnose",	1,	C_diag,
	"errors",	1,	C_errors,
	"format",	1,	C_format,
	"help",		1,	C_help,
	"info",		1,	C_info,
	"label",	1,	C_label,
	"map",		1,	C_map,
	"read",		1,	C_read,
	"seek",		1,	C_seek,
	"translate",	1,	C_translate,
	"verify",	1,	C_verify,
	"write",	1,	C_write,
	"sformat",	2,	C_sformat,
	"quit",		1,	NULL,
	NULL,
};

int silent = 0;	/* very klugy variable to disable printing error msg */
int nothers;	/* kludge for remembering other's disk sizes */

main() 
{
	char buf[LINEBUFSZ], *p;
	struct cmd *cp;
	int i;

	machinit();
 	printf("\nVersion %s\n", "1.1 86/09/25");
 	printf("Disk Initialization and Diagnosis\n\n");
	printf("When asked if you are sure, respond with 'y' or 'Y'\n\n");
	doabort = 0;
	nothers = 0;

	if (_setjmp(abort_jmp) && doabort)
		goto out;
	C_diag();
	if (_setjmp(abort_jmp) && doabort)
		goto out;
	for (;;) {
		max_retries = NORMAL_RETRIES;
		map_read = 0;
		printf("diag> ");
		gets(buf);
		for (p = buf; *p == ' ' || *p == '\t'; p++)
			;
		if (*p == 0 || p >= &buf[LINEBUFSZ])
			continue;
		for (cp = commands; cp->c_name != NULL; cp++) { 
			i = MAX(cp->c_len, strlen(p));
			if (strncmp(p, cp->c_name, i) == 0) {
				if (cp->c_func == NULL)
					goto out;
				(*cp->c_func)();
				break;
			}
		}
		if (cp->c_name == NULL)
			printf("What? ");
	}
out: ;
}

doexit()
{

	doabort = 1;
	_longjmp(abort_jmp, 1);
	/*NOTREACHED*/
}

C_diag()
{
	register struct specs *sp;
	int i;

	do {
		printf("specify controller:\n");
		for (i = 0; ctlrtab[i].c_cmd != 0; i++)
			printf("\t%d - %s\n", i, ctlrtab[i].c_name);
		controller = pgetn("which one? ");
	} while (controller < 0 || controller >= i);

	printf("Specify controller address on the mainbus (in hex): ");
	i = getx();
	printf("Device address: %x\n\n", i);
	if (ctlrtab[controller].c_cmd == SCcmd) {
		do {
			target = pgetn("Which target? ");
			printf("\n");
		} while (target > 7);
		scsi = 1;
	} else {
		scsi = 0;
	}
	setdevaddr(i);

	do {
		unit = pgetn("Which unit? ");
		printf("\n");
	} while (unit < 0 || unit >= 4);

	do {
		printf("Specify drive:\n");
		for (i = 0, sp = specs[controller]; sp->s_ncyl != 0; sp++) {
			printf("\t%d - %s\n", i++, sp->s_id);
			if (sp->s_ncyl == -1)
				break;
		}
		drive = pgetn("which one? ");
		printf("\n");
	} while (drive < 0 || drive >= i);

	sp = &specs[controller][drive];
	if (sp->s_ncyl == -1) {
		printf("NOTE: # of data cylinders must be at least\n");
		if (controller == C_CCS_EMULEX)
			printf("six less than the physical # of cylinders\n");
		else
			printf("two less than the physical # of cylinders\n");
		sp->s_ncyl = pgetn("# of data cylinders? ");
reacyl:		sp->s_acyl = pgetn("# of alternate cylinders (minimum 2)? ");
		if (sp->s_acyl < 2)
			goto reacyl;
		if (controller == C_ADAPTEC) {
			sp->s_bhead = pgetn("buffered seek? (usually 2) ");
			sp->s_ppart = pgetn("cyl # to start write precomp? ");
		} else if (!scsi) {
			sp->special = pgetn("# of bytes/sector (including overhead bytes) ? ");
			sp->s_bhead = 
			    pgetn("first head? (usually 0, 2 for Lark fixed) ");
			sp->s_ppart = 
			    pgetn("physical partition? (usually 0, 1 for Lark cartridge) ");
		}
		sp->s_nhead = pgetn("# of heads? ");
		if (controller == C_XY440 || controller == C_XY450)
			sp->s_nhead |= (pgetn("drive type? ") & 03) << 6;
		if (nothers > 3) {
			printf("Too many 'others', must reenter diag\n");
			doexit();
		}
		sp->s_param = &s_ask[nothers * NPARAMS];
		nothers++;
		printf("ASCII identification? ");
		gets(sp->s_id);
	}
	ascii_id = sp->s_id;
	ncyl = sp->s_ncyl;
	acyl = sp->s_acyl;
	/* if we need spares/cyl, this needs to be added to the tables */
	nspare = 0;
	basehead  = sp->s_bhead;
	physpart = sp->s_ppart;
	nhead = heads(sp->s_nhead);
	gap1 = X;
	gap2 = X;
	nsect = sp->s_param[controller].s_nsect;
	interleave = sp->s_param[controller].s_interleave;
	if (nsect == 0) {
		nsect = pgetn("# of data sectors/track? ");
		sp->s_param[controller].s_nsect = nsect;
	}
	if (interleave == 0) {
		interleave = pgetn("interleave factor? ");
		sp->s_param[controller].s_interleave = interleave;
	}
	drivetype = type(sp->s_nhead);
	currmap = NULL;
	printf("ncyl %d acyl %d nhead %d nsect %d ", ncyl, acyl, nhead, nsect);
	if (interleave != X && controller != C_SMD2180)
		printf("interleave %d", interleave);
	printf("\n");

	(void) devcmd(INIT);
	if (!scsi)				/* to avoid message */
		(void) devcmd(STATUS);

}

C_time()
{

	timing ^= 1;
	printf("timing %s\n", timing ? "on" : "off");
}

C_info()
{

	infomsgs ^= 1;
	printf("informational messages %s\n", infomsgs ? "on" : "off");
}

C_errors()
{

	errors ^= 1;
	printf("errors %s\n", errors ? "on" : "off");
}

C_slipmsgs()
{

	slipmsgs ^= 1;
	printf("slip messages %s\n", slipmsgs ? "on" : "off");
}

C_formatmsgs()
{

	formatmsgs ^= 1;
	printf("format messages %s\n",formatmsgs ? "on" : "off");
}

C_mapcheck()
{

	mapcheck ^= 1;
	printf("Checking for overlapped mappings on errors %s\n",
		mapcheck ? "enabled" : "disabled");
}

C_clear()
{

	printf("clear\n");
 	asm("reset");			/* Ouch */
	DELAY(500000);
	machreset();

	(void) devcmd(INIT);
	(void) devcmd(RESTORE);
}

struct dkbad dkbad;
static int nslip;

int passdata[] = {
	0xc6dec6de,	/* media worst case */
	0x6db6db6d,	/* 0110110 ... */
	0x00000000,	/* 0000000 ... */
	0xffffffff,	/* 1111111 ... */
	0xaaaaaaaa,	/* 1010101 ... */
};

int npassdata = (sizeof passdata/sizeof passdata[0]);

getnpass(usual)
	int usual;
{

	printf("# of surface analysis passes (%d recommended)? ", usual);
	return getn();
}

C_format()
{
	int cyl, head;
	int begin, end;
	int npass, i, flags = ctlrtab[controller].c_flags;

	max_retries = FORMAT_RETRIES;
	nslip = 0;

	if (flags & DKI_FMTVOL) {
		(void) devcmd(FORMAT);
		return;
	}

	printf("DISK FORMAT -- DESTROYS ALL DISK DATA!\nare you sure? ");
	if (!confirm())
		return;

	if (controller == C_XY450 || controller == C_XY751) {
	    fmtdk(1);		/* FIXME get rid of this magic number */
	    return;
	}

	npass = getnpass(npassdata);

	/* initialize dkbad */
	initdkbad();
	if (flags & DKI_FMTTRK)		/* reserve diagnostic track 0 */
		dkbad.bt_bad[nhead-2].bt_cyl = ncyl+acyl-1;

	begin = *RomVecPtr->v_RefrCnt;
	for (cyl = 0; cyl < ncyl+acyl; cyl++) {
		if (!timing)
			printf("\rcyl %d ", cyl);
		for (head = 0; head < nhead; head++)
			(void) fmttrack(cyl, head, npass, 0);
	}
	end = *RomVecPtr->v_RefrCnt;
	if (timing)
		printf("%d ms\n", end-begin);

	for (i = 0; i < NBAD && dkbad.bt_bad[i].bt_cyl != 0xffff; i++) {
		if (flags & DKI_MAPTRK) {
			int newhead, newcyl;

			if (i == nhead-2)	/* diagnostic track */
				continue;
			newhead = (ncyl+acyl)*nhead - 2 - i;
			newcyl = newhead / nhead;
			newhead %= nhead;
			(void) devcmd(MAP, dkbad.bt_bad[i].bt_cyl,
				dkbad.bt_bad[i].bt_trksec >> 8,
				newhead, newcyl);
		} else if (flags & DKI_FMTTRK) {
			if (i == nhead-2)	/* diagnostic track */
				continue;
			(void) devcmd(ZAP, dkbad.bt_bad[i].bt_cyl,
				dkbad.bt_bad[i].bt_trksec >> 8, 0, nsect);
		} else if (flags & DKI_BAD144) {
			(void) devcmd(ZAP, dkbad.bt_bad[i].bt_cyl,
				dkbad.bt_bad[i].bt_trksec >> 8,
				dkbad.bt_bad[i].bt_trksec & 0xff, 1);
		}
	}
	putbad();

	printf("format complete - %d bad %s%s", i+nslip,
	    (flags & DKI_FMTTRK) ? "track" : "sector",
	    ((i+nslip) == 1) ? "" : "s");
	if (nslip)
		printf(" - %d slipped %d", nslip, i);
	printf(" mapped\nUse the label command to label the disk.\n");
}

/*
 * This routine check for a healthy dkbad structure
 * We never have a magic number in the dkbad structure hence the reason
 * d'etre for this routine. 
 */
int
chkmap(b)
    struct bt_bad *b;
{
    if (b->bt_cyl == 0 && (b->bt_trksec >> 8) == 0 && 
			 (b->bt_trksec & 0xff) == 0)
	return (-1);		/* cyl 0, head 0, sect 0 is always good */

    while (b->bt_cyl != 0xffff)
	if (b->bt_cyl < (ncyl+acyl) && (b->bt_trksec >> 8) < nhead && 
			(b->bt_trksec & 0xff) < nsect)
	    b++;
	else
	    return (-1);
    return (0);
}

/*
 * Read bad sector/track table.  If none is found the dkbad table
 * is initialized for no errors and if the readonly flag is not
 * true then the last track is formatted and the newly created
 * bad sector/track is written out using putbad().
 */
getbad(readonly)
	int readonly;
{
	register int sec;

	for (sec = 0; sec < nsect && sec < 10; sec += 2)
		if (devcmd(READ, ncyl+acyl-1, nhead-1, sec, 1, DBUF_PA) == 0) {
			dkbad = *(struct dkbad *)DBUF_VA;
			if (dkbad.bt_mbz != 0)
				continue;

			/*
			 * in case the user change the switch setting
			 * which would allow the controller to read sector 0
			 * but not any other sector.
			 */
			if (devcmd(READ, ncyl+acyl-1, nhead-1, sec+2, 1, DBUF_PA) != 0)
			    continue;
			if (chkmap(&dkbad.bt_bad[0]) == 0)
			    return;
		}
	printf("No bad sector/track table found -- assuming none bad\n");
	/* initialize dkbad */
	initdkbad();
	
	if (!readonly) {
		/* make sure last track is formatted (for putbad) */
		(void) devcmd(FORMAT, ncyl+acyl-1, nhead-1, 0, nsect);
		putbad();
	}
}

/*
 * Write bad sector/track table on the disk.  This table is written
 * on the even sectors of the last track up to 5 times.
 */
putbad()
{
	register int sec;

	*(struct dkbad *)DBUF_VA = dkbad;
	for (sec = 0; sec < nsect && sec < 10; sec += 2)
		(void) devcmd(WRITE, ncyl+acyl-1, nhead-1, sec, 1, DBUF_PA);
}

/*
 * Returns the block number of a previously mapped sector that
 * will collide with a slip of the sector passed.  This occurs
 * if the sector to be slipped is less than or equal to a sector
 * mapped on the same track.  Returns -1 if none found.
 */
int
slipcheck(cyl, head, sec)
	int cyl, head, sec;
{
	register struct bt_bad *b;

	for (b = dkbad.bt_bad; b < &dkbad.bt_bad[NBAD] && b->bt_cyl != 0xffff;
	    b++) {
		if (cyl == b->bt_cyl && head == (b->bt_trksec >> 8) &&
			sec <= (b->bt_trksec & 0xff))
			return ((b->bt_cyl * nhead * nsect) +
			    ((b->bt_trksec >> 8) * nsect) +
			    (b->bt_trksec & 0xff));
	}
	return -1;
}

int
checkdata(dp, n, data)
	register int *dp, n, data;
{
	register int *ip, *lp = &dp[n];

	for (ip = dp; ip < lp; ip++) {
		if (*ip != data) {
			printf("data readback error!\n");
			printf("expected 0x%x got 0x%x @ word %d\n",
				data, *ip, ip - dp);
			return -1;
		}
	}
	return 0;
}

initbuf(b, nw, data)
	register int *b, nw, data;
{
	register int *last = b + nw;

	while (b < last)
		*b++ = data;
}

fmttrack(cyl, head, npass, reslip)
{
	register int *dp = (int *)DBUF_VA;
	register int nw = nsect * SECSIZE / sizeof (int);
	register int data;
	int pass, i, sec, newhead, newcyl;
	int found = 0;
	int flags = ctlrtab[controller].c_flags;
	static int slipped[40];
	register int *sp;

	/*
	 * If reslip is true, then we want to try and reslip any
	 * previously slipped sectors.  We have to get a list of
	 * the sectors slipped on this track before we reformat it.
	 * The list of sectors is terminated by an entry of -1.
	 */
	slipped[0] = -1;
	if (reslip && (flags & DKI_BAD144))
		(void) devcmd(READ_SLIP, cyl, head, slipped);

	if (devcmd(FORMAT, cyl, head, 0, nsect))
		goto bad;

	for (sp = slipped; *sp != -1; sp++) {
		printf("reslipping %d/%d/%d ", cyl, head, *sp);
		if (devcmd(SLIP, cyl, head, *sp) == 0)
			printf("\n");
		else
			printf("[failed]\n");
	}

	if (devcmd(VERIFY, cyl, head, 0, nsect))
		goto bad;

	for (pass = 0; pass < npass; pass++) {
		data = passdata[pass % npassdata];
		initbuf(dp, nw, data);
		if (devcmd(WRITE, cyl, head, 0, nsect, DBUF_PA) != 0 ||
		    devcmd(READ, cyl, head, 0, nsect, DBUF_PA) != 0 ||
		    checkdata(dp, nw, data) != 0)
			goto bad;
	}
	return 0;

bad:
	if (flags & DKI_FMTTRK) {
		if (cyl >= ncyl) {
			printf("%d/%d:  BAD ALTERNATE TRACK - DISK UNUSABLE! (?)\n",
			    cyl, head);
			/* DO SOMETHING INTELLIGENT HERE */
			return 0;
		}
		printf("\n%d/%d: bad track: ", cyl, head);
		for (i = 0; i < NBAD && dkbad.bt_bad[i].bt_cyl != 0xffff; i++) {
			if (i == nhead-2)	/* diagnostic track */
				continue;
			if (dkbad.bt_bad[i].bt_cyl == cyl &&
			    dkbad.bt_bad[i].bt_trksec >> 8 == head) {
				printf("already mapped\n");
				return 0;
			}
		}
		if (i >= NBAD) {
			printf("BAD TRACK TABLE OVERFLOW - DISK UNUSABLE!\n");
			return 0;
		}
		newhead = (ncyl+acyl)*nhead - 2 - i;
		newcyl = newhead / nhead;
		newhead %= nhead;
		if (newcyl < ncyl) {
			printf("ALTERNATE AREA OVERFLOW - DISK UNUSABLE!\n");
			return 0;
		}
		dkbad.bt_bad[i].bt_cyl = cyl;
		dkbad.bt_bad[i].bt_trksec = (head << 8) + nsect;
		printf("mapping to %d/%d\n", newcyl, newhead);
		return 1;
	}

	/* find sector in error */
	for (sec = 0; sec < nsect; sec++) 
		found += checksector(cyl, head, sec, 2*(npass+1));
	if (!found)
		printf("%d/%d: track error, but no sector errors!\n",
			cyl, head);
	return (found);
}

/*
 * Add entry to dkbad structure at the `index' slot.
 * Returns 0 if all ok or -1 on error.
 */
int
mapsector(cyl, head, sec, index)
	register int cyl, head, sec, index;
{
	register int newcyl, newhead, newsec;

	if (index >= NBAD) {
		printf("BAD SECTOR TABLE OVERFLOW - DISK UNUSABLE!\n");
		return 1;
	}
	newsec = (ncyl+acyl)*nhead*nsect - nsect - index - 1;
	newcyl = newsec / (nhead*nsect);
	newsec %= (nhead*nsect);
	newhead = newsec / nsect;
	newsec %= nsect;
	if (newcyl < ncyl) {
		printf("ALTERNATE AREA OVERFLOW - DISK UNUSABLE!\n");
		return 1;
	}
	dkbad.bt_bad[index].bt_cyl = cyl;
	dkbad.bt_bad[index].bt_trksec = (head << 8) + sec;
	printf("mapping to %d/%d/%d\n", newcyl, newhead, newsec);
	return 0;
}

static
checksector(cyl, head, sec, npass)
{
	register int *dp = (int *)DBUF_VA;
	register int data, pass, i;
	int nw = SECSIZE / sizeof (int);
	int didslip = 0;

again:
	for (pass = 0; pass < npass; pass++) {
		data = passdata[pass % npassdata];
		initbuf(dp, nw, data);
		if (devcmd(WRITE, cyl, head, sec, 1, DBUF_PA) != 0 ||
		    devcmd(READ, cyl, head, sec, 1, DBUF_PA) != 0 ||
		    checkdata(dp, nw, data) != 0)
			goto bad;
	}
	return (didslip);

bad:
	printf("%d/%d/%d: bad sector: ", cyl, head, sec);

	/*
	 * If we haven't tried it before and if there are no conflicts
	 * with mapped sectors on this track, try and slip the defective
	 * sector.  If we do then go back and check again and reverify
	 * the logical sector as it is now at a different physical
	 * location on the track.  Note that we only try to slip a
	 * logical sector once to avoid any chance of an infinite loop.
	 */
	if (didslip == 0 && slipcheck(cyl, head, sec) == -1 &&
	    devcmd(SLIP, cyl, head, sec) == 0) {
		addonedef(cyl, head, sec);
		nslip++;
		didslip++;
		printf("slipped to spare sector\n");
		goto again;
	}

	/*
	 * Ok, we found the sector in error and we can't slip it
	 * (i.e. we have to map it).  Make sure that we are not
	 * on an "alternate" cylinder.
	 */
	if (cyl >= ncyl) {
		printf("%d/%d/%d:  BAD ALTERNATE SECTOR!\n",
		    cyl, head, sec);
		/* DO SOMETHING INTELLIGENT HERE */
		return 1;
	}
	for (i = 0; i < NBAD && dkbad.bt_bad[i].bt_cyl != 0xffff; i++) {
		if (dkbad.bt_bad[i].bt_cyl == cyl &&
		    dkbad.bt_bad[i].bt_trksec == (head << 8)+sec) {
			printf("already mapped\n");
			return 1;
		}
	}
	addonedef(cyl, head, sec);
	(void) mapsector(cyl, head, sec, i);
	return 1;
}

static
scompar(b1, b2)
	register int *b1, *b2;
{

	if (*b1 < *b2)
		return -1;
	if (*b1 > *b2)
		return 1;
	return 0;
}

/*
 * Repeatedly scan sectors looking for additional bad sectors to map.
 * When a bad sector is found, it is slipped or zapped immediately.
 */
C_scan()
{
	register int bn, pass, *dp = (int *)DBUF_VA;
	register int data, cyl, head, sec, i;
	register struct bt_bad *b;
	int s[NBAD+1], *bbn, n = 0;
	int flags = ctlrtab[controller].c_flags;
	int sb, eb;
	int nw = SECSIZE / sizeof (int);
	int secprt, labelwrite, reinit;
	int dofix, dorand;
	jmp_buf save_jmp;

	printf("scan - continuous scan for defective sectors\n");
	printf("DESTROYS DISK DATA!\n");
	
	max_retries = FORMAT_RETRIES;
	if (flags & DKI_BAD144) {
		getbad(0);
		for (b = dkbad.bt_bad; n < NBAD && b->bt_cyl != 0xffff; b++)
			s[n++] = (b->bt_cyl * nhead * nsect) +
				((b->bt_trksec >> 8) * nsect) +
				(b->bt_trksec & 0xff);
	}
	s[n] = 0x7fffffff;
	qsort((char *)s, n, sizeof s[0], scompar);

	printf("scan entire disk? ");
	if (confirm()) {
		sb = 0;
		/*
		 * Don't scan alternate area on disks
		 * to avoid destroying format info there
		 */
		eb = ncyl * (nhead * nsect - nspare);
	} else for(;;) {
		sb = pgetbn("starting block? ");
		if (sb < 0)
			sb = 0;
		eb  = pgetbn("ending block? ");
		if (eb > (ncyl * (nhead * nsect - nspare)))
			eb = ncyl * (nhead * nsect - nspare);
		if (eb <= sb) {
			printf("end block must be greater than start block\n");
			printf("scan will not include the end block\n");
			continue;
		}
		break;
	}

	/*
	 * Compute and remember if any of the label area will be trashed
	 * by checking the range of the area to be scanned.
	 */
	labelwrite = (sb == 0 || eb > ncyl * (nhead * nsect - nspare));
	if (labelwrite)
		printf("Warning - label area included in scan\n");

	printf("Use random bit patterns? ");
	dorand = confirm();

	if (flags & DKI_BAD144) {
		printf("Perform corrections when defects are found? ");
		dofix = confirm();
	} else {
		printf("Controller type is %s,\n", ctlrtab[controller].c_name);
		printf("no corrections will be done when defects are found\n");
		dofix = 0;
	}

	printf("OK to scan from ");
	putbn(sb);
	printf(" to ");
	putbn(eb - 1);
	printf("? ");
	if (!confirm())
		return;

	/*
	 * Set flag for printing block #s on sector boundaries.
	 * We print out each sector if we are scanning a track
	 * or less or if we are not using track boundaries.
	 */
	secprt = (((eb - sb) <= nsect) ||
	    (((eb % (nhead * nsect - nspare)) % nsect) != 0) ||
	    (((sb % (nhead * nsect - nspare)) % nsect) != 0));

	for (i = 0; i < sizeof save_jmp/sizeof save_jmp[0]; i++)
		save_jmp[i] = abort_jmp[i];

	if (_setjmp(abort_jmp)) {
		/*
		 * We just got interruptted out of the scan command.
		 * If we are doing fixing then write out the
		 * bad sector list and re-zap sectors and warn user
		 * if the label might have been trashed.
		 * Then restore the abort buffer and return.
		 */
		if (dofix) {
			putbad();
			for (b = dkbad.bt_bad; b < &dkbad.bt_bad[NBAD] &&
			    b->bt_cyl != 0xffff; b++) {
				bn = (b->bt_cyl * nhead * nsect) +
					((b->bt_trksec >> 8) * nsect) +
					(b->bt_trksec & 0xff);
				if (bn < sb || bn >= eb)
					continue;
				(void) devcmd(ZAP, b->bt_cyl, b->bt_trksec >> 8,
					b->bt_trksec & 0xff, 1);
			}
		}
		if (labelwrite)
			printf("Re-issue label command to rewrite label\n");
		for (i = 0; i < sizeof save_jmp/sizeof save_jmp[0]; i++)
			abort_jmp[i] = save_jmp[i];
		return;
	}
	printf("type control-C to quit\n");

	for (pass = 1; ;pass++) {
		if (dorand) {
			data = rand();
			printf("\n[pass %d - bit pattern: 0x%x]\n", pass, data);
		} else {
			data = passdata[(pass - 1) % npassdata];
			printf("\n[pass %d - bit pattern #%d: 0x%x]\n",
				pass, ((pass - 1) % npassdata) + 1, data);
		}
		reinit = 1;
		for (bn = sb, bbn = s; bn < eb; bn++) {
			if (reinit) {
				reinit = 0;
				initbuf(dp, nw, data);
			}
			cyl = bn / (nhead * nsect - nspare);
			sec = bn % (nhead * nsect - nspare);
			head = sec / nsect;
			sec %= nsect;

			if (secprt)
				printf("\r%d/%d/%d  ", cyl, head, sec);
			else if (sec == 0)
				printf("\r%d/%d ", cyl, head);

			while (*bbn < bn)
				bbn++;		/* skip to next bad block */
			if (*bbn == bn)
				continue;	/* already mapped, skip it */

			if (devcmd(WRITE, cyl, head, sec, 1, DBUF_PA) == 0 &&
			    devcmd(READ, cyl, head, sec, 1, DBUF_PA) == 0 &&
			    checkdata(dp, nw, data) == 0)
				continue;	/* looks ok, on to next */

			reinit = 1;		/* set flag to reset buffer */

			printf("found bad sector %d/%d/%d ", cyl, head, sec);

			if (!dofix) {		/* don't try and correct */
				printf("[ignored]\n");
				continue;
			}

			/*
			 * If slipping this track looks ok, give it a whirl
			 */
			if (slipcheck(cyl, head, sec) == -1 &&
			    devcmd(SLIP, cyl, head, sec) == 0) {
				addonedef(cyl, head, sec);
				printf("slipped to spare sector\n");
				continue;	/* all taken care of */
			}

			/*
			 * Oh well, use mapping instead if we're
			 * still within range for it
			 */
			if (cyl >= ncyl) {
				printf("%d/%d/%d:  BAD ALTERNATE SECTOR!\n",
				    cyl, head, sec);
				continue;
			}

			if (mapsector(cyl, head, sec, n))
				continue;	/* out of entries or space */
			addonedef(cyl, head, sec);

			/*
			 * Add newly found bad block to sorted bad block list
			 * Just in case the user doesn't interrupt with ^C
			 * we write out the new table and ZAP the bad sector.
			 */
			s[n++] = bn;
			qsort((char *)s, n, sizeof s[0], scompar);
			s[n] = 0x7fffffff;

			putbad();
			(void) devcmd(ZAP, cyl, head, sec, 1);
		} /* next block */
	} /* next data pass */
}

C_fix()
{
	register int cyl, head, npass;
	register int i, bn, sb, eb;
	register struct bt_bad *b;
	int reslip, flags = ctlrtab[controller].c_flags;
	char buf[LINEBUFSZ];

	if (flags & DKI_FMTVOL) {
		printf("Cannot fix SCSI disks\n");
		return;
	}
	printf("Warning! please use the 'format' command when you are fixing the whole disk\n");

	max_retries = FORMAT_RETRIES;

	/*
	 * Force track boundaries as sector formatting doesn't work
	 * with Eagles (Xylogics bug) and doesn't work when slip
	 * sectoring is being used.
	 */
	printf("fix -- DESTROYS SOME DISK DATA!\n");
	printf("Formats a range of tracks.\n");
	printf("Enter track numbers as 'cyl/track'\n");

tryagain:
	sb = pgetbn("starting track? ");
	eb = pgetbn("ending track? ");
	if (((sb % nsect) != 0 || (eb % nsect) != 0)) {
		printf("can only format on track boundaries!\n");
		goto tryagain;
	}
	if (eb <= sb) {
		printf("ending track must be greater than starting track\n");
		printf("format will NOT include ending track\n");
		goto tryagain;
	}
	if (eb > (ncyl * nhead * nsect)) 
	    eb = ncyl * nhead * nsect;
	npass = getnpass(npassdata);
	printf("OK to format from ");
	putbn(sb);
	printf(" to ");
	putbn(eb - 1);
	printf("? ");

	/*
	 * Instead of using confirm() here we add the ability to
	 * explicitly set the value of reslip if the correct
	 * magic string is given ("yup" - for internal use only).
	 * Normally we will want to reslip previously slipped sector
	 * to preserve any mappings and so we don't lose the fact
	 * that a sector looked bad at some time in the past.
	 */
	gets(buf);
	if (buf[0] != 'y' && buf[0] != 'Y') {
		return;
	} else if (strcmp(buf, "yup") == 0) {
		printf("reslip previously slipped sectors? ");
		reslip = confirm();
	} else {
		reslip = 1;
	}

	getbad(0);
	for (bn = sb; bn < eb; bn += nsect) {
		cyl = bn / (nhead*nsect);
		head = (bn % (nhead*nsect)) / nsect;
		printf("\r%d/%d  ", cyl, head);
		(void) fmttrack(cyl, head, npass, reslip);
	}

	for (i=0, b = dkbad.bt_bad; i < NBAD && b->bt_cyl != 0xffff; i++, b++) {
		bn = b->bt_cyl * nhead * nsect;
		bn += (b->bt_trksec >> 8) * nsect;
		if ((flags & DKI_FMTTRK) == 0)
			bn += b->bt_trksec & 0xff;
		if (bn < sb || bn >= eb)
			continue;
		if (flags & DKI_MAPTRK) {
			int newhead, newcyl;

			if (i == nhead-2)	/* diagnostic track */
				continue;
			newhead = (ncyl+acyl)*nhead - 2 - i;
			newcyl = newhead / nhead;
			newhead %= nhead;
			(void) devcmd(MAP, b->bt_cyl, b->bt_trksec >> 8,
				newhead, newcyl);
		} else if (flags & DKI_FMTTRK) {
			if (i == nhead-2)	/* diagnostic track */
				continue;
			(void) devcmd(ZAP, b->bt_cyl, b->bt_trksec >> 8,
				0, nsect);
		} else if (flags & DKI_BAD144) {
			(void) devcmd(ZAP, b->bt_cyl, b->bt_trksec >> 8,
				b->bt_trksec & 0xff, 1);
		}
	}
	putbad();
}

C_map()
{
	int i, j, newcyl, newhead, newsec;
	register struct bt_bad *b;
	int flags = ctlrtab[controller].c_flags;

	if (flags & DKI_FMTVOL) {
		printf("Cannot map SCSI disks.\n");
		return;
	}
	getbad(0);
	printf("Current mapping:\n");
	for (i=0, b = dkbad.bt_bad; i < NBAD && b->bt_cyl != 0xffff; i++, b++) {
		if (flags & DKI_FMTTRK) {
			if (i == nhead-2)	/* diagnostic track */
				continue;
			newhead = (ncyl+acyl)*nhead - 2 - i;
			newcyl = newhead / nhead;
			newhead %= nhead;
			printf("track %d/%d mapped to %d/%d\n",
				b->bt_cyl, b->bt_trksec >> 8,
				newcyl, newhead);
		} else {
			newsec = (ncyl+acyl)*nhead*nsect - nsect - 1 - i;
			newcyl = newsec / (nhead*nsect);
			newsec %= (nhead*nsect);
			newhead = newsec / nsect;
			newsec %= nsect;
			printf("sector %d/%d/%d mapped to %d/%d/%d\n",
				b->bt_cyl, b->bt_trksec >> 8,
				b->bt_trksec & 0xff,
				newcyl, newhead, newsec);
		}
	}
	if (i == 0)
		printf("None.\n");
	if (i >= NBAD) {
		printf("Map table full - no mapping may be added.\n");
		return;
	}
	
	printf("Do you wish to add a mapping? ");
	if (!confirm())
		return;
	printf("mapping may be removed only by complete format of the disk\n");

	for (;;) {
		j = pgetn("cylinder to be mapped? ");
		if (j >= 0 && j < ncyl)
			break;
		printf("Cylinder number must be within 0 and %d\n",
		    ncyl-1);
	}
	b->bt_cyl = j;
	for (;;) {
		j = pgetn("track to be mapped? ");
		if (j >= 0 && j < nhead)
			break;
		printf("Track number must be within 0 and %d\n", nhead-1);
	}
	b->bt_trksec = j << 8;
	if (flags & DKI_FMTTRK) {
		b->bt_trksec |= nsect;
		newhead = (ncyl+acyl)*nhead - 2 - i;
		newcyl = newhead / nhead;
		newhead %= nhead;

		printf("Attempt to preserve data? ");
		if (confirm()) {
			j = devcmd(READ, b->bt_cyl, b->bt_trksec >> 8, 0,
			    nsect, DBUF_PA);
			j |= devcmd(WRITE, newcyl, newhead, 0,
			    nsect, DBUF_PA);
			if (j)
				printf("Data transfer failed\n");
			else
				printf("Data transfer successful\n");
		}

		printf("OK to map %d/%d? ", b->bt_cyl, b->bt_trksec >> 8);
		if (!confirm()) {
			b->bt_cyl = 0xffff;
			return;
		}
		printf("mapping to %d/%d\n", newcyl, newhead);
		if (flags & DKI_MAPTRK)
			(void) devcmd(MAP, b->bt_cyl, b->bt_trksec >> 8,
				newhead, newcyl);
		else if (flags & DKI_FMTTRK)
			(void) devcmd(ZAP, b->bt_cyl, b->bt_trksec >> 8,
				0, nsect);
	} else {
		for (;;) {
			j = pgetn("sector to be mapped? ");
			if (j >= 0 && j < nsect)
				break;
			printf("Sector number must be within 0 and %d\n",
			    nsect-1);
		}

		b->bt_trksec |= j;
		newsec = (ncyl+acyl)*nhead*nsect - nsect - 1 - i;
		newcyl = newsec / (nhead*nsect);
		newsec %= (nhead*nsect);
		newhead = newsec / nsect;
		newsec %= nsect;

		printf("Attempt to preserve data? ");
		if (confirm()) {
			j = devcmd(READ, b->bt_cyl, b->bt_trksec >> 8,
			    b->bt_trksec & 0xff, 1, DBUF_PA);
			j |= devcmd(WRITE, newcyl, newhead, newsec,
			    1, DBUF_PA);
			if (j)
				printf("Data transfer failed\n");
			else
				printf("Data transfer successful\n");
		}

		printf("OK to map %d/%d/%d? ", b->bt_cyl, b->bt_trksec>>8,
		    b->bt_trksec & 0xff);
		if (!confirm()) {
			b->bt_cyl = 0xffff;
			return;
		}
		printf("mapping to %d/%d/%d\n", newcyl, newhead, newsec);
		if (flags & DKI_BAD144)
			if (devcmd(ZAP, b->bt_cyl, b->bt_trksec >> 8,
				b->bt_trksec & 0xff, 1) != 0)
			    printf("Map command failed\n");
		addonedef((int) b->bt_cyl, (int) (b->bt_trksec >> 8),
				(int) (b->bt_trksec & 0xff));
	}
	putbad();
}

C_slip()
{
	int cyl, head, sec, blk, preserve, e;
	int flags = ctlrtab[controller].c_flags;

	if ((flags & DKI_BAD144) == 0 || controller == C_XY440) {
		printf("Wrong controller type for slipping\n");
		return;
	}

	printf("slip sector\n");
	printf("slipping may be removed only by complete format of the disk\n");
	for (;;) {
		cyl = pgetn("cylinder number? ");
		if (cyl >= 0 && cyl < ncyl+acyl)
			break;
		printf("Cylinder number must be within 0 and %d\n",
		    ncyl+acyl-1);
	}
	for (;;) {
		head = pgetn("track number? ");
		if (head >= 0 && head < nhead)
			break;
		printf("Track number must be within 0 and %d\n", nhead-1);
	}
	for (;;) {
		sec = pgetn("logical sector to be slipped? ");
		if (sec >= 0 && sec < nsect)
			break;
		printf("Sector number must be within 0 and %d\n", nsect-1);
	}

	getbad(1);
	if ((blk = slipcheck(cyl, head, sec)) != -1) {
		printf("Sorry, slipping %d/%d/%d would %s ",
		    cyl, head, sec, "invalidate previous map of");
		putbn(blk);
		printf("!\n");
		return;
	}

	printf("Attempt to preserve data? ");
	if ((preserve = confirm())) {
		/*
		 * Read current track data in, offseting by SECSIZE so
		 * that the SLIP command doesn't trash the data.
		 * This will not succeed if the track has a sector
		 * mapped already.
		 */
		e = devcmd(READ, cyl, head, 0, nsect, DBUF_PA+SECSIZE);
	}

	printf("OK to attempt slip of logical sector %d/%d/%d? ",
	    cyl, head, sec);
	if (!confirm())
		return;

	if (devcmd(SLIP, cyl, head, sec) == 0) {
		addonedef(cyl, head, sec);
		printf("Slip of %d/%d/%d successful\n", cyl, head, sec);
		if (preserve) {
			/*
			 * Write the track data back out to the track.
			 */
			e |= devcmd(WRITE, cyl, head, 0, nsect,DBUF_PA+SECSIZE);
			if (e)
				printf("Data transfer failed\n");
			else
				printf("Data transfer successful\n");
		}
	} else
		printf("Slip of %d/%d/%d failed\n", cyl, head, sec);
}

C_seek()
{
	int cyl, cyl2;
	int begin, end;

	printf("seek test\n");
	printf("perform automatic hour-glass seeks? ");
	if (confirm()) {
		begin = *RomVecPtr->v_RefrCnt;
		for (cyl = 0; cyl < ncyl+acyl; cyl++) {
			(void) devcmd(SEEK, cyl);
			(void) devcmd(SEEK, ncyl+acyl-1-cyl);
		}
		end = *RomVecPtr->v_RefrCnt;
		printf("seek test done - %d ms\n", end-begin);
	} else {
		for (;;) {
			cyl = pgetn("First cylinder number? ");
			if (cyl >= 0 && cyl < ncyl + acyl)
				break;
			printf("Cylinder must be within 0 and %d\n",
			    ncyl + acyl - 1);
		}
		for (;;) {
			cyl2 = pgetn("Second cylinder number? ");
			if (cyl2 >= 0 && cyl2 < ncyl + acyl)
				break;
			printf("Cylinder must be within 0 and %d\n",
			    ncyl + acyl - 1);
		}
		for (;;) {
			(void) devcmd(SEEK, cyl);
			(void) devcmd(SEEK, cyl2);
		}
	}
}

printbad(blk, cnt)
	register blk, cnt;
{
	register int badblk, badcnt;
	register int flags = ctlrtab[controller].c_flags;
	register struct bt_bad *b;
	int found = 0;

	if ((flags & DKI_FMTVOL) || (mapcheck == 0))
		return;

	if (map_read == 0) {
		getbad(1);
		map_read++;
	}

	badcnt = (flags & DKI_FMTTRK) ? nsect : 1;

	for (b = dkbad.bt_bad; b < &dkbad.bt_bad[NBAD] && b->bt_cyl != 0xffff;
	    b++) {
		badblk = b->bt_cyl * (nhead * nsect) +
		    (b->bt_trksec >> 8) * nsect + (b->bt_trksec & 0xff);
		if (badblk >= blk && ((badblk+badcnt) <= (blk+cnt))) {
			if (!found++) {
				if (flags & DKI_FMTTRK)
					printf("[Overlapping track ");
				else
					printf("[Overlapping sector ");
			}
			if (flags & DKI_FMTTRK)
				printf("%d/%d ", badblk/(nsect*nhead),
				    (badblk%(nsect*nhead))/nsect);
			else
				printf("%d/%d/%d ", badblk/(nsect*nhead),
				    (badblk%(nsect*nhead))/nsect, badblk%nsect);
		}
	}
	if (found)
		printf("mapped]\n");
}

transfer(cmd)
{
	int blk, sblk, nblk, incr, nsec, eblk;
	int begin, end;
	register int cyl, head, sec;

	sblk = pgetbn("starting block? ");
	nblk = pgetbn("# of blocks? ");
	incr = pgetbn("increment? ");
	nsec = pgetbn("# of blocks per transfer? ");

	eblk = sblk+nblk;
	if (eblk > ((ncyl+acyl)*(nhead*nsect-nspare))) {
		eblk = (ncyl+acyl)*(nhead*nsect-nspare);
		printf("Operation truncated at end of disk\n");
	}
	if (eblk <= sblk)
		return;
	if (cmd == WRITE)
		if (eblk > (ncyl*(nhead*nsect-nspare))) {
			printf("\nWrite includes alternate area, ");
			printf("are you sure? ");
			if (!confirm())
				eblk = ncyl*(nhead*nsect-nspare);
		}

	begin = *RomVecPtr->v_RefrCnt;
	for (blk = sblk; blk < eblk; blk += incr) {
		cyl = blk / (nhead * nsect - nspare);
		sec = blk % (nhead * nsect - nspare);
		head = sec / nsect;
		sec %= nsect;
		if (eblk-blk < nsec)
			nsec = eblk-blk;
		if (!timing)
			printf("\r%d/%d/%d  ", cyl, head, sec);
		if (devcmd(cmd, cyl, head, sec, nsec, DBUF_PA) && !scsi)
			printbad(blk, nsec);
	}
	end = *RomVecPtr->v_RefrCnt;
	if (timing)
		printf("%d ms\n", end-begin);
}

C_write()
{

	printf("write\n");
	transfer(WRITE);
}

C_read()
{

	printf("read\n");
	transfer(READ);
}

C_test()
{
	int cyl, head, sect, data;
	int n;
	uchar *bp;

	printf("test\n");
	n = pgetbn("# of sectors per transfer? ");
	printf("This writes on disk, ok to proceed? ");
	if (!confirm())
		return;

	for (;;) {
		cyl = random() % ncyl;
		head = random() % nhead;
		if (head == nhead - 1)
			sect = random() % (nsect - nspare);
		else
			sect = random() % nsect;
		data = random() % 256;

		for (bp = DBUF_VA; bp < (DBUF_VA+n*SECSIZE); bp++)
			*bp = data;

		if (devcmd(WRITE, cyl, head, sect, n, DBUF_PA) && !scsi) {
			printbad(cyl*nhead*nsect+head*nsect+sect, n);
			continue;
		}

		for (bp = DBUF_VA; bp < (DBUF_VA+n*SECSIZE); bp++)
			*bp = 0x55;

		if (devcmd(READ, cyl, head, sect, n, DBUF_PA))
			continue;

		for (bp = DBUF_VA; bp < (DBUF_VA+n*SECSIZE); bp++) {
			if (*bp != data) {
				printf("%d|%d|%d|%x ", cyl, head, sect, data);
				printf("!%x@%d ", *bp, bp-DBUF_VA);
				break;
			}
		}
	}
}

C_position()
{
	int cyl, head, sect;

	printf("position test\n");
	for (;;) {
		cyl = random() % ncyl;
		head = random() % nhead;
		if (head == nhead - 1)
			sect = random() % (nsect - nspare);
		else
			sect = random() % nsect;
		if (devcmd(READ, cyl, head, sect, 1, DBUF_PA) && !scsi)
			printbad(cyl*nhead*nsect+head*nsect+sect, 1);
	}
}

C_status()
{

	printf("status\n");
	(void) devcmd(STATUS);
}

print_bn(bn)
	int bn;
{

	printf("= %d = 0x%x = %d/%d/%d (logical)\n", bn, bn,
		bn/(nsect*nhead-nspare), (bn%(nsect*nhead-nspare))/nsect,
		(bn%(nsect*nhead-nspare))%nsect);
}

C_add()
{
	int bn;

	bn  = pgetbn("number: ");
	bn += pgetbn("  plus: ");
	print_bn(bn);
}

C_sub()
{
	int bn;

	bn  = pgetbn("number: ");
	bn -= pgetbn(" minus: ");
	print_bn(bn);
}

C_translate()
{

	print_bn(pgetbn("block number? "));
}

C_help()
{

	printf(" -- commands are:\n");
	printf("diag (to re-initialize), quit\n");
	printf("label, verify, partition\n");
	printf("format, map, slip, scan, fix (partial formatting)\n");
	printf("version, clear (to clear drive faults)\n");
	printf("status, translate, + (add), - (subtract)\n");
	printf("read, write, seek, test, position\n");
	printf("errors (off/on), info messages (off/on), time (off/on)\n");
	printf("slipmsgs (off/on), abortdma (on/off), mapcheck (on/off)\n");
	printf("dmatest, rhdr, whdr (read/write track headers)\n");
	printf("formatmsgs (off/on), sformat\n");
}

C_version()
{
 	extern char etext, edata;
 	register char *cp;
 	static char pat[] = "(#)";	/* Skip '@' so we don't match here! */

 	for (cp = &etext; cp < &edata; cp++) {
		if (*cp == '@' && strncmp(cp+1, pat, sizeof (pat) - 1) == 0) {
			cp += sizeof (pat);
			while (*cp && *cp != '"' && *cp != '>' && *cp != '\n')
				putchar(*cp++);
			putchar('\n');
 		}
 	}
}

confirm()
{
	char buf[LINEBUFSZ];

	gets(buf);
	return (buf[0] == 'y' || buf[0] == 'Y');
}

/*VARARGS1*/
devcmd(cmd, p1, p2, p3, p4, p5, p6, p7, p8) 
{

	tryabort();
	return (*ctlrtab[controller].c_cmd) (cmd,p1,p2,p3,p4,p5,p6,p7,p8);
}

tryabort()
{
	int c;

	c = maygetchar();
	if (c == ('c' & 037)) {
		printf("\nCommand aborted\n");
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}
}

random()
{

	return (rand() & 0x7fffffff);
}
