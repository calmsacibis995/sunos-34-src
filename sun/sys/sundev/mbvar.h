/*	@(#)mbvar.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _MBVAR_
#define _MBVAR_

/*
 * This file contains definitions related to the kernel structures
 * for dealing with the Mainbus.
 *
 * The Mainbus has an mb_hd structure.
 * Each Mainbus controller which is not a device has an mb_ctlr structure.
 * Each Mainbus device has an mb_device structure.
 */

/*
 * The Mainbus structure.
 *
 * At boot time we determine the devices attached to the Mainbus.
 *
 * During normal operation, resources are allocated and returned
 * to the structures here. 
 * 
 * When Mainbus resources are needed and not available, or if a device
 * which can tolerate no other Mainbus activity gets on the bus,
 * then device drivers may have to wait to get to the bus and are
 * queued here.
 */
struct	mb_hd {
	struct	mb_ctlr *mh_actf;	/* head of queue to transfer */
	struct	mb_ctlr *mh_actl;	/* tail of queue to transfer */
	struct	map *mh_map;		/* DVMA resource map */
	short	mh_users;		/* transient use count */
	short	mh_xclu;		/* a bus hog is using the Mainbus */
	char	mh_mrwant;		/* someone is waiting for map reg */
};

/*
 * Per-controller structure.
 * (E.g. one for each disk and tape controller)
 *
 * If a controller has devices attached, then there are
 * cross-referenced mb_device structures.
 * This structure is the one which is queued in Mainbus resource wait,
 * and saves the information about Mainbus resources which are used.
 * The queue of devices waiting to transfer is also attached here.
 */
struct mb_ctlr {
	struct	mb_driver *mc_driver;
	short	mc_ctlr;	/* controller index in driver */
	short	mc_alive;	/* controller exists */
	caddr_t	mc_addr;	/* address of device in i/o space */
	int	mc_intpri;	/* interrupt priority level */
	struct	vec *mc_intr;	/* if vectored interrupts used */
	u_int	mc_space;	/* encode bits for addr device space */
	struct	mb_hd *mc_mh;
	int	mc_mbinfo;	/* Mainbus resource allocation info */
	char	mc_rw;		/* READ or WRITE */
	struct	buf mc_tab;	/* queue of devices for this controller */
	struct	mb_ctlr	*mc_forw;	/* link in transfer queue */
};

/*
 * Per ``device'' structure.
 * (A controller has devices or uses and releases Mainbus memory).
 * (Everything else is a ``device''.)
 *
 * If a controller has many drives attached, then there will
 * be several mb_device structures associated with a single mb_ctlr
 * structure.  Some of the new drivers use this structure in conjunction
 * with the mb_ctlr structure to queue for Mainbus resources.
 *
 * This structure contains all the information necessary to run a Mainbus
 * device such as a serial line interface.  It also contains information
 * for slaves of Mainbus controllers as to which device on the slave
 * this is.  A flags field here can also be given in the system specification
 * and is used to tell which mux lines are hard wired or other device
 * specific parameters.
 */
struct mb_device {
	struct	mb_driver *md_driver;
	short	md_unit;	/* unit number on the system */
	short	md_ctlr;	/* mass ctlr number; -1 if none */
	short	md_slave;	/* slave on controller */
	caddr_t	md_addr;	/* address of device in i/o space */
	int	md_intpri;	/* interrupt priority */
	short	md_dk;		/* if init 1 set to number for iostat */
	int	md_flags;	/* parameter from system specification */
	struct	vec *md_intr;	/* if vectored interrupts used */
	u_int	md_space;	/* encode bits for addr device space */
	short	md_alive;	/* device exists */
	short	md_type;	/* driver specific type information */
/* if the device is connected to a controller, this is the controller */
	struct	mb_ctlr *md_mc;
	struct	mb_hd *md_hd;
	struct	diskhd md_utab;		/* queueing per unit */
	struct	mb_device *md_forw;	/* link in transfer queue */
	int 	md_mbinfo;		/* Mainbus resource allocation info */
};

/*
 * Per-driver structure.
 *
 * Each Mainbus driver defines entries for a set of routines
 * as well as an array of types which are acceptable to it.
 * These are used at boot time by the configuration program.
 */
struct mb_driver {
	int	(*mdr_probe)();		/* see if a driver is really there */
	int	(*mdr_slave)();		/* see if a slave is there */
	int	(*mdr_attach)();	/* setup driver for a slave */
	int	(*mdr_go)();		/* routine to start transfer */
	int	(*mdr_done)();		/* routine to finish transfer */
	int	(*mdr_intr)();		/* polling interrupt routine */
	int	mdr_size;		/* amount of memory space needed */
	char	*mdr_dname;		/* name of a device */
	struct	mb_device **mdr_dinfo;	/* backpointers to mbdinit structs */
	char	*mdr_cname;		/* name of a controller */
	struct	mb_ctlr **mdr_cinfo;	/* backpointers to mbcinit structs */
	short	mdr_flags;		/* Mainbus usage flags */
	struct	mb_driver *mdr_link;	/* interrupt routine linked list */
};

/* Driver flags */
#define	MDR_XCLU	0x01		/* needs exclusive use of bus */
#define	MDR_BIODMA	0x02		/* (block device) does Mainbus DMA */
#define	MDR_SWAB	0x04		/* Mainbus buffer must be swabbed */
#define	MDR_OBIO	0x08		/* device in on-board I/O space */
#define	MDR_DMA		0x10		/* (nonblock device) does Mainbus DMA */


/* Flags to mbsetup */
#define	MB_CANTWAIT	0x01		/* don't block me */

/*
 * If mbcookie.mbi_mapreg < dvmasize, then mbi_mapreg is the
 * starting map register within DVMA space which is mapped
 * in mbsetup() and unmapped in mbrelse().  Otherwise
 * mbi_mapreg is the starting page of contigously mapped
 * region within an external "mainbus" mapping region
 * (e.g. Multibus Memory or VMEbus) as checked and
 * approved by buscheck().
 */
struct mbcookie {
unsigned int
		mbi_mapreg:(32-PGSHIFT),/* starting map register (page) */
		mbi_offset:PGSHIFT;	/* byte offset */
};

/*
 * Macros to bust return word from map allocation routines.
 */
#define	MBI_MR(i)	((int)((unsigned)(i)>>PGSHIFT))
#define	MBI_ADDR(i)	((int)(i))
#define MBI_OFFSET(i)   ((int)((unsigned)(i)&PGOFSET))

struct vec {
	int	(*v_func)();		/* interrupt function to call */
	int	v_vec;			/* vector number (64-255) */
	int	*v_vptr;		/* pointer to value passed */
};

/* Convert interrupt prio to SR */
#define pritospl(n)	(SR_SMODE|((n)<<8))

#ifdef KERNEL
/*
 * Maximum interrupt priority used by Mainbus DMA.
 * This value is now a variable determined by auto configuration code by
 * taking the max of the m[cd]_intpri field from the mb_{cltr,device}
 * structures which are found to exist and have the MDR_BIODMA or
 * MDR_DMA flags on in their corresponding mb_driver structure.
 */
extern	int SPLMB;

/*
 * Mainbus related kernel variables
 */
extern	struct	mb_hd mb_hd;

/*
 * Mbcinit and mbdinit initialize the mass storage controller and
 * device tables specifying possible devices.
 */
extern	struct	mb_ctlr mbcinit[];
extern	struct	mb_device mbdinit[];

#ifdef sun2
extern	char mbio[];			/* mb device addr space */
#endif

/*
 * NOTE: the xy and xd drivers assume that the IOPBMAP is wholly 
 * contained in the first 64K of DVMA space.
 */

struct	map *iopbmap;
#define IOPBMEM		(0x2000/NBPG)	/* 8k's worth of pages for IOPB crud */
#define IOPBMAPSIZE	64

#define	DVMAMAPSIZE	25
extern	char DVMA[];

#endif KERNEL

#endif _MBVAR_
