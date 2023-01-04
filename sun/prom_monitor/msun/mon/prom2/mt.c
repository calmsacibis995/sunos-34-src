/*
 * @(#)mt.c 1.7 84/02/07 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Mag tape boot routine for the Tapemaster controller.
 */

#include "../h/sunmon.h"
#include "../h/sasun.h"
#include "../h/bootparam.h"
#include "../h/msg.h"

#define MTBUF_PA	0x002000
#define MTBUF_VA	(DEF_MBMEM_VA+MTBUF_PA)
#define MTBUF_SIZE	0x005000	/* 20K max record size */

#define NSTD	2
int mtstd[NSTD] = { 0xA0, 0xA2 };

typedef unsigned short ushort;

struct addr86 {
	ushort a_offset;
	ushort a_base;
};

typedef struct addr86 ptr86_t;
typedef int bit;

/*
 * System Configuration Pointer
 * At a jumpered address (low nibble 6)
 */
struct scp {
	char	scb_busx, scb_bus;	/* 8/16 bit bus flag */
	ptr86_t	scb_ptr;		/* pointer to configuration block */
};

/* Definitions for scb_bus */
#define SYSBUS8	 0	/* 8 bit bus */
#define SYSBUS16 1	/* 16 bit bus */

/*
 * System Configuration Block
 * Statically located between controller resets
 */
struct scb {
	char	scb_03x, scb_03;	/* constant 0x03 */
	ptr86_t	ccb_ptr;		/* pointer to channel control block */
};
#define SCBCONS	0x03	/* random constant for SCB */

/*
 * Channel Control Block
 * Statically located between controller resets
 */
struct ccb {
	char ccb_gate, ccb_ccw;		/* interrupt control */
	ptr86_t	tpb_ptr;		/* pointer to tape parm block */
};
/* definitions for ccb_gate */
#define G_OPEN	 0x00		/* open - ctlr available */ 
#define G_CLOSED 0xFF		/* closed - ctlr active or alloc */
/* definitions for ccb_ccw */
#define C_NORMAL 0x11		/* normal command */
#define C_CLRINT 0x09		/* clear active interrupt */

/*
 * Tape Status Structure - one word
 */
struct tstat {
	bit	ts_entered:1;	/* tpb entered */
	bit	ts_compl : 1;	/* tpb complete */
	bit	ts_retry : 1;	/* tpb was retried */
	bit	ts_error : 5;	/* error code */
	/* byte */
	bit 	ts_eof   : 1;	/* filemark */
	bit	ts_online: 1;	/* on line */
	bit	ts_load  : 1;	/* at load point */
	bit	ts_eot   : 1;	/* end of tape */
	bit	ts_ready : 1;	/* ready */
	bit	ts_fbusy : 1;	/* fmt busy */
	bit	ts_prot  : 1;	/* wrt protected */
	bit		 : 1;	/* unused */
	/* byte */
};
/* Tape error codes (ts_error) */
#define E_NOERROR 0	/* normal completion */
#define E_BADTAPE 0x0A	/* bad spot on tape */
#define E_OVERRUN 0x0B	/* Bus over/under run */
#define E_PARITY  0x0D	/* Read parity error */
#define E_SHORTREC 0x0F	/* short record on read; error on write */
#define E_EOF	  0x15	/* end of file on read */

/*
 * Tape Parameter Block
 * Dynamically located via CCB
 */
struct tpb {
	short	tp_cmd;			/* command word(byte) */
	short	tp_cmd2;		/* zero command word */
	struct {
		bit	tc_width : 1;	/* bus width */
		bit              : 2;	/* unused */
		bit	tc_cont  : 1;	/* continuous movement */
		bit	tc_speed : 1;	/* slow or stream */
		bit	tc_rev   : 1;	/* reverse */
		bit		 : 1;	/* unused */
		bit	tc_bank  : 1;	/* bank select */
		/* byte */
		bit	tc_lock  : 1;	/* bus lock */
		bit	tc_link  : 1;	/* tpb link */
		bit	tc_intr  : 1;	/* want interrupt */
		bit	tc_mail  : 1;	/* mailbox intr */
		bit	tc_tape  : 2;	/* tape select */
		bit		 : 2;	/* unused */
		/* byte */
	}	tp_ctl;			/* control word */
	ushort	tp_count;		/* return count */
	ushort	tp_bsize;		/* buffer size */
	ushort	tp_rcount;		/* real size / overrun */
	ptr86_t tp_baddr;		/* buffer address */
	struct tstat tp_stat;		/* tape status */
	ptr86_t	tp_intrlink;		/* intr/link addr */
};

/*
 * Interesting tape commands (tp_cmd)
 */
#define CONFIG	0x00	/* Configure controller */
#define NOP	0x20	/* NOP - for clearing intrs */
#define STATUS	0x28	/* Drive Status */
#define REWIND	0x34	/* Rewind (non-overlapped) */
#define UNLOAD	0x38	/* Unload or go offline */
#define WEOF	0x40	/* Write file mark (EOF) */ 
#define SEARCH	0x44	/* search for file mark */
#define SPACE	0x48	/* move over tape record */
#define READ	0x2C	/* Read to MB memory */
#define WRITE	0x30	/* Write to MB memory */
#define ERASE	0x4C	/* Erase fixed length */

/*
 * Tape directions (tc_rev)
 */
#define FORWARD 0
#define REVERSE 1

#define NOTZERO	1	/* don't care value-but not zero (clr inst botch) */
#define IOATTN(ioctlr)	(*(ioctlr + 1) = NOTZERO)
#define IORESET(ioctlr)	(* ioctlr      = NOTZERO)

/* wired-in address for all controllers = 0x1106 */
#define MBCTLR	((struct mbctlr *)(DEF_MBMEM_VA+0x1100))

/*
 * Data which must be present for each controller
 * in Multibus memory.
 * The SCP is pointed to by hardware configuration;
 * we assume the rest is contiguous.
 * We put the SCB before the SCP so that the whole mess
 * is at an address with a low nibble of zero.
 * The SCP must have a low address nibble of 6;	
 * conveniently, the SCB size is 6.
 */
struct mbctlr {
	struct scb mb_scb;	/* System Conf Block */
	struct scp mb_scp;	/* System Conf Pointer */
	struct ccb mb_ccb;	/* Channel Control Block */
	struct tpb mb_tpb;	/* Tape Parameter Block */
};

/*
 * String used to print errors (occurs several places, so we out-smart
 * the compiler and make a variable out of it.  Foo.
 */
char MT_ERR_MSG[] = "mt: error 0x%x\n";

/*
 * Start an operation and wait for it to complete
 */
#define gowait(ioctlr)				\
{							\
	MBCTLR->mb_ccb.ccb_gate = G_CLOSED;		\
	IOATTN(ioctlr);					\
	while(MBCTLR->mb_ccb.ccb_gate != G_OPEN) ;	\
}

#define	clrtpb(tp)	bzero((char *)tp, sizeof(*tp))

/*
 * Boot from tape
 */
mtboot(bp)
	struct bootparam *bp;
{
	register int len;
	register char *addr;
	register int unit = bp->bp_unit & 3;
	register char *ioctlr;
	int ctlr;

	if ((ctlr = bp->bp_ctlr) < NSTD)
		ctlr = mtstd[ctlr];
	ioctlr = (char *)(DEF_MBIO_VA + ctlr);

	/* probe for controller by trying to reset it */
	if(pokec(ioctlr, NOTZERO)) {
		printf(msg_noctlr, ctlr);
		return (-1);
	}

	if(!ctlrinit(ioctlr)) {
		printf("mt: controller does not initialize\n");
		return (-1);
	}

	if (simple(ioctlr, unit, STATUS))
		return (-1);
	{
		register struct tpb *tp;

		tp = &MBCTLR->mb_tpb;
		if(!tp->tp_stat.ts_online || !tp->tp_stat.ts_ready) {
			printf("mt: unit not ready\n");
			return (-1);
		}
	}

	if (simple(ioctlr, unit, REWIND))
		return (-1);

	addr = (char *)LOADADDR;
	while((len = mtread(ioctlr, unit)) > 0) {
		register char *from = MTBUF_VA;
		register int n = (len+1)>>1;
		register char temp;
		
		while ((--n) >= 0) {
			temp = *from++;
			*addr++ = *from++;
			*addr++ = temp;
		}
	}
	return( (len < 0)? -1: LOADADDR);	/* If no error, return addr */
}

#define SPININIT  100000 	/* spin count for ctlr initialize */

/*
 * Initialize a controller
 * Reset it, set up SCP, SCB, and CCB,
 * and give it an attention.
 * Make sure its there by waiting for the gate to open
 * Once initialization is done, issue CONFIG just to be safe.
 */
static ctlrinit(ioctlr)
	register char *ioctlr;
{
	register struct mbctlr *mc = MBCTLR;
	register struct tpb *tp;
	register int spin;

	bzero((char *)mc, sizeof (*mc));	/* Clear block, set parity */

	IORESET(ioctlr);
	
	/* setup System Configuration Pointer */
	mc->mb_scp.scb_bus = mc->mb_scp.scb_busx = SYSBUS16;
	c68t86((int)&mc->mb_scb, &mc->mb_scp.scb_ptr);

	/* setup System Configuration Block */
	mc->mb_scb.scb_03 = mc->mb_scb.scb_03x = SCBCONS;
	c68t86((int)&mc->mb_ccb, &mc->mb_scb.ccb_ptr);

	/* setup Channel Control Block */
	mc->mb_ccb.ccb_gate = G_CLOSED;

	IOATTN(ioctlr); 
	for(spin = SPININIT; mc->mb_ccb.ccb_gate != G_OPEN; ) {
		if(--spin <= 0) {
			return(0);	/* Return failure */
		}
	}

	/* Finish CCB, point it at TPB */
	mc->mb_ccb.ccb_ccw = C_NORMAL;
	tp = &mc->mb_tpb;
	c68t86((int)tp, &mc->mb_ccb.tpb_ptr);

	/* Issue CONFIG command */
	clrtpb(tp);
	tp->tp_cmd = CONFIG;
	tp->tp_cmd2 = 0;
	tp->tp_ctl.tc_width = 1;

	/* Get the gate */
	while(mc->mb_ccb.ccb_gate != G_OPEN)
		;

	/* Start the command and wait for interrupt */
	gowait(ioctlr);
	return(1);
}

/*
 * Read from a tape.
 * FIXME: This can probably be merged into  simple() to save space.
 */
mtread(ioctlr, unit)
char *ioctlr;
{
	register struct tpb *tp = &MBCTLR->mb_tpb;

	clrtpb(tp);
	tp->tp_cmd = READ;
	tp->tp_ctl.tc_width = 1;
	tp->tp_ctl.tc_tape = unit;
	tp->tp_bsize = MTBUF_SIZE;
	c68t86(MTBUF_PA, &tp->tp_baddr);
	gowait(ioctlr);
	if(tp->tp_stat.ts_error == E_EOF)
		return(0);
	if(tp->tp_stat.ts_error != E_SHORTREC 
	   && tp->tp_stat.ts_error != E_NOERROR) {
		printf(MT_ERR_MSG, tp->tp_stat.ts_error);
		return(-1);
	}
	return(tp->tp_rcount);
}

/*
 * Do a simple tape command
 */
static simple(ioctlr, unit, command)
	char *ioctlr;
	int command;
{
	register struct mbctlr *mc = MBCTLR;
	register struct tpb *tp;

	tp = &mc->mb_tpb;
	clrtpb(tp);
	tp->tp_cmd = command;
	tp->tp_ctl.tc_width = 1;
	tp->tp_ctl.tc_tape = unit;
	gowait(ioctlr);
	switch(tp->tp_stat.ts_error) {
	case E_NOERROR:
	case E_EOF:
		return 0;
	}
	printf(MT_ERR_MSG, tp->tp_stat.ts_error);
	return 1;
}

/*
 * Convert a 68000 address into a 8086 address
 */
static c68t86(a68, a86)
int a68;
ptr86_t *a86;
{
	a86->a_offset = a68 & 0xFFFF;
	a86->a_base = (a68 & 0xF0000) >> 4;
}
