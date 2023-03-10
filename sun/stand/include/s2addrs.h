/*	@(#)s2addrs.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*	@(#)s2addrs.h 1.19 83/10/10 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Memory Addresses of Interest in the Sun-2 "Standalone" memory map
 *
 * This file is used for both standalone code (ROM Monitor, 
 * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
 * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
 * COPY, and make sure the change is upward-compatible.  The last thing we 
 * need is seventeen different copies of this file, like we have with the
 * Sun-1 header files.
 *
 * THE VALUE FOR PROM_BASE must be changed in several places when it is
 * changed.  A complete list:
 *	../conf/Makefile	RELOC= near top of file
 *	../h/sunromvec.h	#define romp near end of file
 *	../* /Makefile		(best to rerun sunconfig to generate them)
 * Since sunromvec is exported to the world, such a change invalidates all
 * object programs that use it (eg standalone diagnostics, demos, boot code,
 * etc) until they are recompiled.  As a transition aid, it is often useful
 * to specially map the old sunromvec location(s) so they also work.
 */

/*
 * The following define the base addresses in mappable memory where
 * the various devices and forms of memory are mapped in when the ROM
 * Monitor powers up.  Ongoing operation of the monitor requires that
 * some of these locations remain mapped as they were.
 */
#define	MAINMEM_BASE	((char *)			0x000000)
#define		MAINMEM_SIZE				0x600000

#define	HOLE_BASE	((char *)			0x600000)
#define		HOLE_SIZE				0x800000

#define	UNUSED_BASE	((char *)			0xE00000)

#define	INVPMEG_BASE	((char *)			0xEA8000)

#define	MBIO_BASE	((char *)			0xEB0000)

#define	VIDEOMEM_BASE	((char *)			0xEC0000)

/* On-board and off-board I/O devices of interest */
#define	TIMER_BASE	((struct am9513_device *)	0xEE0000)
#define	ROP_BASE	((struct mem_regs *)		0xEE0800)
#define	CLOCK_BASE	((struct ns58167 *)		0xEE1000)
#define	DES_BASE	((struct deschip *)		0xEE1800)
#define	PARALLEL_BASE	((short *)			0xEE2000)
#define	SCSI_BASE	((struct scsi_ha_reg *)		0xEE2800)
#define	ETHER_BASE	((struct i82587_device *)	0xEE3000)
#define	VIDEOCTL_BASE	((struct videoctl *)		0xEE3800)
#define	SOUND_BASE	((struct ti76489_device *)	0xEE4000)
#define LAST_IO_BASE	((char *)			0xEE4800)

#define	KEYBMOUSE_BASE	((struct zscc_device *)		0xEEC000)
#define	SERIAL0_BASE	((struct zscc_device *)		0xEEC800)
/* If further serial ports exist, they are mapped in successive pages here */

#define	PROM_BASE	((struct sunromvec *)		0xEF0000)

#define	MBMEM_BASE	((char *)			0xF00000)


/*
 * The Monitor maps only as much main memory as it can detect; the rest
 * of the address space (up thru the special addresses defined above)
 * is mapped as invalid.
 *
 * The last pmeg in the page map is always filled with PAGE_INVALID
 * entries.  This pmeg number ("The Invalid Pmeg Number") can be used to
 * invalidate an entire segment's worth of memory.  All segments of all contexts
 * except 0 are mapped using this pmeg number and thus are totally invalid.
 *		B E   C A R E F U L !
 * If you change a page map entry in this pmeg, you change it for thousands
 * of virtual addresses.  (The standard getpagemap/setpagemap routines
 * will cause a trap if you attempt to write a valid page map entry to this
 * pmeg, but you could do it by hand if you really wanted to mess things up.)
 *
 * Because there is twice as much virtual memory space in a single context
 * as there are total pmegs to map it with, half of the monitor's own memory
 * map must be re-mappings of the same pmegs.  There's no reason to duplicate
 * useful addresses, and several reasons not to, so we map the extra 8 megs
 * of virtual address space with the Invalid Pmeg Number.  This means that
 * 8 megs of the address space have their own page map entries, and the other
 * 8 megs all share the one Invalid Pmeg.  Remember this when trying to map 
 * things; if the address you want is segmapped to the Invalid Pmeg, you'd
 * better find it a pmeg before you set a page map entry.  The entries which 
 * use the Invalid Pmeg start at HOLE_BASE (defined above) and extend HOLE_SIZE
 * bytes (8 megs).
 *
 * The Monitor always uses page map entry PAGE_INVALID to map an invalid
 * page, although the only relevant bits are the (all zero) permission bits
 * and the Valid bit.
 */
#define	SEG_INVALID	NUMPMEGS-1
extern struct pgmapent PAGE_INVALID;

/*
 * The 68010 processor allows the interrupt vector table to reside anywhere
 * in memory.  Its address is contained in the "Vector Base Register",
 * which can be set with the  movc  instruction.  The monitor's VBR value
 * is defined below.  All accesses to the interrupt vectors should use
 * this value as a base register, or should read out the actual VBR contents
 * with movc.
 */
#define	VECTOR_BASE	(long *)		0x0
