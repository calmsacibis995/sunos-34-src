/*      @(#)glob.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - global constants
 */
#define	MAXMSGS		1000	/* maximum number of messages supported */
#define	MAXFOLDERS	200	/* maximum number of folders supported */
#define	DEFMAXFILES	10	/* default max # of file names in file menu */

/*
 * Mailtool - global variables
 */

extern	int mt_aborting;		/* aborting, don't save messages */
extern	char *mt_cmdname;		/* our name */
extern	int  mt_mailclient;	/* client handle */

extern	int mt_curmsg;		/* number of current message */
extern	int mt_prevmsg;		/* number of previous message */
extern	int mt_maxmsg;		/* highest numbered message */
extern	int mt_scandir;		/* scan direction */
extern	char *mt_mailbox;		/* name of user's mailbox */
extern	char *mt_folder;		/* name of current folder */
extern	char *mt_info;		/* info from save, file, etc. */

struct	msg {
	int	m_start;	/* start char of header in headersw */
	char	*m_header;	/* text from header line */
	struct	msg *m_next;	/* linked list of deleted messages */
	int	m_deleted:1;	/* message has been deleted */
};

extern	struct msg *mt_message;	/* all the messages */

extern	struct icon *mt_icon_ptr;
extern	struct icon mt_unknown_icon;
extern	struct icon mt_mail_icon;
extern	struct icon mt_nomail_icon;

extern	char mt_hdrfile[];
extern	char mt_msgfile[];
extern	char mt_replyfile[];
extern	char mt_printfile[];
extern	char mt_dummybox[];

extern	char *getenv();
extern	char *index();
extern	char *mt_savestr();
extern	char *mt_value();
extern	char **mt_get_folder_list();
