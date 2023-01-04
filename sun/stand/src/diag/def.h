/*	@(#)def.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define INVALID 	0xffff
#define BYTES(bits)	(bits/8) + 1
#define BYTEFI(x,y)	(x << 8) | y
#define DLENGHT(x,y)	(x << 8) | y
#define SYNC_BYTE 	0x19
#define LAST_BYTE 	0xf0
#define HDR_BYTES 	24
#define DISKMAGIC 	0x89898989
#define NDEFCT		 1000
#define DEFSECT(x) 	((x*12) / SECSIZE) + 1	

#define adaptive(x, y, z)	( (x-y) >= 0 ? x-y : z + (x-y) )
#define span(x, y, z)		( (x%z)+BYTES(y) > z ? 1 : 0 )

#define XCYL(i)		defect[i].cyl
#define XHEAD(i) 	defect[i].head
#define XSECT(i) 	defect[i].sect
#define XNBITS(i) 	defect[i].nbits
#define XBFI(i) 	defect[i].bfi

#define NEWDISK		0x01
#define OLDISK		0x02

struct tdef {
    u_int bfi[4];
    u_int length[4];
};

struct dformat {
	u_char	byt0;	/* sync byte (should be 0x19) */
	u_char	byt1;	/* cylinder address (high) */
	u_char  byt2;   /* cylinder address (low) */
	u_char	byt3;	/* head */
	u_char  byt4;	/* zero */
	u_char  byt5;	/* high byte of first defect */
	u_char  byt6;	/* low byte of first defect */
	u_char  byt7;	/* high byte of length of first defect */
	u_char  byt8;	/* low byte of length of first defect */
	u_char  byt9;	/* high byte of second defect */
	u_char  byt10;	/* low byte of second defect */
	u_char  byt11;	/* high byte of length of second defect */
	u_char  byt12;	/* low byte of length of second defect */
	u_char  byt13;	/* high byte of third defect */
	u_char  byt14;	/* low byte of third defect */
	u_char  byt15;	/* high byte of length of third defect */
	u_char  byt16;	/* low byte of length of third defect */
	u_char  byt17;	/* high byte of fourth defect */
	u_char  byt18;	/* low byte of fourth defect */
	u_char  byt19;	/* high byte of length of fourth defect */
	u_char  byt20;	/* low byte of length of fourth defect */
	u_char  byt21;	/* last byte should be 0xf0 */
	u_char  byt22;	/* zero */
	u_char  byt23;	/* zero */
};
struct shift {
    u_char mask;
    u_char bits;
};

struct defhdr {		/* control block for saved defects table */
    int magicno;	/* some magic number */
    int count;		/* number of defects */
    int chksum;
    int save[125];	/* make it an even sector */
};

struct defect_list {
    short cyl;
    short head;
    short sect;
    short nbits;	/* number of bits */
    int bfi;		/* byte from index */
};
