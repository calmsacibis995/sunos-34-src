/*	@(#)buserr.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Definitions and structures related to 68020 bus error handling for Sun-3.
 */

#ifndef _BUSERR_
#define	_BUSERR_
#ifndef LOCORE
/*
 * Auxiliary information stacked by hardware on Bus Error or Address Error.
 */

/*
 * 68010/68020 "type 0" normal four word stack.
 */
#define	SF_NORMAL	0x0

/*
 * 68020 "type 1" throwaway four word stack.
 */
#define	SF_THROWAWAY	0x1

/*
 * 68020 "type 2" normal six word stack frame.
 */
#define	SF_NORMAL6	0x2
struct	bei_normal6 {
	int	bei_pc;		/* pc where error occurred */
};

/*
 * 68020 "type 9" co-processor mid-instruction exception stack frame.
 */
#define	SF_COPROC	0x9
struct	bei_coproc {
	int	bei_pc;		/* pc where error occured */
	int	bei_internal;	/* an internal register */
	int	bei_eea;	/* evaluated effective address */
};

/*
 * 68020 "type A" medium bus cycle fault stack frame.
 */
#define	SF_MEDIUM	0xA
struct	bei_medium {
	short	bei_internal;	/* an internal register */
	u_int	bei_faultc: 1;	/* fault on stage c of instruction pipe */
	u_int	bei_faultb: 1;	/* fault on stage b of instruction pipe */
	u_int	bei_rerunc: 1;	/* rerun stage c */
	u_int	bei_rerunb: 1;	/* rerun stage b */
	u_int		  : 3;
	u_int	bei_dfault: 1;	/* fault/rerun on data */
	u_int	bei_rmw	  : 1;	/* read/modify/write */
	u_int	bei_rw	  : 1;	/* read=1,write=0 */
	u_int	bei_size  : 2;	/* size code for data cycle */
	u_int		  : 1;
	u_int	bei_fcode : 3;	/* function code */
	short	bei_ipsc;	/* instruction pipe stage c */
	short	bei_ipsb;	/* instruction pipe stage b */
	int	bei_fault;	/* fault address */
	int	bei_internal2;	/* another internal register */
	int	bei_dob;	/* data output buffer */
	int	bei_internal3;	/* yet another internal register */
};

/*
 * 68020 "type B" long bus cycle fault stack frame.
 */
#define	SF_LONGB	0xB
struct	bei_longb {
	short	bei_internal;	/* an internal register */
	u_int	bei_faultc: 1;	/* fault on stage c of instruction pipe */
	u_int	bei_faultb: 1;	/* fault on stage b of instruction pipe */
	u_int	bei_rerunc: 1;	/* rerun stage c */
	u_int	bei_rerunb: 1;	/* rerun stage b */
	u_int		  : 3;
	u_int	bei_dfault: 1;	/* fault/rerun on data */
	u_int	bei_rmw	  : 1;	/* read/modify/write */
	u_int	bei_rw	  : 1;	/* read=1,write=0 */
	u_int	bei_size  : 2;	/* size code for data cycle */
	u_int		  : 1;
	u_int	bei_fcode : 3;	/* function code */
	short	bei_ipsc;	/* instruction pipe stage c */
	short	bei_ipsb;	/* instruction pipe stage b */
	int	bei_fault;	/* fault address */
	int	bei_internal2;	/* another internal register */
	int	bei_dob;	/* data output buffer */
	int	bei_internal3;	/* yet another internal register */
	int	bei_internal4;	/* yet another internal register */
	int	bei_stageb;	/* stage B address */
	int	bei_internal5;	/* yet another internal register */
	int	bei_dib;	/* data input buffer */
	int	bei_internal6[11];/* more internal registers */
};
#endif !LOCORE

/*
 * The System Bus Error Register captures the state of the
 * system as of the last bus error.  The Bus Error Register
 * always latches the cause of the most recent bus error.
 * Thus, in the case of stacked bus errors, the information
 * relating to the earlier bus errors is lost.  The Bus Error
 * register is read-only and is addressed in FC_MAP space.
 * The Bus Error Register is clocked for bus errors for CPU Cycles
 * line.  The Bus Error Register is not clocked for DVMA cycles
 * or for CPU accesses to the floating point chip with the
 * chip not enabled.
 *
 * The be_proterr bit is set only if the valid bit was set AND the
 * page protection does not allow the kind of operation attempted.
 * The rest of the bits are 1 if they caused the error.
 */
#ifndef LOCORE
struct	buserrorreg {
	u_int	be_invalid	:1;	/* Page map Valid bit was off */
	u_int	be_proterr	:1;	/* Protection error */
	u_int	be_timeout	:1;	/* Bus access timed out */
	u_int	be_vmeberr	:1;	/* VMEbus bus error */
	u_int	be_fpaberr	:1;	/* FPA bus error */
	u_int	be_fpaena	:1;	/* FPA enable error */
	u_int			:1;
	u_int	be_watchdog	:1;	/* Watchdog reset */
};
#endif !LOCORE

/*
 * Equivalent bits.
 */
#define	BE_INVALID	0x80		/* page map was invalid */
#define	BE_PROTERR	0x40		/* protection error */
#define	BE_TIMEOUT	0x20		/* bus access timed out */
#define	BE_VMEBERR	0x10		/* VMEbus bus error */
#define	BE_FPABERR	0x08		/* FPA bus error */
#define	BE_FPAENA	0x04		/* FPA enable error */
#define	BE_WATCHDOG	0x01		/* Watchdog reset */

#define	BUSERRREG	0x60000000	/* addr of buserr reg in FC_MAP space */

#define BUSERR_BITS	"\20\10INVALID\7PROTERR\6TIMEOUT\5VMEBERR\4FPABERR\3FPAENA\1WATCHDOG"
#endif !_BUSERR_
