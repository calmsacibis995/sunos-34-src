#ifndef lint
static	char sccsid[] = "@(#)swapgeneric.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/systm.h"
#include "../h/reboot.h"
#include "../h/file.h"

#include "../machine/pte.h"
#include "../sundev/mbvar.h"
#include "../mon/sunromvec.h"

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev, argdev, dumpdev;
struct vnode *argdev_vp;
struct vnode *rootvp;
struct	swdevt swdevt[] = {
	{ -1,	0,	0 },
	{ 0,	0,	0 },
};

#include "ip.h"
#if NIP > 0
extern	struct mb_driver ipcdriver;
#endif

#include "xy.h"
#if NXY > 0
extern	struct mb_driver xycdriver;
#endif

#include "sc.h"
#if NSC > 0
extern	struct mb_driver scdriver;
#endif

#include "si.h"
#if NSI > 0
extern	struct mb_driver sidriver;
#endif

struct	genericconf {
	char	*gc_name;
	struct mb_driver *gc_driver;
	dev_t	gc_root;
} genericconf[] = {
#if NXY > 0
	"xy", &xycdriver, makedev(3, 0),
#endif
#if NIP > 0
	"ip", &ipcdriver, makedev(0, 0),
#endif
#if NSC > 0
	"sd", &scdriver, makedev(7, 0),
#endif
#if NSI > 0
	"sd", &sidriver, makedev(7, 0),
#endif
#include "nd.h"
#if NND > 0
	"nd", 0, makedev(5, 0),
#endif
#ifdef NFSROOT
	"nf", 0, makedev(99,99),
#endif
   0,
};

setconf()
{
	register struct genericconf *gc;
	int unit, swaponroot = 0;

	if (boothowto & RB_ASKNAME) {
		char name[128];
retry:
		printf("root device? ");
		gets(name);
		for (gc = genericconf; gc->gc_name; gc++)
			if (gc->gc_name[0] == name[0] &&
			    gc->gc_name[1] == name[1])
				goto gotit;
		goto bad;
gotit:
		if (name[3] == '*') {
			name[3] = name[4];
			swaponroot++;
		}
		if (name[2] >= '0' && name[2] <= '7' && name[3] == 0) {
			unit = name[2] - '0';
			goto found;
		}
		printf("bad/missing unit number\n");
bad:
		printf("use one of: ");
		for (gc = genericconf; gc->gc_name; gc++)
			printf("%s%%d ", gc->gc_name);
		printf("\n");
		goto retry;
	}
	unit = 0;
	for (gc = genericconf; gc->gc_name; gc++) {
		register struct bootparam *bp = (*romp->v_bootparam);

		if (!((*gc->gc_name == bp->bp_dev[0]) &&
			(*(gc->gc_name+1) == bp->bp_dev[1])))
			continue;
		if (chkroot(gc)) {
			printf("root on %s0\n", gc->gc_name);
			goto found;
		} 
	} 

	for (gc = genericconf; gc->gc_name; gc++) {
		if (gc->gc_driver == 0) {
			/* nd is last entry in table, just try it */
			printf("root on %s0\n", gc->gc_name);
			goto found;
		}
		if (chkroot(gc)) {
			printf("root on %s0\n", gc->gc_name);
			goto found;
		} 
	}
	printf("no suitable root\n");
	boot(0, RB_NOSYNC|RB_HALT);
	/* NOTREACHED */

found:
	gc->gc_root = makedev(major(gc->gc_root), unit*8);
	rootdev = gc->gc_root;
	swdevt[0].sw_dev = argdev = dumpdev =
	    makedev(major(rootdev), minor(rootdev)+1);
	/* swap size, dumplo, and dumpsize set during autoconfigure */
	if (swaponroot)
		rootdev = dumpdev;
}

chkroot(gc)
	register struct genericconf *gc;
{       
	register struct mb_device *md;
	dev_t try;
	
	for (md = mbdinit; md->md_driver; md++) {
		if (md->md_alive == 0)
			continue;
		if (md->md_unit == 0 && md->md_driver == gc->gc_driver) {
			try = makedev(major(gc->gc_root), 0);
			if ((*bdevsw[major(try)].d_open)(try, FREAD) == 0) {
				(void)(*bdevsw[major(try)].d_close)(try, FREAD);
				return (1);
			}
		}
	}
	return (0);
} 

getchar()
{
	register c;

	c = cngetc();
	if (c=='\r')
		c = '\n';
	cnputc(c);
	return (c);
}

gets(cp)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	for (;;) {
		c = getchar() & 0177;
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case 0177:
			cnputc('\b');
		case '\b':
		case '#':
			lp--;
			if (lp < cp)
				lp = cp;
			continue;
		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}
