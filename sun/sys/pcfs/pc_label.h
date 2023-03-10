/*	@(#)pc_label.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * PC boot block defines.
 * can't be a structure because of padding (sigh).
 * All shorts are little endian
 */
#define PCB_BPSEC	11	/* (short) bytes per sector */
#define PCB_SPC		13	/* (byte) sectors per cluster */
#define PCB_RESSEC	14	/* (short) reserved sectors */
#define PCB_NFAT	16	/* (byte) number of fats */
#define PCB_NROOTENT	17	/* (short) number of root dir entries */
#define PCB_NSEC	19	/* (short) number of sectors on disk */
#define PCB_MEDIA	21	/* (byte) media descriptor */
#define PCB_SPF		22	/* (short) sectors per fat */
#define PCB_SPT		24	/* (short) sectors per track */
#define PCB_NHEAD	26	/* (short) number of heads */
#define PCB_HIDSEC	28	/* (short) number of hidden sectors */

/*
 * Media descriptor byte.
 * Found in the boot block and in the first byte of the fat.
 * Second and third byte must be 0xFF.
 */
#define SS8SPT		0xFE	/* single sided 8 sectors per track */
#define DS8SPT		0xFF	/* double sided 8 sectors per track */
#define SS9SPT		0xFC	/* single sided 9 sectors per track */
#define DS9SPT		0xFD	/* double sided 9 sectors per track */

#define PC_FATBLOCK	1	/* starting block number of fat */
#define PC_SECSIZE	512	/* pc filesystem sector size */
#define PC_SECSHIFT	9	/* log2 pc filesystem sector size */

/*
 * conversions to/from little endian format
 */
#ifdef vax
/* little endian machines */
ltohs(S)		(S)
ltohl(L)		(L)
htols(S)		(S)
htoll(L)		(L)
#endif

#ifdef sun
/* big endian machines */
#define getbyte(A, N)	(((unsigned char *)(&(A)))[N])
#define ltohs(S)	((getbyte(S,1) << 8) | getbyte(S,0))
#define ltohl(L)	((getbyte(L,3) << 24) | (getbyte(L,2) << 16) | \
			 (getbyte(L,1) << 8) | getbyte(L,0))
#define htols(S)	((getbyte(S,1) << 8) | getbyte(S,0))
#define htoll(L)	((getbyte(L,3) << 24) | (getbyte(L,2) << 16) | \
			 (getbyte(L,1) << 8) | getbyte(L,0))
#endif
