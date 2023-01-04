/*	@(#)buserr.h 2.8 84/05/08 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * buserr.h
 *
 * definitions for stacked structure showing details of Bus Error
 * or Address Error on MC68010; also shows Sun-2 bus error register
 */

#ifndef VOR
/*
 * In the 68010, the BusErrInfo structure starts at the stack pointer upon
 * entry to the bus error routine and proceeds up the stack.  The info described
 * herein will automatically be popped off the stack by the RTE instruction.
 */
struct BusErrInfo {
	unsigned short	SR;
	unsigned long	PC;
	union {
		unsigned short VOR_u_all;
		struct {
			unsigned char	VOR_u_format:4;
			unsigned char		:2;
			unsigned short	VOR_u_offset:10;
		} VOR_u_parts;
	} VOR_u;
#define	VOR_format	VOR_u.VOR_u_parts.VOR_u_format
#define	VOR_offset	VOR_u.VOR_u_parts.VOR_u_offset
#define	VOR		VOR_u.VOR_u_all
	unsigned char	SSWB_RR	:1;	/* Rerun? (0=yes, 1=done by s/ware) */
	unsigned char		:1;
	unsigned char	SSWB_IF	:1;	/* Access was ifetch to IRC. */
	unsigned char	SSWB_DF	:1;	/* Access was data fetch to DIB. */
	unsigned char	SSWB_RM	:1;	/* Access was read-modify-write */
	unsigned char	SSWB_HB	:1;	/* High byte=1, low byte=0, iff BY */
	unsigned char	SSWB_BY	:1;	/* Byte.  1=byte, 0=word xfer. */
	unsigned char	SSWB_RW	:1;	/* Read/Write, 0=Write 1=Read. */
	unsigned char		:5;
	unsigned char	SSWB_FC	:3;	/* Function codes from access. */
	unsigned long	AOB;		/* Address out buffer: failing addr */
	unsigned short	RegE;
	unsigned short	DOB;		/* Data output buffer */
	unsigned short	Reg12;
	unsigned short	DIB;		/* Data input buffer */
	unsigned short	Reg16;
	unsigned short	IRC;		/* Instruction register */
	unsigned short	UPC;		/* Micro program counter */
	unsigned short	Reg1C;
	unsigned short	Reg1E;
	unsigned short	Reg20;
	unsigned short	Reg22;
	unsigned short	Reg24;
	unsigned short	Reg26;
	unsigned short	Reg28;
	unsigned short	Reg2A;
	unsigned short	Reg2C;
	unsigned short	Reg2E;
	unsigned short	Reg30;
	unsigned short	Reg32;
	unsigned short	Reg34;
	unsigned short	Reg36;
	unsigned short	Reg38;
};



/*
 * The System Bus Error Register captures the state of the system as of the
 * last bus error.  (If multiple bus errors occur, only the first one is 
 * kept.  Software indicates that it has read out that bus error by writing
 * to the bus error reg; the data doesn't matter and isn't saved.)  Enough
 * information is provided to uniquely determine the cause
 * of the bus error.
 *
 * If be_proterr is set and be_valid == 1, the protection field of the
 * page map entry caused the bus error.  If be_proterr is set and be_valid
 * == 0, the (in)validity bit of the page map entry caused the bus error.
 *
 * The rest of the bits are 1 if they caused the error.
 */
struct buserrorreg {
	unsigned			:8;
	unsigned	be_valid	:1;	/* Page map Valid bit is on */
	unsigned	be_vmebuserr	:1;	/* Bus error signaled on VME */
	unsigned			:2;	/* Reserved */
	unsigned	be_proterr	:1;	/* Protection error */
	unsigned	be_timeout	:1;	/* Bus access timed out */
	unsigned	be_parerr_u	:1;	/* Parity error, upper byte */
	unsigned	be_parerr_l	:1;	/* Parity error, lower byte */
};

#define	BUSERR_BITS "\20\10VALID\7VMEBUSERR\4PROTERR\3TIMEOUT\2UPARERR\1LPARERR"
#endif VOR
