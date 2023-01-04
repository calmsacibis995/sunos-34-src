/*    @(#)tool.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - tool global variables
 */

extern  Frame mt_frame;			/* the mailtool frame */
extern	Textsw mt_headersw;		/* the header subwindow */
extern	Panel mt_cmdpanel;			/* the command panel */
extern	int mt_cmdpanel_fd;		/* the command panel subwindow fd */
extern	Textsw mt_msgsw;			/* the mail message subwindow */
extern	Textsw mt_replysw;			/* the mail reply subwindow */
extern	struct pixfont *mt_font;		/* the default font */

#ifdef NOTDEF	/*	I thin these are vestgial: jf	*/
extern	int charheight, charwidth;	/* size of default font */
#endif

extern	int mt_idle;			/* closed, not processing mail */
extern	int mt_nomail;			/* no mail in current folder */
extern	char *mt_wdir;			/* Mail's working directory */
extern	time_t mt_msg_time;		/* time msgfile was last written */

extern	Panel_item mt_deliver_item, mt_cancel_item;
extern	Panel_item mt_file_item, mt_info_item, mt_dir_item;
extern	Panel_item mt_pre_item;
