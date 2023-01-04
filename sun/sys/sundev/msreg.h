/*      @(#)msreg.h 1.1 86/09/25 SMI      */


/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Software mouse registers
 */
struct	ms_softc {
	struct	mousebuf {
		short	mb_size;	/* size (in mouseinfo units) of buf */
		short	mb_off;		/* current offset in buffer */
		struct	mouseinfo {
			char	mi_x, mi_y;
			char	mi_buttons;
#define	MS_HW_BUT1		0x4	/* left button position */
#define	MS_HW_BUT2		0x2	/* middle button position */
#define	MS_HW_BUT3		0x1	/* right button position */
			struct	timeval mi_time; /* timestamp */
		} mb_info[1];		/* however many samples */
	} *ms_buf;
	short	ms_bufbytes;		/* buffer size (in bytes) */
	short	ms_flags;		/* currently unused */
	short	ms_oldoff;		/* index into mousebuf */
	short	ms_oldoff1;		/* at mi_x, mi_y or mi_buttons... */
	short	ms_readformat;		/* format of read stream */
#define	MS_3BYTE_FORMAT	VUID_NATIVE	/* 3 byte format (buts/x/y) */
#define	MS_VUID_FORMAT	VUID_FIRM_EVENT	/* vuid Firm_event format */
	short	ms_vuidaddr;		/* vuid addr for MS_VUID_FORMAT */
	short	ms_vuidcount;		/* count of unread firm events */
	short	ms_samplecount;		/* count of unread mouseinfo samples */
	char	ms_readbuttons;		/* button state as of last read */
};

#ifdef KERNEL
#define	MSIOGETBUF	_IOWR(m, 1, int)/* MSIOGETBUF is OBSOLETE */
	/* Get mouse buffer ptr so (window system in particular) can chase
	   around buffer to get events. */
#endif
