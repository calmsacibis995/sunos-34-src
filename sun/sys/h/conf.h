/*	@(#)conf.h 1.1 86/09/25 SMI; from UCB 4.11 83/05/18	*/

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_dump)();
	int	(*d_psize)();
	int	d_flags;
};
#ifdef KERNEL
extern struct bdevsw bdevsw[];
#endif

/*
 * Character device switch.
 */
struct cdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_stop)();
	int	(*d_reset)();
	struct	tty *d_ttys;
	int	(*d_select)();
	int	(*d_mmap)();
	struct	streamtab *d_str;
};
#ifdef KERNEL
extern struct cdevsw cdevsw[];
#endif

/*
 * tty line control switch.
 */
struct linesw {
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_rint)();
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)();
	int	(*l_modem)();
};
#ifdef KERNEL
extern struct linesw linesw[];
#endif

/*
 * Swap device information
 */
struct swdevt {
	dev_t	sw_dev;
	int	sw_freed;
	int	sw_nblks;
	struct vnode *sw_vp;
};
#ifdef KERNEL
extern struct swdevt swdevt[];
#endif

#ifdef STREAMS
/*
 * Streams module information
 */
#define	FMNAMESZ	8

struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct  streamtab *f_str;
};
#ifdef KERNEL
extern struct fmodsw fmodsw[];
extern int fmodcnt;
#endif
#endif
