/*
 * @(#)usecmd.c 2.13 84/10/15 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * "U" monitor command
 *
 * Reports on, or changes, or changes baud rates of, I/O devices.
 * Syntax: [abc] means exactly one of a, b, or c.
 * 	   [ab~] means zero or one of a and b.
 *
 * u		report current state of world
 * u[ab]	select uart a or b as input and output device
 * u[ab]io	(same)
 * u[ab][io]	select uart a or b as input or output device
 * uk[i~]	select keyboard as input device (i ignored)
 * us[o~]	select screen as output device (o ignored)
 * usk		select screen and keyboard
 * uks		(same)
 * u[ab~]9600	select speed of uart, defaults to current input dev
 * 		supports the usual speeds.
 * ue		echo input to output
 * une		don't echo input to output
 * u[ab~]r	Reset uart to default state, a la powerup.
 * uu <addr>	Use uart at virtual address <addr>
 *        If TRANSPAR is supported (it currently isn't):
 * u[ab~]t	enter transparent mode to uart; defaults to b.
 * ux<char>	use escape char from transparent mode (not w/INKEYB)
 * uf		use flow control in transparent mode
 * unf		don't.
 */

#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/s2addrs.h"
#include "../h/s2misc.h"
#include "../h/zsreg.h"

unsigned char peekchar(), getone();

#define SELDEFAULT	0xFF

char inchars[] = "kab";			/* InSource chars */
char outchars[]= "sab";			/* OutSink chars */
/* ***** End of order-dependent tables */

/* Generate a baud rate clock count from the baud rate */
struct sptab {int speeds, counts};
#define B(speed)	ZSTIMECONST(ZSCC_PCLK, speed)

struct sptab speedtab[] = {
	0x76800, B(76800),
	0x38400, B(38400),
	0x19200, B(19200),
	0x9600,  B(9600),
	0x4800,  B(4800),
	0x2400,  B(2400),
	0x1200,  B(1200),
	0x600,   B(600),
	0x300,   B(300),
	0x150,   B(150),
	0x110,   B(110),
	0,       0 		};

/*
 *	UART initialization sequence.  This is written to both 
 *	halves of the UART in a little loop.
 */
unsigned char	uart_init[] = {
	/* Set up all the elements on the chip: */
	0,	0,			/* Be sure we're at WR0 */
	9,	ZSWR9_RESET_WORLD,	/* Reset the world first */
#define	uart_init_skip	4
	0,	0,			/* Be sure we're at WR0 */
	4,	ZSWR4_PARITY_EVEN|	/* Async mode, etc, etc, etc */
		ZSWR4_1_STOP|
		ZSWR4_X16_CLK,
	3,	ZSWR3_RX_8,		/* 8-bit chars, no auto CD/CTS shake */
	5,	ZSWR5_RTS|		/* Set RTS/DTR, xmit 8-bit chars */
		ZSWR5_TX_8|
		ZSWR5_DTR,
	9,	ZSWR9_NO_VECTOR,	/* Don't try to respond to intacks */
	11,	ZSWR11_TRXC_XMIT|	/* Output xmitter clock */
/*		ZSWR11_TRXC_OUT_ENA|    ... but don't put it on the cable */
/* This is temporary circumvention since unused lines on the RS232 ports
   are not yet terminated.  Running 9600 baud clock here causes crosstalk. */
/* FIXME */
		ZSWR11_TXCLK_BAUD|
		ZSWR11_RXCLK_BAUD,
	12,	ZSTIMECONST(ZSCC_PCLK, 9600),	/* Default baud rate */
	13,	(ZSTIMECONST(ZSCC_PCLK, 9600))/256,	/* Ditto, high order */
	14,	ZSWR14_BAUD_FROM_PCLK,	/* Baud Rate Gen source = CPU clock */
#ifdef FIXME
/* FIXME!!!!!  Compiler bug!
	2,	(EVEC_LEVEL6 - (long *)0), /* Int vector = level 6 autovec */
#endif FIXME
	/* Now enable the various set-up elements: */
	3,	ZSWR3_RX_8|		/* Enable receiver */
		ZSWR3_RX_ENABLE,
	5,	ZSWR5_RTS|		/* Enable transmitter */
		ZSWR5_TX_ENABLE|
		ZSWR5_TX_8|
		ZSWR5_DTR,
	14,	ZSWR14_BAUD_ENA|	/* Enable baud rate generator */
		ZSWR14_BAUD_FROM_PCLK,
	/* Now clear out assorted garbage */
	0,	ZSWR0_RESET_STATUS,	/* Reset status latches */
	0,	ZSWR0_RESET_STATUS,	/* ...twice like the manual sez */
};


/*
 * Reset a ZSCC chip or channel.
 * The arguments are the chip "control" port address, and a flag:
 * 0 to reset just that channel, 1 to reset the whole chip (hardware
 * reset).
 */
void
reset_uart(addr, wholechip)
	register unsigned char *addr;
	char wholechip;
{
	register unsigned char *p = uart_init;

	if (!wholechip) p += uart_init_skip;

	for (; p < &uart_init[sizeof(uart_init)] ;) {
		(*(long *)0) += 100;	/* Waste time */
		*addr = *p++;
		(*(long *)0) -= 100;	/* Waste time */
	}
}


/*
 * Interpret a command from the user.
 */
usecmd()
{
	register char c;
	register struct sptab *sp;
	register int speed;
	register short selector = SELDEFAULT;
	register char inputok=0, outputok=0;	

another_sel:
	c = getone();
	switch (c & UPCASE) {

	case 'A':
		selector = INUARTA;
		inputok = outputok = 1;
		goto another_sel;

	case 'B':
		selector = INUARTB;
		inputok = outputok = 1;
		goto another_sel;

	case 'K':
		selector = INKEYB; 
		inputok = 1;
		goto another_sel;
		/* The effect here is that "sk" or "ks" will leave selector at
		   INKEYB (which = OUTSCREEN) but will also set outputok. */

	case 'S':
		if (!gp->g_fbthere) goto invalid;
		selector = OUTSCREEN;
		outputok = 1;
		goto another_sel;
		/* The effect here is that "sk" or "ks" will leave selector at
		   INKEYB (which = OUTSCREEN) but will also set outputok. */
	}

	/*
	 * Now decode the function to perform.  Possible functions
	 * include printing, redirection, baudsetting, etc.
	 */

	switch (c & UPCASE) {

	case '\0':	
		if (selector != SELDEFAULT) {
			/* Set input and/or output to A, B, K, or S */
			if (inputok)
				gp->g_insource = selector;
			if (outputok)
				gp->g_outsink = selector;
			break;
		}

		/* Report current state of world */
		printf ("u%ci, u%co, ua%x, ub%x, uu%x, u", 
			  inchars[gp->g_insource],
				outchars[gp->g_outsink],
				       findspeed(INUARTA),
					     findspeed(INUARTB),
						   gp->g_inzscc);
		if (gp->g_echo == 0) putchar ('n');
#ifdef TRANSPAR
		printf ("e, u");
		if (gp->g_transpstate == 0) putchar ('n');
		printf ("f, ux");
		if (gp->g_insource == INKEYB) 
			printf("{Setup+E}");
		else {
			c = gp->g_transpend;
			if (c < ' ') {putchar ('^'); c+=64;}
			putchar (c);
		}
		putchar ('\n');
#else  TRANSPAR
		printf ("e\n");
#endif TRANSPAR
		break;

	case 'I':
		if (!inputok) goto invalid;
		gp->g_insource = selector;
		if ('O' == (UPCASE & peekchar() ) ) goto another_sel;
		break;

	case 'O':
		if (!outputok) goto invalid;
		gp->g_outsink = selector;
		if ('I' == (UPCASE & peekchar() ) ) goto another_sel;
		break;

#ifdef TRANSPAR
	case 'T':
		if (selector != INUARTA) selector = INUARTB;
		transparent (selector);
		putchar('\n');
		break;
#endif TRANSPAR

	case 'N':
		if (selector != SELDEFAULT)
			goto invalid;
		else {
			c = UPCASE & getone();
			if ('E' == c) gp->g_echo = 0;
#ifdef TRANSPAR
		   else if ('F' == c) gp->g_transpstate = 0;
#endif TRANSPAR
		   else goto invalid;
		}
		break;

	case 'E':
		if (selector == SELDEFAULT)	gp->g_echo = 1;
		else				goto invalid;
		break;

#ifdef TRANSPAR
	case 'F':
		if (selector == SELDEFAULT)	gp->g_transpstate = -1;
		else				goto invalid;
		break;

	case 'X':
		gp->g_transpend = getone();
		break;
#endif TRANSPAR

	case 'U':
		/* Reset uart address */
		speed = getnum();
		if (speed != 0) {
			gp->g_inzscc  = (struct zscc_device *)speed;
			gp->g_outzscc = (struct zscc_device *)speed;
		}
		break;

	case 'R':
		/* Reset uart channel specified */
		if (selector == SELDEFAULT) selector = gp->g_insource;
		reset_uart(&gp->g_inzscc[2-selector].zscc_control, 0);
		break;

	default:
		if ( (c>='0') && (c<='9') ) {
			if (selector == SELDEFAULT)
				selector = gp->g_insource;
			if (selector < INUARTA)
				goto invalid;
			gp->g_lineptr--;	/* Push the char back */
			speed = getnum();	/* we know it'll work */
			for (sp = speedtab; ; sp++) {
				if (sp->speeds == speed) break;
				if (sp->speeds == 0) goto invalid;
			}
			/* Reload time constants for BRG in Uart chip */
			/* Manual advises disabling BRG while doing this
			   to avoid burps.  Try it this way -- they say
			   it will work OK but might reload with half the
			   old and half the new time constant. */
			gp->g_inzscc[2-selector].zscc_control = 12;
			gp->g_inzscc[2-selector].zscc_control = sp->counts;
			gp->g_inzscc[2-selector].zscc_control = 13;
			gp->g_inzscc[2-selector].zscc_control = sp->counts>>8;
			break;
		}
invalid:
		printf ("Invalid selection\n");
		return;
	}

	if ('\0' != getone()) printf ("Extra chars in command\n");
}


/*
 * Find and print the speed setting of Uart <selector>, which
 * is one of INUARTA, INUARTB.
 *
 * If the speed is not in the table, return the counter value,
 * which will be more useful than nothing (maybe).
 */

int
findspeed(selector)
	int selector;
{
	register int count;
	register struct sptab *cp;

	        gp->g_inzscc[2-selector].zscc_control = 12;
	count = gp->g_inzscc[2-selector].zscc_control;
	        gp->g_inzscc[2-selector].zscc_control = 13;
	count+=(gp->g_inzscc[2-selector].zscc_control) << 8;

	for (cp = speedtab; ; cp++) {
		if (cp->counts == count) return cp->speeds;
		if (cp->speeds == 0) return count;
	}
}
