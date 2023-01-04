#ifndef lint
static	char sccsid[] = "@(#)autoconf.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the Mainbus
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/ioctl.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/kernel.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../sun/autoconf.h"
#include "../sun/consdev.h"
#include "../sun/fbio.h"
#include "../sundev/mbvar.h"
#include "../sundev/kbd.h"
#include "../sundev/kbio.h"
#include "../sundev/zsvar.h"
#include "../mon/idprom.h"

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	dkn;		/* number of iostat dk numbers assigned so far */

/*
 * This allocates the space for the per-Mainbus information.
 */
struct	mb_hd mb_hd;

/*
 * Maximum interrupt priority used by Mainbus DMA.
 * This value is determined by taking the max of the m[cd]_intpri
 * field from the mb_{cltr,device} structures which are found to
 * exist and have the MDR_BIODMA or MDR_DMA flags on in their
 * corresponding mb_driver structure.
 */
int SPLMB = 2;		/* reasonable default */

/*
 * Determine mass storage and memory configuration for a machine.
 * Get cpu type, and then switch out to machine specific procedures
 * which will probe adaptors to see what is out there.
 */
configure()
{

	idprom();
	/*
	 * Configure the Mainbus.
	 */
	mbconfig();
	/*
	 * Attach pseudo-devices
	 */
	pseudoconfig();
#ifdef GENERIC
	setconf();
#endif
}

static	caddr_t mbinuse;	/* pointer to array of allocated mbio space */
#define	mbaddr(addr)	(u_short *)(&mbio[addr])
static	int	(*vec_save)();	/* used to save original vector value */

/*
 * Find devices on the Mainbus.
 * Uses per-driver routine to probe for existence of the device
 * and then fills in the tables, with help from a per-driver
 * slave initialization routine.
 */
mbconfig()
{
	register struct mb_device *md;
	register struct mb_ctlr *mc;
	u_short *reg;
	struct mb_driver *mdr;
	caddr_t zmemall();
	u_short *doprobe();

	vec_save = scb.scb_user[0];	/* save default trap routine */

	/*
	 * Grab some memory to record the Mainbus address space in use,
	 * so we can be sure not to place two devices at the same address.
	 * If we run out of kernelmap space, we could reuse the mapped
	 * pages if we did all probes first to determine the target
	 * locations and sizes, and then remucked with the kernelmap to
	 * share spaces, then did all the attaches.
	 *
	 * We could use just 1/8 of this (we only want a 1 bit flag) but
	 * we are going to give it back anyway, and that would make the
	 * code here bigger (which we can't give back), so ...
	 */
	mbinuse = zmemall(memall, 0x10000);
	if (mbinuse == (caddr_t)0)
		panic("no mem for mbconfig");

	/*
	 * Check each Mainbus mass storage controller.
	 * See if it is really there, and if it is record it and
	 * then go looking for slaves.
	 */
	for (mc = mbcinit; mdr = mc->mc_driver; mc++) {
		if (cpu != CPU_SUN2_50) {
			/* disallow any use of vectored interrupts */
			mc->mc_intr = (struct vec *)0;
		}
		if ((reg = doprobe((u_long)mc->mc_addr, (u_long)mc->mc_space,
		    mdr, mdr->mdr_cname, mc->mc_ctlr, mc->mc_intpri,
		    mc->mc_intr)) == 0)
			continue;
		if (((mdr->mdr_flags & (MDR_BIODMA | MDR_DMA)) != 0) &&
		    mc->mc_intpri > SPLMB)
			SPLMB = mc->mc_intpri;
		mc->mc_alive = 1;
		mc->mc_mh = &mb_hd;
		mc->mc_addr = (caddr_t)reg;
		if (mdr->mdr_cinfo)
			mdr->mdr_cinfo[mc->mc_ctlr] = mc;
		for (md = mbdinit; md->md_driver; md++) {
			if (md->md_driver != mdr || md->md_alive ||
			    md->md_ctlr != mc->mc_ctlr && md->md_ctlr != '?')
				continue;
			if ((*mdr->mdr_slave)(md, reg)) {
				md->md_alive = 1;
				md->md_ctlr = mc->mc_ctlr;
				md->md_hd = &mb_hd;
				md->md_addr = (caddr_t)reg;
				if (md->md_dk && dkn < DK_NDRIVE)
					md->md_dk = dkn++;
				else
					md->md_dk = -1;
				md->md_mc = mc;
				/* md_type comes from driver */
				if (mdr->mdr_dinfo)
					mdr->mdr_dinfo[md->md_unit] = md;
				printf("%s%d at %s%d slave %d\n",
				    mdr->mdr_dname, md->md_unit,
				    mdr->mdr_cname, mc->mc_ctlr, md->md_slave);
				if (mdr->mdr_attach)
					(*mdr->mdr_attach)(md);
			}
		}
	}

	/*
	 * Now look for non-mass storage peripherals.
	 */
	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (cpu != CPU_SUN2_50) {
			/* disallow any use of vectored interrupts */
			md->md_intr = (struct vec *)0;
		}
		if (md->md_alive || md->md_slave != -1)
			continue;
		if ((reg = doprobe((u_long)md->md_addr, (u_long)md->md_space,
		    mdr, mdr->mdr_dname, md->md_unit, md->md_intpri,
		    md->md_intr)) == 0)
			continue;
		if (((mdr->mdr_flags & (MDR_BIODMA | MDR_DMA)) != 0) &&
		    md->md_intpri > SPLMB)
			SPLMB = md->md_intpri;
		md->md_hd = &mb_hd;
		md->md_alive = 1;
		md->md_addr = (caddr_t)reg;
		md->md_dk = -1;
		/* md_type comes from driver */
		if (mdr->mdr_dinfo)
			mdr->mdr_dinfo[md->md_unit] = md;
		if (mdr->mdr_attach)
			(*mdr->mdr_attach)(md);
	}

	wmemfree(mbinuse, 0x10000);
}

/*
 * Make non-zero if want to be set up to handle
 * both vectored and auto-vectored interrupts
 * for the same device at the same time.
 */
int paranoid = 0;

/*
 * Probe for a device or controller at the specified addr.
 * The space argument give the page type and cpu type for the device.
 */
u_short *
doprobe(addr, space, mdr, dname, unit, br, vp)
	register u_long addr, space;
	register struct mb_driver *mdr;
	char *dname;
	int unit, br;
	register struct vec *vp;
{
	register u_short *reg = NULL;
	char *name;
	long a = 0;
	int ismbio = 0, i, devsize, extent, machine;
	u_int pageval;

	machine = space & SP_MACHMASK;

	/*
	 * Check to see that the machine type
	 * is something we understand
	 */
	if (machine != SP_MACH_ALL && machine != MAKE_MACH(cpu & CPU_MACH))
		return (0);

	switch (space & SP_BUSMASK) {

	case SP_VIRTUAL:
		name = "virtual";
		reg = (u_short *)addr;
		break;

	case SP_OBMEM:
		name = "obmem";
		pageval = PGT_OBMEM | btop(addr);
		break;

	case SP_OBIO:
		name = "obio";
		pageval = PGT_OBIO | btop(addr);
		break;

#ifdef SUN2_120
	case SP_MBMEM:
		if (cpu != CPU_SUN2_120)
			return (0);
		name = "mbmem";
		pageval = PGT_MBMEM | btop(addr);
		break;

	case SP_MBIO:
		if (cpu != CPU_SUN2_120 || mbinuse[addr])
			return (0);
		name = "mbio";
		reg = mbaddr(addr);
		ismbio = 1;
		break;
#endif SUN2_120

#ifdef SUN2_50
	case SP_VME16D16:
		if (cpu != CPU_SUN2_50)
			return (0);
		name = "vme16";
		addr &= ((1 << 16) - 1);	/* paranoid */
		pageval = PGT_VME8 | btop((VME8_SIZE - (1 << 16)) | addr);
		break;

	case SP_VME24D16:
		if (cpu != CPU_SUN2_50)
			return (0);
		name = "vme24";
		if (addr < VME0_SIZE)
			pageval = PGT_VME0 | btop(addr);
		else
			pageval = PGT_VME8 | btop(addr - VME0_SIZE);
		break;
#endif SUN2_50

	default:
		return (0);
	}

	if (reg == NULL) {
		int offset = addr & PGOFSET;

		extent = btoc(mdr->mdr_size + offset);
		if (extent == 0)
			extent = 1;
		if ((a = rmalloc(kernelmap, (long)extent)) == 0)
			panic("out of kernelmap for devices");
		reg = (u_short *)((int)kmxtob(a) | offset);
		mapin(&Usrptmap[a], btop(reg), pageval, extent, PG_V|PG_KW);
	}

	i = devsize = (*mdr->mdr_probe)(reg, unit);
	if (i == 0) {
		if (a)
			rmfree(kernelmap, (long)extent, a);
		return (0);
	}
	printf("%s%d at %s %x ", dname, unit, name, addr);
	if (br < 0 || br >= 7) {
		printf("bad priority (%d)\n", br);
		if (a)
			rmfree(kernelmap, (long)extent, a);
		return (0);
	}

	/*
	 * acquire resources after locating device
	 */
	if (ismbio)
		while (--i >= 0) {
			if (mbinuse[addr+i]) {
			    printf("\tmbio overlap -- discarding device\n");
				while (i++ < devsize)
					/* reclaim space */
					mbinuse[addr+i] = 0;
				return (0);
			}
			mbinuse[addr+i] = 1;
		}

	/*
	 * If br is 0, then no priority was specified in the
	 * config file and the device cannot use interrupts.
	 */
	if (br != 0) {
		/*
		 * If we are paranoid or vectored interrupts are not
		 * going to be used then set up for polling interrupts.
		 */
		if (paranoid || vp == (struct vec *)0) {
			printf("pri %d ", br);
			addintr(br, mdr);
		}

#ifdef SUN2_50
		/*
		 * now set up vectored interrupts if conditions are right
		 */
		if (vp != (struct vec *)0) {
			for (; vp->v_func; vp++) {
				printf("vec 0x%x ", vp->v_vec);
				if (vp->v_vec < VEC_MIN || vp->v_vec > VEC_MAX)
					panic("bad vector");
				else if (scb.scb_user[vp->v_vec - VEC_MIN] !=
				    vec_save)
					panic("duplicate vector");
				else
					scb.scb_user[vp->v_vec - VEC_MIN] =
					    vp->v_func;
			}
		}
#endif SUN2_50
	}
	printf("\n");
	return (reg);
}

#define SPURIOUS	0x80000000	/* recognized in locore.s */

int level2_spurious, level3_spurious, level4_spurious;

not_serviced2()
{

	call_default_intr();
	if ((level2_spurious++ % 100) == 1)
		printf("iobus level 2 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced3()
{

	call_default_intr();
	if ((level3_spurious++ % 100) == 1)
		printf("iobus level 3 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced4()
{

	call_default_intr();
	if ((level4_spurious++ % 100) == 1)
		printf("iobus level 4 interrupt not serviced\n");
	return (SPURIOUS);
}

typedef	int (*func)();

#define NVECT 10

/*
 * These vectors are used in locore.s to jump to device interrupt routines.
 */
func	level2_vector[NVECT] = {not_serviced2};
func	level3_vector[NVECT] = {not_serviced3};
func	level4_vector[NVECT] = {not_serviced4};

func	*vector[7] = {NULL, NULL, level2_vector, level3_vector,
	level4_vector, NULL, NULL};

/*
 * Arrange for a driver to be called when a particular 
 * auto-vectored interrupt occurs.
 * NOTE: every device sharing a driver must be on the 
 * same interrupt level for polling interrupts because
 * there is only one entry made per driver.
 */
addintr(lvl, mdr)
	struct mb_driver *mdr;
{
	register func f;
	register func *fp;
	register int i;

	switch (lvl) {
	case 1:
		return;		/* bogus - these devices don't interrupt */
	case 2:
		fp = level2_vector;
		break;
	case 3:
		fp = level3_vector;
		break;
	case 4:
		fp = level4_vector;
		break;
	case 5:
	case 6:
	case 7:
	default:
		printf("cannot set up polling level %d interrupts\n", lvl);
		panic("addintr");
		/*NOTREACHED*/
	}
	if ((f = mdr->mdr_intr) == NULL)
		return;
	for (i = 0; i < NVECT; i++) {
		if (*fp == NULL)	/* end of list found */
			break;
		if (*fp == f)		/* already in list */
			return;
		fp++;
	}
	if (i >= NVECT)
		panic("addintr: too many devices");
	fp[0] = fp[-1];		/* move not_serviced to end */
	fp[-1] = f;		/* add f to list */
}

/*
 * This is for crazy devices that don't know when they interrupt.
 * We just call them at the end after all the sane devices have decided
 * the interrupt is not their fault.
 */
func	default_intrs[NVECT];

add_default_intr(f)
	func f;
{
	register int i;
	register func *fp;

	fp = default_intrs;
	for (i = 0; i < NVECT; i++) {
		if (*fp == NULL)	/* end of list found */
			break;
		if (*fp == f)		/* already in list */
			return;
		fp++;
	}
	if (i >= NVECT)
		panic("add_default_intr: too many devices");
	*fp = f;		/* add f to list */
}

call_default_intr()
{
	register func *fp;

	for (fp = default_intrs; *fp; fp++)
		(*fp)();
}

#define	MEG	(1024*1024/DEV_BSIZE)

struct {
	int	space;
	int	dmmax;
} swptab[] = {
	16*MEG,	1024,
	12*MEG,	1024,
	8*MEG,	1024,
	6*MEG,	512,
	0,	256,
};

int	dmmin = 0, dmmax = 0, dmtext = 0;
#define	DMMIN	32

#ifdef NFSSWAP
int	swaponnfs = 1;
struct vnode *config_nfsswap(), *nfs_swap();
#endif
/*
 * Configure swap space and related parameters.
 */
swapconf()
{
	register struct swdevt *swp;
	register int nblks, totblks = 0;
	extern long dumpsize;
	struct vattr vattr;
	int error;

	for (swp = swdevt; swp->sw_dev; swp++) {
#ifdef NFSSWAP
		if (swaponnfs) {
			swp->sw_vp = (struct vnode *) config_nfsswap();
		} else {
#endif
			swp->sw_vp = bdevvp(swp->sw_dev);
#ifdef NFSSWAP
		}
#endif
		error = VOP_GETATTR(swp->sw_vp, &vattr, u.u_cred);
		if (error) {
			printf("swapconf: getattr failed %d\n", error);
		}
		nblks = vattr.va_size / DEV_BSIZE;
		if (swp->sw_nblks == 0 || swp->sw_nblks > nblks)
			swp->sw_nblks = nblks;
		totblks += swp->sw_nblks;
	}
	if (dumpsize == 0)
		dumpsize = physmem;
	if (dumplo == 0)
		dumplo = swdevt[0].sw_nblks - ctod(dumpsize);
	if (dumplo < 0)
		dumplo = 0;
	if (dmmin == 0)
		dmmin = DMMIN;
	if (dmmax == 0) {
		int i;

		for (i = 0; swptab[i].space; i++)
			if (totblks >= swptab[i].space)
				break;
		dmmax = swptab[i].dmmax;
	}
	if (dmtext == 0)
		dmtext = dmmax;
	if (dmtext > dmmax)
		dmtext = dmmax;
}

#define	PIMAJOR		25	/* parallel input keyboard and mouse */
#define	ZSMAJOR		12	/* serial */
#define	KBDMINOR 	0
#define	MOUSEMINOR	1
#define	BWONEMAJOR	26	/* Sun 1 black and white driver */
#define	BWTWOMAJOR	27	/* Sun 2 black and white driver */
#define	CGTWOMAJOR	31	/* Sun 2 color driver */
#define NOUNIT		-1

/*
 * Configure keyboard, mouse, and frame buffer using
 *	monitor provided values
 *	configuration flags
 * N.B. some console devices are statically initialized by the
 * drivers to NODEV.
 */
consconfig()
{
	char *cp;
	register struct mb_device *md;
	register struct mb_driver *mdr;
	int zsunit = NOUNIT;
	int bwunit = NOUNIT;
	int bwmajor;
	int fb_type = *romp->v_fbtype;
	struct cdevsw *dp;
	struct sgttyb sg;
	int ldisc;
	int usepi = isconfig("pi")
	    && (    (*romp->v_keybid & 0xF) == KB_VT100
		||  (*romp->v_keybid & 0xF) == KB_KLUNK);
	int e;
	int kbdtranslatable = TR_CANNOT;
	int kbdspeed = B9600;

	stopnmi();

	/*
	 * check for console on same ascii port to allow full speed
	 * output by using the UNIX driver and avoiding the monitor.
	 */
	if (*romp->v_insource == INUARTA && *romp->v_outsink == OUTUARTA)
		consdev = makedev(ZSMAJOR, 0);
	else if (*romp->v_insource == INUARTB && *romp->v_outsink == OUTUARTB)
		consdev = makedev(ZSMAJOR, 1);
	if (consdev) {
		kbddev = rconsdev = consdev;
		/*
		 * Opening causes interrupts, etc. to be initialized.
		 * Console device drivers must be able to do output
		 * after being closed!
		 */
		dp = &cdevsw[major(consdev)];
		if (e = (*dp->d_open)(consdev, FREAD+FWRITE))
			printf("console open failed: error %d\n", e);
		/* now we must close it to make console logins happy */
		(*dp->d_close)(consdev);
		/* undo undesired ttyopen side effects */
		u.u_ttyp = 0;
		u.u_ttyd = 0;
		u.u_procp->p_pgrp = 0;
		return;
	}

	if (*romp->v_insource == INUARTA)
		kbddev = makedev(ZSMAJOR, 0);
	if (*romp->v_insource == INUARTB)
		kbddev = makedev(ZSMAJOR, 1);
	if (kbddev != NODEV)
		kbdspeed = zsgetspeed(kbddev);

	/*
	 * Look for the [last] kbd/ms and matching fbtype.
	 * N.B.	We can not use fbaddr to discriminate (eg, the 110),
	 *	as it is hardwired into the proms, based on cpu type.
	 */
	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (!md->md_alive || md->md_slave != -1)
			continue;
		cp = mdr->mdr_dname;
		if (strncmp(cp, "zs", 2) == 0) {
			if (md->md_flags & ZS_KBDMS)
				zsunit = md->md_unit;
		}
		if (fb_type==FBTYPE_SUN1BW && strncmp(cp, "bwone", 5)==0)
			bwmajor = BWONEMAJOR, bwunit = md->md_unit;
		if (fb_type==FBTYPE_SUN2BW && strncmp(cp, "bwtwo", 5)==0)
			bwmajor = BWTWOMAJOR, bwunit = md->md_unit;
		if (fb_type==FBTYPE_SUN2COLOR && strncmp(cp, "cgtwo", 5)==0)
			bwmajor = CGTWOMAJOR, bwunit = md->md_unit;
	}
	/*
	 * Use serial keyboard and mouse if found flagged uart
	 */
	if (zsunit != NOUNIT) {
		if (mousedev == NODEV)
			mousedev = makedev(ZSMAJOR, 2*zsunit+MOUSEMINOR);
		if (kbddev == NODEV) {
			kbddev = makedev(ZSMAJOR, 2*zsunit+KBDMINOR);
			kbdtranslatable = TR_CAN;
		}
	} else {				/* Sun-1 Video */
		if (kbddev == NODEV && usepi) {
			kbddev = makedev(PIMAJOR, KBDMINOR);
			kbdtranslatable = TR_CAN;
		}
	}
	if (usepi && mousedev == NODEV)
		mousedev = makedev(PIMAJOR, MOUSEMINOR);
	if (kbddev == NODEV)
		panic("no keyboard found");
	/*
	 * We need to open the keyboard here so that aborts are
 	 * processed even if no one has the console open 
	 */
	dp = &cdevsw[major(kbddev)];
	if (e = (*dp->d_open)(kbddev, FREAD))
		printf("keyboard open failed: error %d\n", e);
	e = 0;			/* Flush both read and write */
	(*dp->d_ioctl) (kbddev, TIOCFLUSH, (caddr_t)&e, 0);
	ldisc = KBDLDISC;
	if (e = (*dp->d_ioctl) (kbddev, TIOCSETD, (caddr_t)&ldisc, 0))
		printf("consconfig: TIOCSETD error %d\n", e);
	/* undo undesired ttyopen side effects */
	u.u_ttyp = 0;
	u.u_ttyd = 0;
	u.u_procp->p_pgrp = 0;
	(*dp->d_ioctl) (kbddev, TIOCSPGRP, (caddr_t)&u.u_procp->p_pgrp, 0);
	(*dp->d_ioctl) (kbddev, KIOCTRANSABLE, (caddr_t)&kbdtranslatable, 0);
	if (kbdtranslatable == TR_CANNOT) {
		/*
		 * For benefit of serial port keyboard
		 */
		(*dp->d_ioctl) (kbddev, TIOCGETP, (caddr_t)&sg, 0);
		sg.sg_ispeed = sg.sg_ospeed = kbdspeed;
		if (e = (*dp->d_ioctl) (kbddev, TIOCSETP, (caddr_t)&sg, 0))
			printf("consconfig: TIOCSETP error %d\n", e);
	}
 	kbddevopen = 1;
	/*
	 * Setup default frame buffer.
	 */
	if (fbdev == NODEV) {
		if (bwunit != NOUNIT)
			fbdev = makedev(bwmajor, bwunit);
		else
			printf("No default frame buffer found\n");
	}
}

/*
 * See if a device is configured -- used to avoid references
 * that screw up object reconfiguration
 */
isconfig(name)
	char *name;
{
	register struct mb_device *md;
	register struct mb_driver *mdr;
	register int	i = strlen(name) + 1;	/* since strcmp not defined */

	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (!md->md_alive)
			continue;
		if (strncmp(mdr->mdr_dname, name, i) == 0)
			return (1);
	}
	return (0);
}

/*
 * Some things, like cputype, are contained in the idprom, but are
 * needed and obtained earlier; hence they are not set (again) here.
 */
idprom()
{
	register u_char *cp, val = 0;
	register int i;
	struct idprom id;

	getidprom((char *)&id);
	cp = (u_char *)&id;
	for (i = 0; i < 16; i++)
		val ^= *cp++;
	if (val != 0)
		printf("WARNING: ID prom checksum error\n");
	if (id.id_format == 1) {
		(void) localetheraddr((struct ether_addr *)id.id_ether,
		    (struct ether_addr *)NULL);
	} else
		printf("INVALID FORMAT CODE IN ID PROM\n");
}

/*
 * Set up delay value for DELAY() and CDELAY() macros based on
 * cpu type.  Delay is constant for all known Sun-2's.
 */
int cpudelay = 5;

/*
 * We set the cpu type and associated variables.  Should there get to
 * be too many variables, they should be collected together in a
 * structure and indexed by cpu type.
 */
setcputype()
{
	struct idprom id;

	cpu = -1;
	getidprom((char *)&id);
	if (id.id_format == 1) {
		switch (id.id_machine) {
		case CPU_SUN2_120:
		case CPU_SUN2_50:
			cpu = id.id_machine;
			break;
		default:
			printf("UNKNOWN MACHINE TYPE 0x%x IN ID PROM\n",
			    id.id_machine);
			break;
		}
	}
	else
		printf("INVALID FORMAT TYPE IN ID PROM\n");

	if (cpu == -1) {
		printf("DEFAULTING MACHINE TYPE TO SUN2_120\n");
		cpu = CPU_SUN2_120;
	}

	switch (cpu) {
	case CPU_SUN2_120:
#ifndef SUN2_120
		panic("not configured for SUN2_120");
#endif !SUN2_120
		dvmasize = btoc(SUN2_120_DVMASIZE);
		break;
	case CPU_SUN2_50:
#ifndef SUN2_50
		panic("not configured for SUN2_50");
#endif !SUN2_50
		/*
		 * Can't use the last segment for DVMA,
		 * it is reserved for on-board Ethernet.
		 */
		dvmasize = btoc(SUN2_50_DVMASIZE) - NPAGSEG;
		break;
	default:
		panic("unknown cpu type");
	}
}

machineid()
{
	struct idprom id;
	register int x;

	getidprom((char *)&id);
	x = id.id_machine << 24;
	x += id.id_serial;
	return (x);
}

/*
 * Initialize pseudo-devices
 * Reads count and init routine from table in ioconf.c
 * created by 'init blah' syntax on pseudo-device line in config file
 * Calls init routine once for each unit configured
 */
pseudoconfig()
{
	extern struct pseudo_init { 
		int 	ps_count;
		int	(*ps_func)();
	} pseudo_inits[]; 
	register struct pseudo_init *ps;
	int unit;

	for (ps = pseudo_inits; ps->ps_count > 0; ps++)
		for (unit = 0; unit < ps->ps_count; unit++)
			(*ps->ps_func)(unit);
}
