# include "sendmail.h"
# include <signal.h>

SCCSID(@(#)clock.c 1.1 86/09/25 SMI); /* from UCB 4.3 12/5/84 */

/*
**  SETEVENT -- set an event to happen at a specific time.
**
**	Events are stored in a sorted list for fast processing.
**	An event only applies to the process that set it.
**
**	Parameters:
**		intvl -- intvl until next event occurs.
**		func -- function to call on event.
**		arg -- argument to func on event.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

EVENT *
setevent(intvl, func, arg)
	time_t intvl;
	int (*func)();
	int arg;
{
	register EVENT **evp;
	register EVENT *ev;
	auto time_t now;
	extern tick();

# ifdef DEBUG
	if (intvl <= 0)
	{
		syserr("setevent: intvl=%ld\n", intvl);
		return (NULL);
	}
# endif DEBUG

	(void) time(&now);

	/* search event queue for correct position */
	for (evp = &EventQueue; (ev = *evp) != NULL; evp = &ev->ev_link)
	{
		if (ev->ev_time >= now + intvl)
			break;
	}

	/* insert new event */
	ev = (EVENT *) xalloc(sizeof *ev);
	ev->ev_time = now + intvl;
	ev->ev_func = func;
	ev->ev_arg = arg;
	ev->ev_pid = getpid();
	ev->ev_link = *evp;
	*evp = ev;

# ifdef DEBUG
	if (tTd(5, 5))
		printf("setevent: intvl=%ld, for=%ld, func=%x, arg=%d, ev=%x\n",
			intvl, now + intvl, func, arg, ev);
# endif DEBUG

	tick();
	return (ev);
}
/*
**  CLREVENT -- remove an event from the event queue.
**
**	Parameters:
**		ev -- pointer to event to remove.
**
**	Returns:
**		none.
**
**	Side Effects:
**		arranges for event ev to not happen.
*/

clrevent(ev)
	register EVENT *ev;
{
	register EVENT **evp;

# ifdef DEBUG
	if (tTd(5, 5))
		printf("clrevent: ev=%x\n", ev);
# endif DEBUG
	if (ev == NULL)
		return;

	/* find the parent event */
	(void) signal(SIGALRM, SIG_IGN);
	for (evp = &EventQueue; *evp != NULL; evp = &(*evp)->ev_link)
	{
		if (*evp == ev)
			break;
	}

	/* now remove it */
	if (*evp != NULL)
	{
		*evp = ev->ev_link;
		free((char *) ev);
	}

	/* restore clocks and pick up anything spare */
	tick();
}
/*
**  TICK -- take a clock tick
**
**	Called by the alarm clock.  This routine runs events as needed.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		calls the next function in EventQueue.
*/

tick()
{
	register time_t now;
	register EVENT *ev;
	int pid = getpid();

	(void) signal(SIGALRM, SIG_IGN);
	(void) alarm(0);
	now = curtime();

# ifdef DEBUG
	if (tTd(5, 4))
		printf("tick: now=%ld\n", now);
# endif DEBUG

	while ((ev = EventQueue) != NULL &&
	       (ev->ev_time <= now || ev->ev_pid != pid))
	{
		int (*f)();
		int arg;
		int pid;

		/* process the event on the top of the queue */
		ev = EventQueue;
		EventQueue = EventQueue->ev_link;
# ifdef DEBUG
		if (tTd(5, 6))
			printf("tick: ev=%x, func=%x, arg=%d, pid=%d\n", ev,
				ev->ev_func, ev->ev_arg, ev->ev_pid);
# endif DEBUG

		/* we must be careful in here because ev_func may not return */
		(void) signal(SIGALRM, tick);
 		/*
 		 * Patch for 4.2BSD to enable alarm signals now that
 		 * we have declared ourselves ready to accept another
 		 * signal.  We are still running in the signal handler,
 		 * so the system is nicely disabling the signal for us.
 		 */
 		(void) sigsetmask(sigblock(0) & ~(1<<(SIGALRM-1)));

		f = ev->ev_func;
		arg = ev->ev_arg;
		pid = ev->ev_pid;
		free((char *) ev);
		if (pid != getpid())
			continue;
		if (EventQueue != NULL)
		{
			if (EventQueue->ev_time > now)
				(void) alarm((unsigned) (EventQueue->ev_time - now));
			else
				(void) alarm(3);
		}
		(*f)(arg);
		(void) alarm(0);
		now = curtime();
	}
	(void) signal(SIGALRM, tick);
	if (EventQueue != NULL)
		(void) alarm((unsigned) (EventQueue->ev_time - now));
}
/*
**  SLEEP -- a version of sleep that works with this stuff
**
**	Because sleep uses the alarm facility, I must reimplement
**	it here.
**
**	Parameters:
**		intvl -- time to sleep.
**
**	Returns:
**		none.
**
**	Side Effects:
**		waits for intvl time.  However, other events can
**		be run during that interval.
*/

static bool	SleepDone;

sleep(intvl)
	int intvl;
{
	extern endsleep();

	if (intvl == 0)
		return;
	SleepDone = FALSE;
	(void) setevent((time_t) intvl, endsleep, 0);
	while (!SleepDone)
		pause();
}

static
endsleep()
{
	SleepDone = TRUE;
}
