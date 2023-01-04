/*	@(#)fifo.h 1.1 86/09/25 SMI;	*/

#ifdef KERNEL
/*
 *	Configuration Constants
 *	SHOULD BE SETTABLE BY "/etc/config".
 */
#define FIFOBUF	4096	/* max # bytes stored in a fifo */
#define FIFOMAX ~0	/* largest size of a single write to a fifo */
#define FIFOBSZ 4096	/* number of data bytes in each fifo data buffer */
#define FIFOMNB FIFOBUF*10	/* total # bytes allowed for all fifos */

/*
 *NOTE: When FIFOBUF == FIFOBSZ, a single buffer is used for each fifo.
 *	Multiple, linked buffers will be used if FIFOBUF > FIFOBSZ.
 *	In this case, FIFOBUF should be a multiple of FIFOBSZ and,
 *	in order to minimize unnecessary fragmentation, FIFOBSZ should
 *	probably be set to a power of 2 minus 4 (e.g., 4092).
 *
 *	Note, also, that this decision is made at compile-time so
 *	the run-time modification of fifoinfo parameters is dangerous.
 */
#if (FIFOBUF > FIFOBSZ)
struct fifo_bufhdr {
	struct fifo_bufhdr *fb_next;	/* ptr to next buffer */
	char fb_data[1];
};
#define FIFO_BUFHDR_SIZE (sizeof(struct fifo_bufhdr *))
#define FIFO_BUFFER_SIZE (fifoinfo.fifobsz + FIFO_BUFHDR_SIZE)

#else /*(FIFOBUF == FIFOBSZ)*/
struct fifo_bufhdr {
	union {
		struct fifo_bufhdr *fu_next;
		char fu_data[1];	/* must be at first byte in buffer */
	} fb_u;
};
#define fb_next fb_u.fu_next
#define fb_data fb_u.fu_data
#define FIFO_BUFFER_SIZE (fifoinfo.fifobsz)
#endif



/*
 *	Fifo information structure.
 */
struct fifoinfo {
	int	fifobuf,	/* max # bytes stored in a fifo */
		fifomax,	/* largest size of a single write to a fifo */
		fifobsz,	/* # of data bytes in each fifo data buffer */
		fifomnb;	/* max # bytes reserved for all fifos */
};
#define PIPE_BUF (fifoinfo.fifobuf)
#define PIPE_MAX (fifoinfo.fifomax)
#define PIPE_BSZ (fifoinfo.fifobsz)
#define PIPE_MNB (fifoinfo.fifomnb)


int fifo_alloc;			/* total number of bytes reserved for fifos */
struct fifoinfo	fifoinfo;	/* fifo parameters */

#endif KERNEL
