/*	@(#)machdep.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * @(#)machdep.h 1.6 06/15/84 Copyright Sun Micro -- blame kevin@sun
 */
/*
 *	various handy constants for mapping and self-abuse 
 */
#define ADDRSIZE	(0x1000000)	/* 24-bit addresses */
#define ADDRMASK	(ADDRSIZE - 1)
#define SEGSIZE		(0x8000)	/* 16 pages per pmeg */
#define SEGMASK		(SEGSIZE - 1)
#define SEGSHIFT	(15)
#define PAGESIZE	(0x800)		/* 2k page */
#define PAGEMASK	(PAGESIZE -1)
#define PAGESHIFT	(11)
/*
 *	handy macros
 */
#define BTOP(x)		((u_long)(x)>>PAGESHIFT)	/* byte to page */
#define PTOB(x)		((u_long)(x)<<PAGESHIFT)	/* page to byte */
#define GETPHYS(x)	((PTOB(getpgreg(x) & 0xfff)& ADDRMASK)+ \
				((u_long)(x)&PAGEMASK))
#define ISVALID(x)	((getpgreg(x) & (u_long)0x80000000) == 0x80000000)
/*
 *	for parity(gen,check)  mnemonics
 */
#define PAR_GEN		(1)	/* like in parity(PAR_GEN or ! PAR_GEN */
#define PAR_CHECK	(1)	/* ditto */
/*
 *	offsets into fc3 accesses
 */
#define PG_OFF		(0)	/* page map offset in fc3 space */
#define SM_OFF		(4)	/* segment map */
#define CX_OFF		(6)	/* context registers */
#define ID_OFF		(8)	/* id prom */
#define LED_OFF		(10)	/* led (diagnostic register) */
#define BERR_OFF	(12)	/* buserror register */
#define ENABLE_OFF	(14)	/* system enable register */
/*
 *	assembly setups (for internal use)
 *		users should use get/set/fc3()
 */
#define	MOVC(from, to)	asm("	movc	from, to")
#define	MOVL(from, to)	asm("	movl	from, to")
#define MOVSB(from, to)	asm("	movsb	from, to")
#define MOVSW(from, to)	asm("	movsw	from, to")
#define MOVSL(from, to)	asm("	movsl	from, to")
/*
 *	permissions available
 */
#define PMP_Sr		(040)	/* supervisor read permission */
#define PMP_Sw		(020)	/* supervisor write permission */
#define	PMP_Sx		(010)	/* supervisor execute permission */

#define PMP_Ur		(04)	/* user read permission */
#define PMP_Uw		(02)	/* user write permission */
#define	PMP_Ux		(01)	/* user execute permission */
 
#define PMP_NONE	(0)	/* no permissions */
#define PMP_SALL	(070)	/* free supervisor */
#define PMP_UALL	(007)	/* free user */
#define PMP_ALL		(077)	/* real loose */

/*
 *	memory spaces available
 */
enum pm_type {		/* possible address spaces on board */
	PM_MEM		= 0,		/* on-board memory */
	PM_IO		= 1,		/* on-board I/O */
	PM_BUSMEM	= 2,		/* bus memory (MB or VME) */
	PM_BUSIO	= 3		/* bus io (MB or VME) */
	/* 4-7 are reserved for futures */
};
#define PM_MEMORY	PM_MEM	/* for compatibility */
/*
 *	io pages we know about (needs cast to unsigned in use)
 */
enum pm_iopage {
	IOPG_PROM,	/* 0 = onboard prom */
	IOPG_FLOAT,	/* 1 = floating point chip */
	IOPG_DES,	/* 2 = encryption chip */
	IOPG_PARALLEL,	/* 3 = parallel port */
	IOPG_SERIAL0,	/* 4 = first dual serial scc */
	IOPG_TIMER,	/* 5 = am9513 timer chip */
	IOPG_ROP,	/* 6 = RasterOp chip */
	IOPG_CLOCK,	/* 7 = NatSemi abortion of a RTC chip */
	IOPG_SERIAL1,	/* 8 = scc second dual port */
	IOPG_SERIAL2,	/* 9 = scc third dual port */
		/* 10-13 reserved at moment */
	IOPG_ETHER = 14,/* 14 = Ethernet Interface */
	IOPG_SCSI	/* 15 = SCSI bus interface */
};
/*
 *		These are the layouts of various MMU registers in
 *	both whole entry layout, and bitfield definitions.  To use,
 *	pass and return ??_reg.??_whole, and to play with afterwards,
 *	use ??_reg.??_field.??_whatever.
 *		The machdep.c routines are defined with ??_size
 *	usage (in and ??out)
 *
 *			WARNING WARNING
 *
 *	 DO NOT ATTEMPT TO PASS OR RETURN THE STRUCTS, the compiler
 *	has bugs on things which are not long aligned.
 */
typedef u_short	cx_size;
typedef u_char	sm_size;
typedef u_long	pg_size;

union cx_reg {
	cx_size	cx_whole;		/* supv and user contexts */
	struct cx_field {
		unsigned		:5;
		unsigned	cx_sup	:3;	/* supv [0..7] */
		unsigned		:5;
		unsigned	cx_user	:3;	/* user [0..7] */
	} cx_field;
};

union sm_reg {
	sm_size	sm_whole;		/* segment entry */
	struct sm_field {
		u_char		sm_pmeg;	/* pmeg group */
	} sm_field;
};

union pg_reg {
	pg_size	pg_whole;		/* page map entry */
	struct pg_field{
		unsigned	pg_valid:1;		/* valid bit */
		unsigned	pg_permission:6;	/* SrwxUrwx */
		enum pm_type	pg_space:3;		/* address space */
		unsigned	pg_accessed:1;		/* page accessed?? */
		unsigned	pg_modified:1;		/* page modified?? */
		unsigned	:4;
		unsigned	pg_pagenum:16;		/* page number */
	} pg_field;
};
/*
 *	some handy typedefs
 */
typedef union cx_reg	cx_t;
typedef union sm_reg	sm_t;
typedef union pg_reg	pg_t;
/*
 *	the similar definitions of things in fc3 space that are not
 *	MMU related, same caveats apply as above
 */
typedef u_char	id_size, led_size;
typedef u_short	berr_size, enable_size;

union id_reg {
	id_size	id_whole;	/* id prom */
	struct id_field {			/* fields later??*/
		u_char	id_byte;
	} id_field;
};
/*
 * the setledreg routine will invert the given led pattern,
 * because 0=lit, so passed lit=1
 */
union led_reg {
	led_size	led_whole;	/* led (diagnostic) register */
	struct led_field {
		unsigned	led_d7	:1;	/* individual bits inverted */
		unsigned	led_d6	:1;
		unsigned	led_d5	:1;
		unsigned	led_d4	:1;
		unsigned	led_d3	:1;
		unsigned	led_d2	:1;
		unsigned	led_d1	:1;
		unsigned	led_d0	:1;
	} led_field;
};

union berr_reg {
	berr_size	berr_whole;	/* bus error register */
	struct berr_field {
		unsigned			:8;	/* high byte not used */
		unsigned	berr_pagevalid	:1;
		unsigned			:2;	/* reserved */
		unsigned	berr_busmaster	:1;	/* p1 bus master ??*/
		unsigned	berr_proterr	:1;	/* protection error ??*/
		unsigned	berr_timeout	:1;	/* timeout error ??*/
		unsigned	berr_parerru	:1;	/* upper byte parity */
		unsigned	berr_parerrl	:1;	/* lower byte parity */
	} berr_field;
};

union enable_reg {
	enable_size	enable_whole;	/* system enable register */
	struct enable_field {
		unsigned			:8;	/* high byte not used */
		unsigned	enable_normal	:1;	/* 0 = bootstate */
		unsigned	enable_ints	:1;	/* enable all ints */
		unsigned	enable_dvma	:1;	/* enable dvma */
		unsigned	enable_parcheck	:1;	/* enable party check */
		unsigned	enable_int3	:1;	/* cause int3 */
		unsigned	enable_int2	:1;	/* cause int2 */
		unsigned	enable_int1	:1;	/* cause int1 */
		unsigned	enable_pargen	:1;	/* enable party set */
	} enable_field;
};
/*
 *	more less handy typedefs
 */

typedef	union id_reg		id_t;
typedef	union led_reg		led_t;
typedef	union berr_reg		berr_t;
typedef	union enable_reg	enable_t;

/*
 *	definition of the exception vector table
 *	accessed with ex_vector->?????
 */
struct mc68k_vector {
	char	*e_initsp;	/* 0  initial stack pointer */
	int	(*e_initpc)();	/* 1  initial program counter */
	int	(*e_buserr)();	/* 2  buserror */
	int	(*e_addrerr)();	/* 3  address error */
	int	(*e_illinst)();	/* 4  illegal instruction */
	int	(*e_zerodiv)();	/* 5  divide by zero */
	int	(*e_chk)();	/* 6  CHK instruction */
	int	(*e_trapv)();	/* 7  TRAPV instruction */
	int	(*e_priv)();	/* 8  privilege violation */
	int	(*e_trace)();	/* 9  trace */
	int	(*e_line10)();	/* 10 line 1010 emulator */
	int	(*e_line15)();	/* 11 line 1111 emulator */
	int	(*e_res0[2])();	/* 12-13 reserved by Motorola */
	int	(*e_format)();	/* 14 format error */
	int	(*e_uninit)();	/* 15 uninitialized interrupt vector */
	int	(*e_res1[8])();	/* 16-23 reserved blah blah */
	int	(*e_int[8])();	/* 24 is spurious, 25-31 are auto level 1-7*/
	int	(*e_trap[16])();	/* 32-47 are trap#n vectors */
	int	(*e_res2[16])();	/* 48-63 are reserved */
	int	(*e_user[192])();	/* 64-255 are user int vectors */
};
#define spurious	e_int[0]
#define	ex_vector	((struct mc68k_vector *)0)
