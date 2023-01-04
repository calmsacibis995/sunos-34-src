#ifndef lint
static	char sccsid[] = "@(#)kern_softint.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "../h/types.h"

/*
 * Handle software interrupts through 'softcall' mechanism
 */

typedef int (*func_t)();

#define NSOFTCALLS	50

struct softcall {
	func_t	sc_func;		/* function to call */
	caddr_t	sc_arg;			/* arg to pass to func */
	struct softcall *sc_next;	/* next in list */
} softcalls[NSOFTCALLS];

struct softcall *softhead, *softtail, *softfree;

/*
 * Call function func with argument arg
 * at some later time at software interrupt priority
 */
softcall(func, arg)
	register func_t func;
	register caddr_t arg;
{
	register struct softcall *sc;
	static int first = 1;
	int s;

	s = spl6();
	if (first) {
		for (sc = softcalls; sc < &softcalls[NSOFTCALLS]; sc++) {
			sc->sc_next = softfree;
			softfree = sc;
		}
		first = 0;
	}
	/* coalesce identical softcalls */
	for (sc = softhead; sc != 0; sc = sc->sc_next)
		if (sc->sc_func == func && sc->sc_arg == arg)
			goto out;
	if ((sc = softfree) == 0)
		panic("too many softcalls");
	softfree = sc->sc_next;
	sc->sc_func = func;
	sc->sc_arg = arg;
	sc->sc_next = 0;
	if (softhead) {
		softtail->sc_next = sc;
		softtail = sc;
	} else {
		softhead = softtail = sc;
		siron();
	}
out:
	(void) splx(s);
}

/*
 * Called to process software interrupts
 * take one off queue, call it, repeat
 * Note queue may change during call
 */
softint()
{
	register func_t func;
	register caddr_t arg;
	register struct softcall *sc;
	int s;

	for (;;) {
		s = spl6();
		if (sc = softhead) {
			func = sc->sc_func;
			arg = sc->sc_arg;
			softhead = sc->sc_next;
			sc->sc_next = softfree;
			softfree = sc;
		}
		(void) splx(s);
		if (sc == 0)
			return;
		(*func)(arg);
	}
}
