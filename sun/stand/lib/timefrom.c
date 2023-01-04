#include <sys/types.h>
#include <machdep.h>

#define CLOCKBASE	((struct tod *)(0xee1000))
#define	BCDTOBIN(x)	(((u_long)(x) & 0xf) + 10*(((u_long)(x))>>4))

struct tod {
	struct {		/* counters */
		u_char	val;
		u_char	: 8;
	} tod_counter[8];
	struct {		/* compare latches - not used */
		u_char	val;
		u_char	: 8;
	} tod_latch[8];
	u_char	tod_isr;	/* interrupt status - not used */
	u_char	: 8;
	u_char	tod_icr;	/* interrupt control - not used */
	u_char	: 8;
	u_char	tod_creset;	/* counter reset mask */
	u_char	: 8;
	u_char	tod_lreset;	/* latch reset mask */
	u_char	: 8;
	u_char	tod_status;	/* bad counter read status */
	u_char	: 8;
	u_char	tod_go;		/* GO - start at integral seconds */
	u_char	: 8;
	u_char	tod_stby;	/* standby mode - not used */
	u_char	: 8;
	u_char	tod_test;	/* test mode - ??? */
	u_char	: 8;
};


#define HR		BCDTOBIN(saves[4])
#define MIN		BCDTOBIN(saves[3])
#define SEC		BCDTOBIN(saves[2])
#define CENTI		BCDTOBIN(saves[1])
#define MILLI		BCDTOBIN(saves[0])

static char	sccsid[] = "@(#)timefrom.c 1.1 9/25/86 Copyright Sun Micro";
/*
 *	this routine reads the RTC, checks for a clock rollover on a read,
 *	then contructs the time in milliseconds, and subtracts it
 *	from the time given as an argument
 */

timefrom(start)
register long			start;
{
	register long		now;
	register struct tod	*tp = CLOCKBASE;
	u_char			saves[5];
	register u_char		*sp;

oops:	sp = saves;				/* set up save pointer */
/*
 *	now get the values.  if the clock rolls over on a read, start over
 */
	for (now = 0; now < 5; now++){
		*sp++ = tp->tod_counter[now].val; 
		if(tp->tod_status)
			goto oops;
	}

	now = ((((HR*60) + MIN)*60 + SEC)*100 + CENTI)*10 + MILLI/10;

	return(now - start);
}

/*
 *	this just reads the clock until we get a rollover so
 *	we can synchronize with the RTC
 */

clocksync()
{
	register struct tod	*tp = CLOCKBASE;
	register 		i = 0, c;

	while(! tp->tod_status){
		c = tp->tod_counter[0].val;
		++i;
	}
	return(i);
}
