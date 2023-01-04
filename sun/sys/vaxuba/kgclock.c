/*	kgclock.c	6.1	83/07/29	*/

#include "kg.h"
#if NKG > 0
/*
 * KL-11 as profiling clock
 */
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../vaxuba/ubavar.h"

int	kgprobe(), kgattach();
struct	uba_device *kginfo[1];
u_short	kgstd[] = { 0177560, 0 };
struct	uba_driver kgdriver =
    { kgprobe, 0, kgattach, 0, kgstd, "kg", kginfo };

struct klregs {
	u_short	fill[2];
	u_short	tcsr;
	u_short	tbuf;
};
#define	KLSTRT	0300		/* intr enbl + done */
struct	klregs *klbase;

kgprobe(reg)
	caddr_t reg;
{
	register int br, cvec;	/* value-result */
	register struct klregs *klp = (struct klregs *)reg;

	klp->tcsr = KLSTRT;
	DELAY(100000);
	klp->tcsr = 0;
}

kgattach(ui)
	struct uba_device *ui;
{

	klbase = (struct klregs *)ui->ui_addr;
}

/*
 * start the sampling clock
 */
startkgclock()
{

	if (klbase)
		klbase->tcsr = KLSTRT;	/* enable interrupts */
}

/* ARGSUSED */
kgclock(dev, r0, r1, r2, r3, r4 ,r5, pc, ps)
	caddr_t pc;
	int ps;
{
	register int k;
	static long otime;
	static long calibrate;

	klbase->tbuf = 0377;	/* reprime clock (scope sync too) */
	if (phz == 0) {
		if (otime == 0) {
			otime = time.tv_sec + 1;
			calibrate = 0;
		}
		if (time.tv_sec >= otime)
			calibrate++;
		if (time.tv_sec >= otime + 4) {
			phz = calibrate / 4;
			otime = 0;
		}
		return;
	}
	gatherstats(pc, ps);	/* this routine lives in kern_clock.c */
}
#endif
