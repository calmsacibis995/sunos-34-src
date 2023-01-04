/*	@(#)msgbuf.h 1.1 86/09/25 SMI; from UCB 4.3 81/08/30	*/

#define	MSG_MAGIC	0x063060
#define	MSG_BSIZE	(2048 - 2 * sizeof (long))
struct	msgbuf {
	long	msg_magic;
	long	msg_bufx;
	char	msg_bufc[MSG_BSIZE];
};
#ifdef KERNEL
struct	msgbuf msgbuf;
#endif
