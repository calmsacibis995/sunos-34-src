/*	@(#)trace.h 1.3 86/10/13 SMI; from UCB 4.3 83/05/25	*/

/*
 * Routing table management daemon.
 */

/*
 * Trace record format.
 */
struct	iftrace {
	time_t	ift_stamp;		/* time stamp */
	struct	sockaddr ift_who;	/* from/to */
	char	*ift_packet;		/* pointer to packet */
	short	ift_size;		/* size of packet */
	short	ift_metric;		/* metric on associated metric */
};

/*
 * Per interface packet tracing buffers.  An incoming and
 * outgoing circular buffer of packets is maintained, per
 * interface, for debugging.  Buffers are dumped whenever
 * an interface is marked down.
 */
struct	ifdebug {
	struct	iftrace *ifd_records;	/* array of trace records */
	struct	iftrace *ifd_front;	/* next empty trace record */
	struct	interface *ifd_if;	/* for locating stuff */
};

/*
 * Packet tracing stuff.
 */
int	tracepackets;		/* watch packets as they go by */
int	tracing;		/* bitmask: */
# define ACTION_BIT 0x0001
# define INPUT_BIT  0x0002
# define OUTPUT_BIT 0x0004
FILE	*ftrace;		/* output trace file */

#define	TRACE_ACTION(action, route) { \
	  if (tracing & ACTION_BIT) \
		traceaction(ftrace, "action", route); \
	}
#define	TRACE_INPUT(ifp, src, size) { \
	  if (tracing & INPUT_BIT) { \
		ifp = if_iflookup(src); \
		if (ifp) \
			trace(&ifp->int_input, src, packet, size, \
				ntohl(ifp->int_metric)); \
	  } \
	  if (tracepackets) \
		dumppacket(stdout, "from", src, packet, size); \
	}
#define	TRACE_OUTPUT(ifp, dst, size) { \
	  if (tracing & OUTPUT_BIT) \
		trace(&ifp->int_output, dst, packet, size, ifp->int_metric); \
	  if (tracepackets) \
		dumppacket(stdout, "to", dst, packet, size); \
	}
