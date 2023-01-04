/*	@(#)video.h 1.4 84/06/08 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun-2 video display definitions
 */

#ifndef MEMPG_VIDEO

/*
 * On the Multibus system, the frame buffer is mapped like memory.
 * On the VMEBus, the frame buffer is mapped in I/O space.  The VMEBus
 * page definitions are in s2map.h.  The page definitions in this file
 * are for the Multibus only.
 */

#ifndef VME
/*
 * When the page type is PM_MEM, the page number field can select video
 * memory or registers, at the top of the address space.
 */
#define	MEMPG_VIDEO		(0xFFF00000 >> BYTES_PG_SHIFT)
#define	MEMPG_VIDEO_ZSCC	(0xFFF80000 >> BYTES_PG_SHIFT)
#define MEMPG_VIDEO_CTRL	(0xFFF81800 >> BYTES_PG_SHIFT)
#endif VME

/*
 * The video memory is just memory, tho it can also copy data that is
 * written to other locations.
 *
 * The A-side of the ZSCC connects to the keyboard.
 * The B-side of the ZSCC connects to the mouse.
 *
 * The video control register is arranged as shown below.
 *
 * Vc_copybase specifies the base (physical) address in main memory
 * where, if a write is done, the write is also done to the frame buffer.
 * Note that copying only works on 128K boundaries even tho the base address
 * is specified in 64K units.  This makes it easier to set since you
 * just lop off the bottom 16 bits of the address.
 */
struct videoctl {
	unsigned vc_video_en:1;		/* Video enable */
	unsigned vc_copy_en:1;		/* Copy enable */
	unsigned vc_int_en:1;		/* Interrupt enable */
	unsigned vc_int:1;		/* Int active - r/o */
	unsigned vc_b_jumper:1;		/* Config jumper, 0=default */
					/* FIXME: 1=manufacturing burnin */
					/* This is a 'temporary' kludge */
	unsigned vc_a_jumper:1;		/* Config jumper, 0=default */
	unsigned vc_color_jumper:1;	/* Config jumper, 0=default */
					/* 1=use S-2 color as console */
	unsigned vc_1024_jumper:1;	/* Config jumper, 0=default */
					/* 1=screen is 1024*1024 */
	unsigned vc_copybase:8;		/* Base addr of memory to copy from */
};
#define VIDEO_COPY_SHIFT	16	/* In 64K units (16 bit shift) */

#endif
