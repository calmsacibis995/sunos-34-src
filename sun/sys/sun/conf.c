#ifndef lint
static	char sccsid[] = "@(#)conf.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/tty.h"
#include "../h/conf.h"
#include "../h/text.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/acct.h"
#include "../h/stream.h"

extern int nulldev();
extern int nodev();

#include "ip.h"
#if NIP > 0
extern int ipopen(), ipstrategy(), ipread(), ipwrite();
extern int ipdump(), ipioctl(), ipsize();
#else
#define	ipopen		nodev
#define	ipstrategy	nodev
#define	ipread		nodev
#define	ipwrite		nodev
#define	ipdump		nodev
#define	ipioctl		nodev
#define	ipsize		0
#endif

#include "xy.h"
#if NXY > 0
extern int xyopen(), xystrategy(), xyread(), xywrite();
extern int xydump(), xyioctl(), xysize();
#else
#define	xyopen		nodev
#define	xystrategy	nodev
#define	xyread		nodev
#define	xywrite		nodev
#define	xydump		nodev
#define	xyioctl		nodev
#define	xysize		0
#endif

#include "mt.h"
#if NMT > 0
extern int tmopen(), tmclose(), tmstrategy(), tmread(), tmwrite();
extern int tmdump(), tmioctl();
#else
#define	tmopen		nodev
#define	tmclose		nodev
#define	tmstrategy	nodev
#define	tmread		nodev
#define	tmwrite		nodev
#define	tmdump		nodev
#define	tmioctl		nodev
#endif

#include "xt.h"
#if NXT > 0
extern int xtopen(), xtclose(), xtstrategy(), xtread(), xtwrite(), xtioctl();
#else
#define	xtopen		nodev
#define	xtclose		nodev
#define	xtstrategy	nodev
#define	xtread		nodev
#define	xtwrite		nodev
#define	xtioctl		nodev
#endif

#include "ar.h"
#if NAR > 0
extern int aropen(), arclose(), arstrategy(), arread(), arwrite(), arioctl();
#else
#define	aropen		nodev
#define	arclose		nodev
#define	arstrategy	nodev
#define	arread		nodev
#define	arwrite		nodev
#define	arioctl		nodev
#endif

#include "nd.h"
#if NND > 0
extern int ndopen(), ndstrategy(), ndread(), ndwrite();
extern int nddump(), ndioctl(), ndsize();
#else
#define	ndopen		nodev
#define	ndstrategy	nodev
#define	ndread		nodev
#define	ndwrite		nodev
#define	nddump		nodev
#define	ndioctl		nodev
#define	ndsize		0
#endif

#include "sd.h"
#if NSD > 0
extern int sdopen(), sdstrategy(), sdread(), sdwrite();
extern int sddump(), sdioctl(), sdsize();
#else
#define	sdopen		nodev
#define	sdstrategy	nodev
#define	sdread		nodev
#define	sdwrite		nodev
#define	sddump		nodev
#define	sdioctl		nodev
#define	sdsize		0
#endif

#include "sf.h"
#if NSF > 0
extern int sfopen(), sfclose(), sfstrategy(), sfread(), sfwrite();
extern int sfioctl(), sfsize();
#else
#define	sfopen		nodev
#define	sfclose		nodev
#define	sfstrategy	nodev
#define	sfread		nodev
#define	sfwrite		nodev
#define	sfioctl		nodev
#define	sfsize		0
#endif

extern int swstrategy(), swread(), swwrite();

struct bdevsw	bdevsw[] =
{
	{ ipopen,	nulldev,	ipstrategy,	ipdump,		/*0*/
	  ipsize,	0 },
	{ tmopen,	tmclose,	tmstrategy,	tmdump,		/*1*/
	  0,		B_TAPE },
	{ nodev,	nodev,		nodev,		nodev,		/*2*/
	  0,		B_TAPE },				/* was ar */
	{ xyopen,	nulldev,	xystrategy,	xydump,		/*3*/
	  xysize,	0 },
	{ nodev,	nodev,		swstrategy,	nodev,		/*4*/
	  0,		0 },
	{ ndopen,	nulldev,	ndstrategy,	nddump,		/*5*/
	  ndsize,	0 },
	{ nodev,	nodev,		nodev,		nodev,		/*6*/
	  0,		0 },
	{ sdopen,	nulldev,	sdstrategy,	sddump,		/*7*/
	  sdsize,	0 },
	{ xtopen,	xtclose,	xtstrategy,	nodev,		/*8*/
	  0,		B_TAPE },
	{ sfopen,	sfclose,	sfstrategy,	nodev,		/*9*/
	  sfsize,	0 },
	{ nodev,	nodev,		nodev,		nodev,		/*10*/
	  0,		0 },
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

extern int cnopen(), cnclose(), cnread(), cnwrite(), cnioctl(), cnselect();
extern struct tty cons;

extern int conskbdopen(), conskbdclose(), conskbdread(), conskbdioctl();
extern int conskbdselect(), consfbopen(), consfbclose();
extern int consfbioctl(), consfbmmap();

#include "ms.h"
#if NMS > 0
extern int consmsopen(), consmsclose(), consmsread();
extern int consmsselect(), consmsioctl();
#else
#define	consmsopen	nodev
#define	consmsclose	nodev
#define	consmsread	nodev
#define	consmsselect	nodev
#define	consmsioctl	nodev
#endif

extern int syopen(), syread(), sywrite(), syioctl(), syselect();

extern int mmopen(), mmread(), mmwrite(), mmmmap();
#define	mmselect	seltrue

#include "vp.h"
#if NVP > 0
extern int vpopen(), vpclose(), vpwrite(), vpioctl();
#else
#define	vpopen		nodev
#define	vpclose		nodev
#define	vpwrite		nodev
#define	vpioctl		nodev
#endif

#include "vpc.h"
#if NVPC > 0
extern int vpcopen(), vpcclose(), vpcwrite(), vpcioctl();
#else
#define	vpcopen		nodev
#define	vpcclose	nodev
#define	vpcwrite	nodev
#define	vpcioctl	nodev
#endif

#include "zs.h"
#if NZS > 0
extern int zsopen(), zsclose(), zsread(), zswrite();
extern int zsioctl(), zsstop(), zsselect();
extern struct tty zs_tty[];
#else
#define	zsopen	nodev
#define	zsclose	nodev
#define	zsread	nodev
#define	zswrite	nodev
#define	zsioctl	nodev
#define	zsstop	nodev
#define	zs_tty	0
#define	zsselect nodev
#endif

#include "pty.h"
#if NPTY > 0
extern int ptsopen(), ptsclose(), ptsread(), ptswrite(), ptsstop();
extern int ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptcselect();
extern int ptyioctl();
extern struct tty pt_tty[];
#else
#define	ptsopen		nodev
#define	ptsclose	nodev
#define	ptsread		nodev
#define	ptswrite	nodev
#define	ptcopen		nodev
#define	ptcclose	nodev
#define	ptcread		nodev
#define	ptcwrite	nodev
#define	ptyioctl	nodev
#define	pt_tty		0
#define	ptcselect	nodev
#define	ptsstop		nulldev
#endif

#include "ropc.h"
#if NROPC > 0
extern int ropcopen(), ropcmmap();
#else
#define	ropcopen	nodev
#define	ropcmmap	nodev
#endif

#include "mti.h"
#if NMTI > 0
extern int mtiopen(), mticlose(), mtiread(), mtiwrite(), mtiioctl();
extern int mtiioctl(), mtistop(), mtireset(), mtiselect();
extern struct tty mti_tty[];
#else
#define	mtiopen		nodev
#define	mticlose	nodev
#define	mtiread		nodev
#define	mtiwrite	nodev
#define	mtiioctl	nodev
#define	mtistop		nodev
#define	mtireset	nulldev
#define	mtiselect	nulldev
#define	mti_tty	0
#endif

#include "cgone.h"
#if NCGONE > 0
extern int cgoneopen(), cgonemmap(), cgoneioctl();
extern int cgoneclose();
#else
#define	cgoneopen	nodev
#define	cgonemmap	nodev
#define	cgoneioctl	nodev
#define	cgoneclose	nodev
#endif

#include "cgtwo.h"
#if NCGTWO > 0
extern int cgtwoopen(), cgtwommap(), cgtwoioctl();
extern int cgtwoclose();
#else
#define	cgtwoopen	nodev
#define	cgtwommap	nodev
#define	cgtwoioctl	nodev
#define	cgtwoclose	nodev
#endif


#include "cgfour.h"
#if NCGFOUR > 0
extern int cgfouropen(), cgfourclose(), cgfourioctl(), cgfourmmap();
#else
#define	cgfouropen	nodev
#define	cgfourclose	nodev
#define	cgfourioctl	nodev
#define	cgfourmmap	nodev
#endif

#include "gpone.h"
#if NGPONE > 0
extern int gponeopen(), gponemmap(), gponeioctl();
extern int gponeclose();
#else
#define	gponeopen	nodev
#define	gponemmap	nodev
#define	gponeioctl	nodev
#define	gponeclose	nodev
#endif

#include "win.h"
#if NWIN > 0
extern int winopen(), winclose(), winread(), winioctl(), winmmap(), winselect();
#else
#define	winopen		nodev
#define	winclose	nodev
#define	winread		nodev
#define	winioctl	nodev
#define winmmap		nodev
#define	winselect	nodev
#endif

#include "st.h"
#if NST > 0
extern int stopen(), stclose(), stread(), stwrite(), stioctl();
#else
#define	stopen		nodev
#define	stclose		nodev
#define	stread		nodev
#define	stwrite		nodev
#define	stioctl		nodev
#endif

#include "sky.h"
#if NSKY > 0
extern int skyopen(), skyclose(), skymmap(), skyioctl();
#else
#define	skyopen		nodev
#define	skyclose	nodev
#define	skymmap		nodev
#define	skyioctl	nodev
#endif

#include "pi.h"
#if NPI > 0
extern int piopen(), piclose(), piread(), piioctl();
extern struct tty pitty[];
#else
#define	piopen	nodev
#define	piclose nodev
#define	piread	nodev
#define	piioctl nodev
#define	pitty	0
#endif

#include "bwone.h"
#if NBWONE > 0
extern int bwoneopen(), bwonemmap(), bwoneioctl();
extern int bwoneclose();
#else
#define	bwoneopen	nodev
#define	bwonemmap	nodev
#define	bwoneioctl	nodev
#define	bwoneclose	nodev
#endif

#include "bwtwo.h"
#if NBWTWO > 0
extern int bwtwoopen(), bwtwommap(), bwtwoioctl();
extern int bwtwoclose();
#else
#define	bwtwoopen	nodev
#define	bwtwommap	nodev
#define	bwtwoioctl	nodev
#define	bwtwoclose	nodev
#endif

#include "des.h"
#if NDES > 0
extern int desopen(), desclose(), desioctl();
#else
#define	desopen		nodev
#define	desclose	nodev
#define	desioctl	nodev
#endif

#include "fpa.h"
#if NFPA > 0
extern int fpaopen(), fpaclose(), fpaioctl();
#else
#define fpaopen		nodev
#define fpaclose	nodev
#define fpaioctl	nodev
#endif

#include "sp.h"
#if NSP > 0
extern struct streamtab spinfo;
#define	sptab	&spinfo
#else
#define	sptab	0
#endif

#include "clone.h"
#if NCLONE > 0
extern struct streamtab cloneinfo;
#define	clonetab	&cloneinfo
#else
#define	clonetab	0
#endif

#include "pc.h"
#if NPC > 0
extern int pcopen(), pcclose(), pcioctl(), pcselect(), pcmmap();
#else
#define	pcopen		nodev
#define	pcclose		nodev
#define	pcioctl		nodev
#define	pcselect	nodev
#define	pcmmap		nodev
#endif

extern int ttselect(), seltrue();

struct cdevsw	cdevsw[] =
{
    {
	cnopen,		cnclose,	cnread,		cnwrite,	/*0*/
	cnioctl,	nulldev,	nulldev,	&cons,
	cnselect,	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*1*/
	nodev,		nodev,		nodev,		0,
	nodev,		0,		0,
    },
    {
	syopen,		nulldev,	syread,		sywrite,	/*2*/
	syioctl,	nulldev,	nulldev,	0,
	syselect,	0,		0,
    },
    {
	mmopen,		nulldev,	mmread,		mmwrite,	/*3*/
	nodev,		nulldev,	nulldev,	0,
	mmselect,	mmmmap,		0,
    },
    {
	ipopen,		nulldev,	ipread,		ipwrite,	/*4*/
	ipioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {
	tmopen,		tmclose,	tmread,		tmwrite,	/*5*/
	tmioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {
	vpopen,		vpclose,	nodev,		vpwrite,	/*6*/
	vpioctl,	nulldev,	nulldev,	0,
	seltrue,	0,		0,
    },
    {
	nulldev,	nulldev,	swread,		swwrite,	/*7*/
	nodev,		nodev,		nulldev,	0,
	nodev,		0,		0,
    },
    {
	aropen,		arclose,	arread,		arwrite,	/*8*/
	arioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {
	xyopen,		nulldev,	xyread,		xywrite,	/*9*/
	xyioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {
	mtiopen,	mticlose,	mtiread,	mtiwrite,	/*10*/
	mtiioctl,	mtistop,	mtireset,	mti_tty,
	mtiselect,	0,		0,
    },
    {
	desopen,	desclose,	nodev,		nodev,		/*11*/
	desioctl,	nodev,		nulldev,	0,
	nodev,		0,		0,
    },
    {
	zsopen,		zsclose,	zsread,		zswrite,	/*12*/
	zsioctl,	zsstop,		nulldev,	zs_tty,
	zsselect,	0,		0,
    },
    {
	consmsopen,	consmsclose,	consmsread,	nodev,		/*13*/
	consmsioctl,	nodev,		nodev,		0,
	consmsselect,	0,		0,
    },
    {
	cgoneopen,	cgoneclose,	nodev,		nodev,		/*14*/
	cgoneioctl,	nodev,		nodev,		0,
	seltrue,	cgonemmap,	0,
    },
    {
	winopen,	winclose,	winread,	nodev,		/*15*/
	winioctl,	nodev,		nodev,		0,
	winselect,	winmmap,	0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*16*/
	nodev,		nodev,		nodev,		0,
	seltrue,	0,		0,
    },
    {
	sdopen,		nulldev,	sdread,		sdwrite,	/*17*/
	sdioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {
	stopen,		stclose,	stread,		stwrite,	/*18*/
	stioctl,	nodev,		nodev,		0,
	nodev,		0,		0,
    },
    {
	ndopen,		nulldev,	ndread,		ndwrite,	/*19*/
	ndioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {
	ptsopen,	ptsclose,	ptsread,	ptswrite,	/*20*/
	ptyioctl,	ptsstop,	nodev,		pt_tty,
	ttselect,	0,		0,
    },
    {
	ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*21*/
	ptyioctl,	nulldev,	nodev,		pt_tty,
	ptcselect,	0,		0,
    },
    {
	consfbopen,	consfbclose,	nodev,		nodev,		/*22*/
	consfbioctl,	nodev,		nodev,		0,
	nodev,		consfbmmap,	0,
    },
    {
	ropcopen,	nulldev,	nodev,		nodev,		/*23*/
	nodev,		nodev,		nulldev,	0,
	nodev,		ropcmmap,	0,
    },
    {
	skyopen,	skyclose,	nodev,		nodev,		/*24*/
	skyioctl,	nodev,		nulldev,	0,
	nodev,		skymmap,	0,
    },
    {
	piopen,		piclose,	piread,		nodev,		/*25*/
	piioctl,	nodev,		nodev,		pitty,
	ttselect,	0,		0,
    },
    {
	bwoneopen,	bwoneclose,	nodev,		nodev,		/*26*/
	bwoneioctl,	nodev,		nodev,		0,
	seltrue,	bwonemmap,	0,
    },
    {
	bwtwoopen,	bwtwoclose,	nodev,		nodev,		/*27*/
	bwtwoioctl,	nodev,		nodev,		0,
	seltrue,	bwtwommap,	0,
    },
    { 
	vpcopen,	vpcclose,	nodev,		vpcwrite,	/*28*/
	vpcioctl,	nulldev,	nulldev,	0,
	seltrue,	0,		0,
    },
    {
	conskbdopen,	conskbdclose,	conskbdread,	nodev,		/*29*/
	conskbdioctl,	nulldev,	nulldev,	0,
	conskbdselect,	0,		0,
    },
    {
	xtopen,		xtclose,	xtread,		xtwrite,	/*30*/
	xtioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    { 
	cgtwoopen,	cgtwoclose,	nodev,		nodev,		/*31*/
	cgtwoioctl,	nodev,		nodev,		0,
	seltrue,	cgtwommap,	0,
    },
    { 
	gponeopen,	gponeclose,	nodev,		nodev,		/*32*/
	gponeioctl,	nodev,		nodev,		0,
	seltrue,	gponemmap,	0,
    },
    {
	sfopen,		sfclose,	sfread,		sfwrite,	/*33*/
	sfioctl,	nodev,		nulldev,	0,
	seltrue,	0,		0,
    },
    {	
	fpaopen,	fpaclose,	nodev,		nodev,		/*34*/
	fpaioctl,	nodev,		nodev,		0,
	nodev,		0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*35*/
	nodev,		nodev,		nodev,		0,
	nodev,		0,		sptab,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*36*/
	nodev,		nodev,		nodev,		0,
	nodev,		0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*37*/
	nodev,		nodev,		nodev,		0,
	nodev,		0,		clonetab,
    },
    {
	pcopen,		pcclose,	nodev,		nodev,		/*38*/
	pcioctl,	nodev,		nodev,		0,
	pcselect,	pcmmap,		0,
    },
    {
	cgfouropen,	cgfourclose,	nodev,		nodev,		/*39*/
	cgfourioctl,	nodev,		nodev,		0,
	seltrue,	cgfourmmap,	0,
    },
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 3;	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t swapdev = makedev(4, 0);
struct vnode *swapdev_vp;
